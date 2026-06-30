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
// r_surf.c: surface-related refresh code

#include "r_local.h"

drawsurf_t	r_drawsurf;

int				lightleft, sourcesstep, blocksize, sourcetstep;
int				lightdelta, lightdeltastep;
int				lightright, lightleftstep, lightrightstep, blockdivshift;
unsigned		blockdivmask;
void			*prowdestbase;
unsigned short	*prowdest565base;
unsigned short	*cacheblock565;
unsigned char	*pbasesource;
int				surfrowbytes;	// used by ASM files
unsigned		*r_lightptr;
int				r_stepback;
int				r_lightwidth;
int				r_numhblocks, r_numvblocks;
unsigned char	*r_source, *r_sourcemax;

void R_DrawSurfaceBlock8_mip0 (void);
void R_DrawSurfaceBlock8_mip1 (void);
void R_DrawSurfaceBlock8_mip2 (void);
void R_DrawSurfaceBlock8_mip3 (void);

void R_DrawSurfaceBlockRGB_mip0 (void);
void R_DrawSurfaceBlockRGB_mip1 (void);
void R_DrawSurfaceBlockRGB_mip2 (void);
void R_DrawSurfaceBlockRGB_mip3 (void);

static void	(*surfmiptable[4])(void) = {
	R_DrawSurfaceBlock8_mip0,
	R_DrawSurfaceBlock8_mip1,
	R_DrawSurfaceBlock8_mip2,
	R_DrawSurfaceBlock8_mip3
};

static void	(*surfmiptableRGB[4])(void) = {
	R_DrawSurfaceBlockRGB_mip0,
	R_DrawSurfaceBlockRGB_mip1,
	R_DrawSurfaceBlockRGB_mip2,
	R_DrawSurfaceBlockRGB_mip3
};

void SWR_BuildLightMap (void);
extern	unsigned		blocklights[1024*3];	// RGB-wide

float           surfscale;
qboolean        r_cache_thrash;         // set if surface cache is thrashing

int         sc_size;
surfcache_t	*sc_rover, *sc_base;

/*
===============
SWR_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
static image_t *SWR_TextureAnimation (mtexinfo_t *tex)
{
	int		c;

	if (!tex->next)
		return tex->image;

	c = refsoft_currententity->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}


/*
===============
R_DrawSurface
===============
*/
void R_DrawSurface (void)
{
	unsigned char	*basetptr;
	int				smax, tmax, twidth;
	int				u;
	int				soffset, basetoffset, texwidth;
	int				horzblockstep;
	unsigned char	*pcolumndest;
	unsigned short	*pcolumndest565;
	void			(*pblockdrawer)(void);
	image_t			*mt;
	int				colored;

	surfrowbytes = r_drawsurf.rowbytes;

	mt = r_drawsurf.image;

	/* colored lighting is active for this surface when enabled, the surface
	 * carries a 24bit lightmap, the world has light data, and we are not in
	 * fullbright -- mirror this test where SWR_BuildLightMapRGB is selected. */
	colored = sw_colored_lighting_enabled && r_drawsurf.surf->samples_rgb
	          && r_refsoft_worldmodel->lightdata && !r_fullbright->value;
	
	r_source = mt->pixels[r_drawsurf.surfmip];
	
// the fractional light values should range from 0 to (VID_GRADES - 1) << 16
// from a source range of 0 - 255
	
	texwidth = mt->width >> r_drawsurf.surfmip;

	blocksize = 16 >> r_drawsurf.surfmip;
	blockdivshift = 4 - r_drawsurf.surfmip;
	blockdivmask = (1 << blockdivshift) - 1;
	
	r_lightwidth = (r_drawsurf.surf->extents[0]>>4)+1;

	r_numhblocks = r_drawsurf.surfwidth >> blockdivshift;
	r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

//==============================

	pblockdrawer = colored ? surfmiptableRGB[r_drawsurf.surfmip] : surfmiptable[r_drawsurf.surfmip];
// TODO: only needs to be set when there is a display settings change
	horzblockstep = blocksize;

	smax = mt->width >> r_drawsurf.surfmip;
	twidth = texwidth;
	tmax = mt->height >> r_drawsurf.surfmip;
	sourcetstep = texwidth;
	r_stepback = tmax * twidth;

	r_sourcemax = r_source + (tmax * smax);

	soffset = r_drawsurf.surf->texturemins[0];
	basetoffset = r_drawsurf.surf->texturemins[1];

// << 16 components are to guarantee positive values for %
	soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
	basetptr = &r_source[((((basetoffset >> r_drawsurf.surfmip) 
		+ (tmax << 16)) % tmax) * twidth)];

	pcolumndest = r_drawsurf.surfdat;
	pcolumndest565 = r_drawsurf.surfdat565;

	for (u=0 ; u<r_numhblocks; u++)
	{
		r_lightptr = blocklights + (colored ? u*3 : u);

		prowdestbase = pcolumndest;
		prowdest565base = pcolumndest565;	/* NULL unless colored */

		pbasesource = basetptr + soffset;

		(*pblockdrawer)();

		soffset = soffset + blocksize;
		if (soffset >= smax)
			soffset = 0;

		pcolumndest += horzblockstep;
		if (pcolumndest565)
			pcolumndest565 += horzblockstep;
	}
}


//=============================================================================

/*
================
R_DrawSurfaceBlock8_mip0
================
*/
void R_DrawSurfaceBlock8_mip0 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 4;
		lightrightstep = (r_lightptr[1] - lightright) >> 4;

		for (i=0 ; i<16 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 4;

			light = lightright;

			for (b=15; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 3;
		lightrightstep = (r_lightptr[1] - lightright) >> 3;

		for (i=0 ; i<8 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 3;

			light = lightright;

			for (b=7; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 2;
		lightrightstep = (r_lightptr[1] - lightright) >> 2;

		for (i=0 ; i<4 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 2;

			light = lightright;

			for (b=3; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 1;
		lightrightstep = (r_lightptr[1] - lightright) >> 1;

		for (i=0 ; i<2 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 1;

			light = lightright;

			for (b=1; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}

/*
================
R_DrawSurfaceBlockRGB

Colored counterpart of R_DrawSurfaceBlock8_mipN. blocklights holds three
interleaved channels (per-luxel RGB light multipliers in [256,65536]); the
texel's palette RGB (d_refsoft_8to24table) is multiplied by the bilinearly
interpolated light per channel, then snapped back to a palette index through
palmap2. shift selects the block size: mip0=4(16), mip1=3(8), mip2=2(4),
mip3=1(2).
================
*/
extern unsigned	d_refsoft_8to24table[256];

static void R_DrawSurfaceBlockRGB (int shift)
{
	int				v, i, b, bw;
	unsigned char	pix, *psource, *prowdest, *pix24;
	unsigned short	*prowdest565;
	int				lleft[3], lright[3], lleftstep[3], lrightstep[3];
	int				light[3], lstep[3];
	int				tr, tg, tb;

	bw = 1 << shift;
	psource = pbasesource;
	prowdest = prowdestbase;
	prowdest565 = prowdest565base;	/* NULL unless this surface is colored */

	for (v=0 ; v<r_numvblocks ; v++)
	{
		lleft[0] = r_lightptr[0]; lright[0] = r_lightptr[3];
		lleft[1] = r_lightptr[1]; lright[1] = r_lightptr[4];
		lleft[2] = r_lightptr[2]; lright[2] = r_lightptr[5];
		r_lightptr += r_lightwidth * 3;
		lleftstep[0]  = ((int)r_lightptr[0] - lleft[0])  >> shift;
		lrightstep[0] = ((int)r_lightptr[3] - lright[0]) >> shift;
		lleftstep[1]  = ((int)r_lightptr[1] - lleft[1])  >> shift;
		lrightstep[1] = ((int)r_lightptr[4] - lright[1]) >> shift;
		lleftstep[2]  = ((int)r_lightptr[2] - lleft[2])  >> shift;
		lrightstep[2] = ((int)r_lightptr[5] - lright[2]) >> shift;

		for (i=0 ; i<bw ; i++)
		{
			lstep[0] = (lleft[0] - lright[0]) >> shift; light[0] = lright[0];
			lstep[1] = (lleft[1] - lright[1]) >> shift; light[1] = lright[1];
			lstep[2] = (lleft[2] - lright[2]) >> shift; light[2] = lright[2];

			for (b=bw-1 ; b>=0 ; b--)
			{
				/* 8bit intermediate keeps two extra low bits over the 6bit
				 * palmap index so the same multiply feeds both the palette
				 * snap (>>2) and the RGB565 pack, and the 8bit cache stays
				 * byte-identical to the palette-only colored path. */
				pix = psource[b];
				pix24 = (unsigned char *)&d_refsoft_8to24table[pix];
				tr = (pix24[0] * light[0]) >> 15; if (tr < 0) tr = 0; else if (tr > 255) tr = 255;
				tg = (pix24[1] * light[1]) >> 15; if (tg < 0) tg = 0; else if (tg > 255) tg = 255;
				tb = (pix24[2] * light[2]) >> 15; if (tb < 0) tb = 0; else if (tb > 255) tb = 255;
				prowdest[b] = palmap2[tr >> 2][tg >> 2][tb >> 2];
				if (prowdest565)
					prowdest565[b] = (unsigned short)
						(((tb >> 3) & 0x1f) | (((tg >> 2) & 0x3f) << 5) | (((tr >> 3) & 0x1f) << 11));
				light[0] += lstep[0];
				light[1] += lstep[1];
				light[2] += lstep[2];
			}

			psource += sourcetstep;
			lright[0] += lrightstep[0]; lleft[0] += lleftstep[0];
			lright[1] += lrightstep[1]; lleft[1] += lleftstep[1];
			lright[2] += lrightstep[2]; lleft[2] += lleftstep[2];
			prowdest += surfrowbytes;
			if (prowdest565)
				prowdest565 += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}

void R_DrawSurfaceBlockRGB_mip0 (void) { R_DrawSurfaceBlockRGB (4); }
void R_DrawSurfaceBlockRGB_mip1 (void) { R_DrawSurfaceBlockRGB (3); }
void R_DrawSurfaceBlockRGB_mip2 (void) { R_DrawSurfaceBlockRGB (2); }
void R_DrawSurfaceBlockRGB_mip3 (void) { R_DrawSurfaceBlockRGB (1); }

//============================================================================


/*
================
R_InitCaches

================
*/
void R_InitCaches (void)
{
	int		size;
	int		pix;

	// calculate size to allocate
	if (sw_surfcacheoverride->value)
	{
		size = sw_surfcacheoverride->value;
	}
	else
	{
		size = SURFCACHE_SIZE_AT_320X240;

		pix = vid.width*vid.height;
		if (pix > 64000)
			size += (pix-64000)*6;	/* extra room for the parallel RGB565 colored-light cache */
	}		

	// round up to page size
	size = (size + 8191) & ~8191;

	ri.Con_Printf (PRINT_ALL,"%ik surface cache\n", size/1024);

	sc_size = size;
	sc_base = (surfcache_t *)malloc(size);
	sc_rover = sc_base;
	
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
}


/*
==================
D_FlushCaches
==================
*/
void D_FlushCaches (void)
{
	surfcache_t     *c;
	
	if (!sc_base)
		return;

	for (c = sc_base ; c ; c = c->next)
	{
		if (c->owner)
			*c->owner = NULL;
	}
	
	sc_rover = sc_base;
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
}

/*
=================
D_SCAlloc
=================
*/
surfcache_t     *D_SCAlloc (int width, int size)
{
	surfcache_t             *new;
	qboolean                wrapped_this_time;

	if ((width < 0) || (width > 256))
		ri.Sys_Error (ERR_FATAL,"D_SCAlloc: bad cache width %d\n", width);

	if ((size <= 0) || (size > 0x30000))
		ri.Sys_Error (ERR_FATAL,"D_SCAlloc: bad cache size %d\n", size);
	
	size = (int)(size_t)&((surfcache_t *)0)->data[size];
	size = (size + 3) & ~3;
	if (size > sc_size)
		ri.Sys_Error (ERR_FATAL,"D_SCAlloc: %i > cache size of %i",size, sc_size);

// if there is not size bytes after the rover, reset to the start
	wrapped_this_time = false;

	if ( !sc_rover || (byte *)sc_rover - (byte *)sc_base > sc_size - size)
	{
		if (sc_rover)
		{
			wrapped_this_time = true;
		}
		sc_rover = sc_base;
	}
		
// colect and free surfcache_t blocks until the rover block is large enough
	new = sc_rover;
	if (sc_rover->owner)
		*sc_rover->owner = NULL;
	
	while (new->size < size)
	{
	// free another
		sc_rover = sc_rover->next;
		if (!sc_rover)
			ri.Sys_Error (ERR_FATAL,"D_SCAlloc: hit the end of memory");
		if (sc_rover->owner)
			*sc_rover->owner = NULL;
			
		new->size += sc_rover->size;
		new->next = sc_rover->next;
	}

// create a fragment out of any leftovers
	if (new->size - size > 256)
	{
		sc_rover = (surfcache_t *)( (byte *)new + size);
		sc_rover->size = new->size - size;
		sc_rover->next = new->next;
		sc_rover->width = 0;
		sc_rover->owner = NULL;
		new->next = sc_rover;
		new->size = size;
	}
	else
		sc_rover = new->next;
	
	new->width = width;
// DEBUG
	if (width > 0)
		new->height = (size - sizeof(*new) + sizeof(new->data)) / width;

	new->owner = NULL;              // should be set properly after return

	if (d_roverwrapped)
	{
		if (wrapped_this_time || (sc_rover >= d_initial_rover))
			r_cache_thrash = true;
	}
	else if (wrapped_this_time)
	{       
		d_roverwrapped = true;
	}

	return new;
}


/*
=================
D_SCDump
=================
*/
void D_SCDump (void)
{
	surfcache_t             *test;

	for (test = sc_base ; test ; test = test->next)
	{
		if (test == sc_rover)
			ri.Con_Printf (PRINT_ALL,"ROVER:\n");
		ri.Con_Printf (PRINT_ALL,"%p : %i bytes     %i width\n",test, test->size, test->width);
	}
}

//=============================================================================

// if the num is not a power of 2, assume it will not repeat

int     MaskForNum (int num)
{
	if (num==128)
		return 127;
	if (num==64)
		return 63;
	if (num==32)
		return 31;
	if (num==16)
		return 15;
	return 255;
}

int D_log2 (int num)
{
	int     c;
	
	c = 0;
	
	while (num>>=1)
		c++;
	return c;
}

//=============================================================================

/*
================
D_CacheSurface
================
*/
surfcache_t *D_CacheSurface (msurface_t *surface, int miplevel)
{
	surfcache_t     *cache;

//
// if the surface is animating or flashing, flush the cache
//
	r_drawsurf.image       = SWR_TextureAnimation (surface->texinfo);
	r_drawsurf.lightadj[0] = r_refsoft_newrefdef.lightstyles[surface->styles[0]].white*128;
	r_drawsurf.lightadj[1] = r_refsoft_newrefdef.lightstyles[surface->styles[1]].white*128;
	r_drawsurf.lightadj[2] = r_refsoft_newrefdef.lightstyles[surface->styles[2]].white*128;
	r_drawsurf.lightadj[3] = r_refsoft_newrefdef.lightstyles[surface->styles[3]].white*128;
	
//
// see if the cache holds apropriate data
//
	cache = surface->cachespots[miplevel];

	if (cache && !cache->dlight && surface->dlightframe != r_framecount
			&& cache->image == r_drawsurf.image
			&& cache->lightadj[0] == r_drawsurf.lightadj[0]
			&& cache->lightadj[1] == r_drawsurf.lightadj[1]
			&& cache->lightadj[2] == r_drawsurf.lightadj[2]
			&& cache->lightadj[3] == r_drawsurf.lightadj[3] )
		return cache;

//
// determine shape of surface
//
	surfscale = 1.0 / (1<<miplevel);
	r_drawsurf.surfmip = miplevel;
	r_drawsurf.surfwidth = surface->extents[0] >> miplevel;
	r_drawsurf.rowbytes = r_drawsurf.surfwidth;
	r_drawsurf.surfheight = surface->extents[1] >> miplevel;
	
//
// allocate memory if needed
//
	{
		int surfcolored = sw_colored_lighting_enabled && surface->samples_rgb
		                  && r_refsoft_worldmodel->lightdata && !r_fullbright->value;
		int texels = r_drawsurf.surfwidth * r_drawsurf.surfheight;

		if (!cache)     // if a texture just animated, don't reallocate it
		{
			/* colored surfaces carry an 8bit palette-snapped block (warp/alpha
			 * fallback) immediately followed by a parallel RGB565 block. */
			cache = D_SCAlloc (r_drawsurf.surfwidth,
							   surfcolored ? texels * 3 : texels);
			surface->cachespots[miplevel] = cache;
			cache->owner = &surface->cachespots[miplevel];
			cache->mipscale = surfscale;
		}
		cache->colored = surfcolored;
		cache->texels  = texels;
	}
	
	if (surface->dlightframe == r_framecount)
		cache->dlight = 1;
	else
		cache->dlight = 0;

	r_drawsurf.surfdat = (pixel_t *)cache->data;
	r_drawsurf.surfdat565 = cache->colored
		? (unsigned short *)(cache->data
		                     + r_drawsurf.surfwidth * r_drawsurf.surfheight)
		: NULL;
	
	cache->image = r_drawsurf.image;
	cache->lightadj[0] = r_drawsurf.lightadj[0];
	cache->lightadj[1] = r_drawsurf.lightadj[1];
	cache->lightadj[2] = r_drawsurf.lightadj[2];
	cache->lightadj[3] = r_drawsurf.lightadj[3];

//
// draw and light the surface texture
//
	r_drawsurf.surf = surface;


	// calculate the lightings
	if (sw_colored_lighting_enabled && surface->samples_rgb
	    && r_refsoft_worldmodel->lightdata && !r_fullbright->value)
		SWR_BuildLightMapRGB ();
	else
		SWR_BuildLightMap ();
	
	// rasterize the surface into the cache
	R_DrawSurface ();

	return cache;
}


