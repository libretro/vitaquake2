#ifndef _OS_H
#define _OS_H
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.

 ********************************************************************/

#include <math.h>
#include <ogg/os_types.h>

#ifndef _V_IFDEFJAIL_H_
#  define _V_IFDEFJAIL_H_

#  ifdef __GNUC__
#    define STIN static __inline__
#  elif _WIN32
#    define STIN static __inline
#  endif
#else
#  define STIN static
#endif

#ifndef M_PI
#  define M_PI (3.1415926536f)
#endif

#ifdef _WIN32
#  include <malloc.h>
#  define rint(x)   (floor((x)+0.5f)) 
#  define NO_FLOAT_MATH_LIB
#  define FAST_HYPOT(a, b) sqrt((a)*(a) + (b)*(b))
#  define LITTLE_ENDIAN 1
#  define BYTE_ORDER LITTLE_ENDIAN
#endif

#if defined(HAVE_ALLOCA_H) || defined(__linux__)
#  include <alloca.h>
#endif

/* alloca: statically-linked console / web toolchains (devkitARM, devkitPPC
 * for GameCube/Wii/Wii U, devkitA64/libnx, Emscripten) ship no external
 * alloca() symbol, so a libc-style alloca() call compiles but fails to link
 * with "undefined reference to alloca". GCC and Clang always provide
 * __builtin_alloca - which is exactly what glibc's alloca() expands to - and
 * it lowers to an inline stack adjustment needing no symbol. Route alloca()
 * through the builtin on those compilers (these are stack scratch buffers, not
 * aligned heap, so memalign is not applicable). MSVC keeps _alloca via
 * <malloc.h>. */
#if defined(__GNUC__) || defined(__clang__)
#  undef alloca
#  define alloca __builtin_alloca
#endif

#ifdef USE_MEMORY_H
#  include <memory.h>
#endif

#ifndef min
#  define min(x,y)  ((x)>(y)?(y):(x))
#endif

#ifndef max
#  define max(x,y)  ((x)<(y)?(y):(x))
#endif

#endif /* _OS_H */
