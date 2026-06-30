/* swimp.c */
#include "../ref_soft/r_local.h"
#define RGB8_to_565(r,g,b)  (((b)>>3)&0x1f)|((((g)>>2)&0x3f)<<5)|((((r)>>3)&0x1f)<<11)

uint16_t d_8to16table[256];
uint32_t start_palette[256];
uint16_t palette_tbl[256];

extern int scr_width;
extern int scr_height;
extern void *tex_buffer;
extern void    *sw_present_target;
extern unsigned sw_present_pitch;
extern int      sw_present_active;

/* Truecolor (RGB565) sky overlay, allocated parallel to vid.buffer. */
unsigned short *sw_sky_overlay = NULL;
extern int      sw_truecolor_sky_enabled;

void VID_NewWindow (int width, int height);

void SWimp_BeginFrame( float camera_separation )
{
}

void SWimp_EndFrame (void)
{
	const pixel_t *src = vid.buffer;
	unsigned short *ovl = sw_sky_overlay;

	if (sw_present_target)
	{
		/* Convert straight into the frontend framebuffer. Its pitch is in
		 * bytes and rows may be padded, so advance per row. */
		int x, y;
		for (y = 0; y < scr_height; y++)
		{
			uint16_t      *dst = (uint16_t *)((uint8_t *)sw_present_target
			                                  + (size_t)y * sw_present_pitch);
			const pixel_t *s   = src + (size_t)y * scr_width;
			unsigned short *o  = ovl ? ovl + (size_t)y * scr_width : NULL;
			for (x = 0; x < scr_width; x++)
			{
				/* Sky pixels carry the sentinel index in vid.buffer and a
				 * non-zero 565 sample in the overlay; anything drawn over the
				 * sky (geometry, models, HUD) overwrites the sentinel so it
				 * falls through to the paletted path. The overlay slot is
				 * cleared as it is read (fused per-frame reset). */
				if (o)
				{
					unsigned short v = o[x];
					o[x] = 0;
					if (s[x] == SKY_SENTINEL_INDEX && v != 0)
					{
						dst[x] = v;
						continue;
					}
				}
				dst[x] = palette_tbl[s[x]];
			}
		}
		sw_present_active = 1;
	}
	else
	{
		/* No frontend buffer this frame: fill tex_buffer linearly. */
		uint16_t *dst = (uint16_t*)tex_buffer;
		int i, n = scr_width * scr_height;
		for (i = 0; i < n; i++)
		{
			if (ovl)
			{
				unsigned short v = ovl[i];
				ovl[i] = 0;
				if (src[i] == SKY_SENTINEL_INDEX && v != 0)
				{
					dst[i] = v;
					continue;
				}
			}
			dst[i] = palette_tbl[src[i]];
		}
	}
}

int			SWimp_Init( void *hInstance, void *wndProc )
{
	return 0;
}

void		SWimp_SetPalette( const unsigned char *palette)
{
	
	if(palette==NULL)
		return;
	
	// SetPalette seems to be called before SetMode, so we save the palette on a temp location
	if (tex_buffer == NULL){
		memcpy(start_palette, palette, sizeof(uint32_t)*256);
		return;
	}
	
	int i;

	uint8_t* pal = (uint8_t*)palette;
	unsigned char r, g, b;

	for(i = 0; i < 256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		palette_tbl[i] = RGB8_to_565(r, g, b);
		pal += 4;
	}
}

void		SWimp_Shutdown( void )
{
	if (vid.buffer)
	{
		free(vid.buffer);
		vid.buffer = NULL;
	}
	if (tex_buffer)
	{
		free(tex_buffer);
		tex_buffer = NULL;
	}
	if (sw_sky_overlay)
	{
		free(sw_sky_overlay);
		sw_sky_overlay = NULL;
	}
}

rserr_t		SWimp_SetMode( int *pwidth, int *pheight, int mode )
{
	if (vid.buffer != NULL) SWimp_Shutdown();
	
	vid.height = scr_height;
	vid.width = scr_width;
	vid.rowbytes = scr_width;
	vid.buffer = malloc(scr_width*scr_height);
	
	tex_buffer = calloc((size_t)scr_width*scr_height, sizeof(uint16_t));
	sw_sky_overlay = (unsigned short*)calloc((size_t)scr_width*scr_height, sizeof(unsigned short));
	
	SWimp_SetPalette((const unsigned char*)start_palette);
	
	*pwidth = scr_width;
	*pheight = scr_height;
	VID_NewWindow(scr_width,scr_height);
	
	return rserr_ok;
}

void		SWimp_AppActivate( qboolean active )
{
}
