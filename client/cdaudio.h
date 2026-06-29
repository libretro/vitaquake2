/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __CDAUDIO_H
#define __CDAUDIO_H

#include <stdint.h>

#define AUDIO_SAMPLE_RATE 44100
/* Audio buffer must be sufficient for operation
 * at 30 fps
 * > (2 * 44100) / 30 = 2490 total samples
 * > buffer size must be a power of 2
 * > Nearest power of 2 to 2490 is 4096 */
/* Sized so a single frame's worth of stereo audio fits even at the highest
 * selectable output rate (96 kHz) and the lowest framerate (30): 96000/30 =
 * 3200 stereo pairs = 6400 samples. Must stay a power of two: it doubles as
 * the ring length and (AUDIO_BUFFER_SIZE >> 1) is used as a pairs ring mask. */
#define AUDIO_BUFFER_SIZE  8192

int  CDAudio_Init(void);
void CDAudio_Shutdown(void);
void CDAudio_Play(int track, qboolean looping);
void CDAudio_Stop(void);
void CDAudio_Update(void);

void CDAudio_Mix(int16_t *buffer, size_t num_frames, float volume);
void CDAudio_MixF(float *buffer, size_t num_frames, float volume);
qboolean CDAudio_Playing(void);

#endif
