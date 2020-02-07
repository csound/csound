/**
 * IMAGE OPCODES
 *
 * imageOpcodes.h
 *
 * Copyright (c) 2007 by Cesare Marilungo. All rights reserved.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

typedef struct
{
  unsigned char *imageData;
  int32_t
  w,h;
} Image;

typedef struct
{
  Image **images;
  size_t cnt;
} Images;

typedef struct
{
  OPDS h;
  MYFLT *kn;
  STRINGDAT *ifilnam ;
} IMGLOAD;

typedef struct
{
  OPDS h;
  MYFLT *kn, *kw, *kh ;
} IMGCREATE;

typedef struct
{
  OPDS h;
  MYFLT *kw, *kh, *kn;
} IMGSIZE;

typedef struct
{
  OPDS h;
  MYFLT *kr,*kg,*kb, *kn, *kx,*ky;
} IMGGETPIXEL;

typedef struct
{
  OPDS h;
  MYFLT *kn,*kx,*ky,*kr,*kg,*kb;
} IMGSETPIXEL;
typedef struct
{
  OPDS h;
  MYFLT *kn;
  STRINGDAT *ifilnam;
} IMGSAVE;

typedef struct
{
  OPDS h;
  MYFLT *kn;
} IMGFREE;
