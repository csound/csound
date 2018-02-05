/*
    lowpassr.h:

    Copyright (C) 1998 Gabriel Maldonado

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

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *kfco, *kres, *istor;
        double  ynm1, ynm2;
        double  coef1, coef2, okf, okr, k;
} LOWPR;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *kfco, *kres, *ord, *istor;
        MYFLT   ynm1[10], ynm2[10] ;
        int32_t     loop;
        MYFLT   coef1, coef2, okf, okr, k;
} LOWPRX;

typedef struct {
        OPDS    h;
        MYFLT   *ar, *asig, *kfco, *kres, *ord, *sep;
        MYFLT   ynm1[10], ynm2[10], cut[10];
        int32_t
        loop;
} LOWPR_SEP;
