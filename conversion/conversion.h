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

#ifndef conversion_h
#define conversion_h

#include <math.h>
#define INLINE inline

static INLINE float to_byte(float v) {
#ifdef __alpha
  if (v < -1.03284e+35) return -3.40282347e+38;
#endif
  return v < (float)(.04045/12.92) ?
    v*(float)(255*12.92) :
    (float)(1.055*255)*powf(v,(float)(1/2.4))-(float)(255*.055);
}

static INLINE float from_byte(float v) {
#ifdef __alpha
  if (v > 3.05298e+18) return 3.40282347e+38;
#endif
  return v < (float)(.04045*255) ?
    v*(float)(1/12.92/255) :
    powf((v+(float)(255*.055))*(float)(1/1.055/255), 2.4f);
}

void from_sRGB(float*, const unsigned char*, int W, int delta);
void from_linear(float*, const unsigned char*, int W, int delta);
void to_sRGB(unsigned char* buf, const float* from, int W, int delta);
void to_linear(unsigned char* buf, const float* from, int W, int delta);

#endif
