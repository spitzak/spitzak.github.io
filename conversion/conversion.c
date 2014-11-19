/*
 *
 * High-speed conversion between 8 bit and floating point image data.
 *
 * Copyright 2002 Bill Spitzak and Digital Domain, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * For use in closed-source software please contact Digital Domain,
 * 300 Rose Avenue, Venice, CA 90291 310-314-2800
 *
 */

#include "conversion.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef FLT_MAX
# define FLT_MAX 3.40282347e+38F
#endif

#if defined(__sgi) || defined(__PPC)
# define BIGENDIAN 1
#else
# define BIGENDIAN 0
#endif

static float index_to_float(const unsigned short i)
{
  float f[1];
  unsigned short *s = (unsigned short *)f;
  /* positive and negative zeros, and all gradual underflow, turn into zero: */
  if (i<0x80 || (i >= 0x8000 && i < 0x8080)) return 0;
  /* All NaN's and infinity turn into the largest possible legal float: */
  if (i>=0x7f80 && i<0x8000) return FLT_MAX;
  if (i>=0xff80) return -FLT_MAX;
#if BIGENDIAN
  s[0] = i;
  s[1] = 0x8000;
#else
  s[0] = 0x8000;
  s[1] = i;
#endif
  return f[0];
}

/* Turn pointer to float into pieces of a floating point number: */
#if BIGENDIAN
#define HIPART(f) (*(unsigned short*)f)
#define LOWPART(f) (*((unsigned short*)f + 1))
#else
#define HIPART(f) (*((unsigned short*)f + 1))
#define LOWPART(f) (*(unsigned short*)f)
#endif

static float from_byte_table[256];

static unsigned short to_byte_table[0x10000];

static void initialize(void)
{
  int i,b;
  static int initialized;
  if (initialized) return;
  initialized = 1;

  /* Fill in the lookup table to convert floats to bytes: */
  for (i = 0; i < 0x10000; i++) {
    float f = to_byte(index_to_float(i));
    if (f <= 0) to_byte_table[i] = 0;
    else if (f < 255) to_byte_table[i] = (unsigned short)(f*0x100+.5);
    else to_byte_table[i] = 0xff00;
  }

  /* Fill in the lookup table to convert bytes to float: */
  for (b = 0; b <= 255; b++) {
    float f[1]; f[0] = from_byte((float)b);
    from_byte_table[b] = f[0];
    i = HIPART(f);
#ifdef DEBUG
    {unsigned short g = to_byte_table[i];
    /* test to see if our calculations are ok: */
    if ((g+0x80)>>8 != b)
      printf("Entry for %d (%g) is %g and does not round to %d\n",
	     b, f[0], g/256.0, b);
    /* test to see if replacing the entry will not mis-order the table: */
    if ((b > 0   && to_byte_table[i-1] >= b*0x100) ||
	(b < 255 && to_byte_table[i+1] <= b*0x100)) {
      printf("Entry around %d (%g) are %g and %g and not in order\n",
	     b,f[0], to_byte_table[i-1]/256.0, to_byte_table[i+1]/256.0);
    }}
#endif
    /* replace entries so byte->float->byte does not change the data: */
    to_byte_table[i] = b*0x100;
  }
}

/****************************************************************/

/*! Converts sRGB file data to linear floating point. The input data is
    \a W bytes in the range 0-255, \a delta is the distance between each
    byte, allowing you to convert interlaced data.

    The conversion is done from right to left so the input and output
    arrays may be the same memory if \a delta is 4 or less.
*/
void from_sRGB(float* buf, const unsigned char* from, int W, int delta)
{
  initialize();
  from += (W-1)*delta;
  buf += W;
  for (; --W >= 0; from -= delta) *--buf = from_byte_table[*from];
}

/*! Converts linear file data to linear floating point. The input data is
    \a W bytes in the range 0-255, \a delta is the distance between each
    byte, allowing you to convert interlaced data. This should be
    used to convert mattes and alpha channels.

    The conversion is done from right to left so the input and output
    arrays may be the same memory if \a delta is 4 or less.
*/
void from_linear(float* buf, const unsigned char* from, int W, int delta)
{
  from += (W-1)*delta;
  buf += W;
  for (; --W >= 0; from -= delta) *--buf = *from*(1.0f/255f);
}

/*! Convert an array of linear floating point pixel values to an
    array of sRGB bytes, with error diffusion to avoid posterizing
    artifacts.

    \a W is the number of pixels to convert.  \a delta is the distance
    between the output bytes (useful for interlacing them into a buffer
    for screen display).

    The input and output buffers must not overlap in memory.
*/
void to_sRGB(unsigned char* buf, const float* from, int W, int delta)
{
  unsigned char* end = buf+W*delta;
  int start = rand()%W;
  const float* q;
  unsigned char* p;
  unsigned error;
  initialize();
  /* go fowards from starting point to end of line: */
  error = 0x80;
  for (p = buf+start*delta, q = from+start; p < end; p += delta) {
    error = (error&0xff) + to_byte_table[HIPART(q)]; ++q;
    *p = (error>>8);
  }
  /* go backwards from starting point to start of line: */
  error = 0x80;
  for (p = buf+(start-1)*delta, q = from+start; p >= buf; p -= delta) {
    --q; error = (error&0xff) + to_byte_table[HIPART(q)];
    *p = (error>>8);
  }
}

/*! Convert an array of floating point pixel values to an array of
    bytes by multiplying by 255 and doing error diffusion to avoid
    banding. This should be used to convert mattes and alpha channels.

    \a W is the number of pixels to convert.  \a delta is the distance
    between the output bytes (useful for interlacing them into a buffer
    for screen display).

    The input and output buffers must not overlap in memory.
*/
void to_linear(unsigned char* buf, const float* from, int W, int delta)
{
  unsigned char* end = buf+W*delta;
  int start = rand()%W;
  const float* q;
  unsigned char* p;
  /* go fowards from starting point to end of line: */
  float error = .5;
  for (p = buf+start*delta, q = from+start; p < end; p += delta) {
    float G = error + *q++ * 255.0f;
    if (G <= 0) {
      *p = 0;
    } else if (G < 255) {
      int i = (int)G;
      *p = i;
      error = G-i;
    } else {
      *p = 255;
    }
  }
  /* go backwards from starting point to start of line: */
  error = .5;
  for (p = buf+(start-1)*delta, q = from+start; p >= buf; p -= delta) {
    float G = error + *--q * 255.0f;
    if (G <= 0) {
      *p = 0;
    } else if (G < 255) {
      int i = (int)G;
      *p = i;
      error = G-i;
    } else {
      *p = 255;
    }
  }
}

/* end of conversion.c */
