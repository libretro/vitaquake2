
#include <stdlib.h>
#include <string.h>

#include <libretro_file.h>
#include <string/stdstring.h>

/* Tremor: fixed-point (integer) Ogg Vorbis decoder.  Unlike stb_vorbis it
 * decodes entirely in integer arithmetic, so the PCM it produces is identical
 * on every platform -- the CD/music path is now deterministic int16 end to
 * end (decode, resample and mix), matching the SFX mixer. */
#include <ivorbisfile.h>

#include "../game/q_shared.h"
#include "../client/cdaudio.h"

extern char g_music_dir[1024];
extern bool cdaudio_enabled;
/* Output sample rate in force this session (resolved in libretro.c from the
 * "Sound Samplerate (Hint)" core option); the OGG stream is resampled to it. */
extern int audio_sample_rate;

/* Shared soft clipper from the SFX mixer (client/snd_mix.c). */
extern int S_SoftClip(int v);
extern float S_SoftClipNormF(float v);

/* ------------------------------------------------------------------ */
/* In-memory Ogg source for Tremor's ov_open_callbacks                */
/* ------------------------------------------------------------------ */
typedef struct
{
   const unsigned char *data;
   size_t               size;
   size_t               pos;
} ogg_mem_t;

static size_t ogg_mem_read(void *ptr, size_t size, size_t nmemb, void *ds)
{
   ogg_mem_t *m = (ogg_mem_t *)ds;
   size_t want  = size * nmemb;
   size_t avail = m->size - m->pos;

   if (size == 0 || avail == 0)
      return 0;
   if (want > avail)
      want = avail - (avail % size);   /* whole elements only */
   memcpy(ptr, m->data + m->pos, want);
   m->pos += want;
   return want / size;
}

static int ogg_mem_seek(void *ds, ogg_int64_t off, int whence)
{
   ogg_mem_t  *m = (ogg_mem_t *)ds;
   ogg_int64_t base, np;

   switch (whence)
   {
      case SEEK_SET: base = 0;                     break;
      case SEEK_CUR: base = (ogg_int64_t)m->pos;   break;
      case SEEK_END: base = (ogg_int64_t)m->size;  break;
      default:       return -1;
   }
   np = base + off;
   if (np < 0 || np > (ogg_int64_t)m->size)
      return -1;
   m->pos = (size_t)np;
   return 0;
}

static long ogg_mem_tell(void *ds)
{
   return (long)((ogg_mem_t *)ds)->pos;
}

/* ------------------------------------------------------------------ */
/* Streaming decode + 16.16 fixed-point cubic resample state          */
/* ------------------------------------------------------------------ */
#define CD_DEC_FRAMES 2048

static OggVorbis_File cd_vf;
static ogg_mem_t      cd_src;
static void          *cd_file    = NULL;   /* owned ogg bytes */
static bool           cd_open    = false;
static bool           cd_playing = false;
static bool           cd_loop    = false;
static int            cd_channels = 2;

static int16_t        cd_dec[CD_DEC_FRAMES * 2]; /* decoded, expanded to stereo */
static int            cd_dec_n   = 0;
static int            cd_dec_i   = 0;

static uint32_t       cd_step    = 0x10000;      /* src_rate / out_rate, 16.16 */
static uint32_t       cd_frac    = 0;
static int16_t        cd_s0[2]   = {0, 0};        /* 4-tap window for cubic resample; */
static int16_t        cd_s1[2]   = {0, 0};        /* the active segment is cd_s1 -> cd_s2, */
static int16_t        cd_s2[2]   = {0, 0};        /* with cd_s0 / cd_s3 as the outer taps. */
static int16_t        cd_s3[2]   = {0, 0};
static bool           cd_primed  = false;

static void cd_close(void)
{
   if (cd_open)
      ov_clear(&cd_vf);          /* close_func is NULL; we free cd_file below */
   cd_open    = false;
   cd_playing = false;
   cd_primed  = false;
   cd_dec_n   = cd_dec_i = 0;
   cd_frac    = 0;
   if (cd_file)
      free(cd_file);
   cd_file    = NULL;
}

/* Pull one stereo source frame; returns 0 when the stream ends (no loop). */
static int cd_pull(int16_t out[2])
{
   while (cd_dec_i >= cd_dec_n)
   {
      char     raw[CD_DEC_FRAMES * 2 * sizeof(int16_t)];
      int      bs   = 0;
      int      want = CD_DEC_FRAMES * cd_channels * (int)sizeof(int16_t);
      long     got;
      int      frames, k;
      int16_t *s;

      if (want > (int)sizeof(raw))
         want = (int)sizeof(raw);

      got = ov_read(&cd_vf, raw, want, &bs);
      if (got <= 0)
      {
         if (got == 0 && cd_loop && ov_pcm_seek(&cd_vf, 0) == 0)
            got = ov_read(&cd_vf, raw, want, &bs);
         if (got <= 0)
            return 0;
      }

      frames = (int)(got / ((long)sizeof(int16_t) * cd_channels));
      s      = (int16_t *)raw;
      for (k = 0; k < frames; k++)
      {
         if (cd_channels == 1)
            cd_dec[k * 2] = cd_dec[k * 2 + 1] = s[k];
         else
         {
            cd_dec[k * 2]     = s[k * cd_channels];
            cd_dec[k * 2 + 1] = s[k * cd_channels + 1];
         }
      }
      cd_dec_n = frames;
      cd_dec_i = 0;
      if (frames == 0)
         return 0;
   }

   out[0] = cd_dec[cd_dec_i * 2];
   out[1] = cd_dec[cd_dec_i * 2 + 1];
   cd_dec_i++;
   return 1;
}

/* Catmull-Rom cubic across the 4-tap window (cd_s0..cd_s3) at the current
 * 16.16 fractional position; the active segment is cd_s1 -> cd_s2. This is a
 * higher-order reconstruction than linear interpolation, so upsampling this
 * (full-bandwidth) music source keeps the low/mid band - where the musical
 * energy sits - far more accurate. Integer throughout; result is clamped to
 * int16 since a cubic can overshoot the sample range on sharp transitions. */
static void cd_interp(int res[2])
{
   int     ch;
   int64_t f = cd_frac;

   for (ch = 0; ch < 2; ch++)
   {
      int64_t im1 = cd_s0[ch];
      int64_t i0  = cd_s1[ch];
      int64_t i1  = cd_s2[ch];
      int64_t i2  = cd_s3[ch];
      int64_t a   = i1 - im1;
      int64_t b   = 2*im1 - 5*i0 + 4*i1 - i2;
      int64_t c   = 3*(i0 - i1) + i2 - im1;
      int64_t inner = b + ((c * f) >> 16);
      int64_t mid   = a + ((inner * f) >> 16);
      int64_t outer = (mid * f) >> 16;
      int     v     = (int)(i0 + (outer >> 1));   /* i0 + 0.5*f*(a+f*(b+f*c)) */

      if (v >  32767) v =  32767;
      else if (v < -32768) v = -32768;
      res[ch] = v;
   }
}

/* Slide the window forward one source frame, pulling a fresh sample into
 * cd_s3. Returns 0 at end of stream. */
static int cd_advance(void)
{
   cd_s0[0] = cd_s1[0]; cd_s0[1] = cd_s1[1];
   cd_s1[0] = cd_s2[0]; cd_s1[1] = cd_s2[1];
   cd_s2[0] = cd_s3[0]; cd_s2[1] = cd_s3[1];
   return cd_pull(cd_s3);
}

/* Fill the window to start playback. cd_s0 holds the first sample (nothing
 * precedes the stream start); a stream shorter than the window duplicates the
 * last sample into the trailing tap. Returns 0 if there is nothing to play. */
static int cd_prime(void)
{
   if (!cd_pull(cd_s1))
      return 0;
   cd_s0[0] = cd_s1[0]; cd_s0[1] = cd_s1[1];
   if (!cd_pull(cd_s2))
      return 0;
   if (!cd_pull(cd_s3))
   {
      cd_s3[0] = cd_s2[0];
      cd_s3[1] = cd_s2[1];
   }
   cd_frac   = 0;
   cd_primed = true;
   return 1;
}

int CDAudio_Init(void)
{
   cd_close();
   return 1;
}

void CDAudio_Shutdown(void)
{
   cd_close();
}

void CDAudio_Play(int track, qboolean looping)
{
   char         file_name[32];
   char         file_path[1024];
   void        *file_contents = NULL;
   int64_t      file_len      = 0;
   ov_callbacks cbs;
   vorbis_info *vi;
   long         rate;
   uint64_t     step;

   cd_close();

   if (!cdaudio_enabled || string_is_empty(g_music_dir))
      return;

   file_name[0] = file_path[0] = '\0';
   snprintf(file_name, sizeof(file_name), "%02i.ogg", track);
   fill_pathname_join(file_path, g_music_dir, file_name, sizeof(file_path));
   if (!path_is_valid(file_path))
   {
      snprintf(file_name, sizeof(file_name), "track%02i.ogg", track);
      fill_pathname_join(file_path, g_music_dir, file_name, sizeof(file_path));
      if (!path_is_valid(file_path))
         return;
   }

   if (!filestream_read_file(file_path, &file_contents, &file_len) ||
       file_len < 1)
   {
      if (file_contents)
         free(file_contents);
      return;
   }

   cd_file     = file_contents;
   cd_src.data = (const unsigned char *)file_contents;
   cd_src.size = (size_t)file_len;
   cd_src.pos  = 0;

   cbs.read_func  = ogg_mem_read;
   cbs.seek_func  = ogg_mem_seek;
   cbs.close_func = NULL;
   cbs.tell_func  = ogg_mem_tell;

   if (ov_open_callbacks(&cd_src, &cd_vf, NULL, 0, cbs) < 0)
   {
      free(cd_file);
      cd_file = NULL;
      return;
   }
   cd_open = true;

   vi          = ov_info(&cd_vf, -1);
   cd_channels = (vi && vi->channels > 0) ? vi->channels : 2;
   rate        = (vi && vi->rate    > 0) ? vi->rate : audio_sample_rate;

   /* 16.16 resample step, clamped to keep the interpolation arithmetic sane */
   step    = ((uint64_t)rate << 16) / audio_sample_rate;
   cd_step = (step == 0) ? 1u :
             (step > 0x00400000u ? 0x00400000u : (uint32_t)step);

   cd_loop    = (looping != false);
   cd_frac    = 0;
   cd_primed  = false;
   cd_dec_n   = cd_dec_i = 0;
   cd_playing = true;
}

void CDAudio_Stop(void)
{
   cd_close();
}

void CDAudio_Update(void)
{
}

void CDAudio_Mix(int16_t *buffer, size_t num_frames, float volume)
{
   size_t n;
   int    vol;

   if (!cd_playing || !buffer || num_frames == 0)
      return;

   vol = (int)(volume * 256.0f);   /* 8.8 fixed master gain (one scalar) */
   if (vol <= 0)
      return;

   if (!cd_primed)
   {
      if (!cd_prime())
      {
         cd_close();
         return;
      }
   }

   for (n = 0; n < num_frames; n++)
   {
      int s[2];
      int l, r;

      cd_interp(s);
      l = s[0];
      r = s[1];

      cd_frac += cd_step;
      while (cd_frac >= 0x10000)
      {
         if (!cd_advance())
         {
            l = (l * vol) >> 8;
            r = (r * vol) >> 8;
            buffer[n * 2]     = (int16_t)S_SoftClip((int)buffer[n * 2]     + l);
            buffer[n * 2 + 1] = (int16_t)S_SoftClip((int)buffer[n * 2 + 1] + r);
            cd_close();
            return;
         }
         cd_frac -= 0x10000;
      }

      l = (l * vol) >> 8;
      r = (r * vol) >> 8;
      buffer[n * 2]     = (int16_t)S_SoftClip((int)buffer[n * 2]     + l);
      buffer[n * 2 + 1] = (int16_t)S_SoftClip((int)buffer[n * 2 + 1] + r);
   }
}

/* Float counterpart of CDAudio_Mix, used when float audio output has been
 * negotiated. Same cubic interpolation via the shared helpers; only the
 * accumulate/clip differs -- the int16 CD samples are normalized to [-1,1]
 * and summed into the float output buffer, then soft-clipped with the same
 * curve. */
void CDAudio_MixF(float *buffer, size_t num_frames, float volume)
{
   size_t n;
   int    vol;

   if (!cd_playing || !buffer || num_frames == 0)
      return;

   vol = (int)(volume * 256.0f);   /* 8.8 fixed master gain (one scalar) */
   if (vol <= 0)
      return;

   if (!cd_primed)
   {
      if (!cd_prime())
      {
         cd_close();
         return;
      }
   }

   for (n = 0; n < num_frames; n++)
   {
      int s[2];
      int l, r;

      cd_interp(s);
      l = s[0];
      r = s[1];

      cd_frac += cd_step;
      while (cd_frac >= 0x10000)
      {
         if (!cd_advance())
         {
            l = (l * vol) >> 8;
            r = (r * vol) >> 8;
            buffer[n * 2]     = S_SoftClipNormF(buffer[n * 2]     + l * (1.0f / 32768.0f));
            buffer[n * 2 + 1] = S_SoftClipNormF(buffer[n * 2 + 1] + r * (1.0f / 32768.0f));
            cd_close();
            return;
         }
         cd_frac -= 0x10000;
      }

      l = (l * vol) >> 8;
      r = (r * vol) >> 8;
      buffer[n * 2]     = S_SoftClipNormF(buffer[n * 2]     + l * (1.0f / 32768.0f));
      buffer[n * 2 + 1] = S_SoftClipNormF(buffer[n * 2 + 1] + r * (1.0f / 32768.0f));
   }
}

qboolean CDAudio_Playing(void)
{
   return (qboolean)cd_playing;
}
