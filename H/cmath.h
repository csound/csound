/*
    cmath.h:

    Copyright (C) 1994 Paris Smaragdis, John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

double besseli(double);

/* returns 0 on success, -1 if there are insufficient arguments, */
/* and -2 in the case of an unknown distribution */
int32_t gen21_rand(FGDATA *ff, FUNC *ftp);

typedef struct  {
        OPDS    h;
        MYFLT   *sr, *in, *powerOf, *norm;
 } POW;

typedef struct  {
        OPDS    h;
        MYFLT   *out, *arg1, *arg2, *arg3;
} PRAND;

typedef struct  {
        OPDS    h;
        MYFLT   *ar, *arg1, *xamp, *xcps;
        MYFLT   *iseed;
        MYFLT   dfdmax, num1, num2;
        int32_t   phs;
        int32_t     ampcod, cpscod;
} PRANDI;

typedef struct {
        OPDS   h;
        MYFLT  *ans;
} GETSEED;

typedef struct gauss{
  OPDS h;
  MYFLT *a, *mu, *sigma;
  MYFLT z;
  int flag;
} GAUSS;
