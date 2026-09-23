/*
    fhtfun.h:

    Copyright (C) 1997 Paris Smaragdis, John ffitch

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#pragma once

typedef struct{
    OPDS          h;
    cs_float         *out, *as, *af, *len, *ovlp, *iwin, *bias;
    AUXCH         mem;
    cs_float         *buffer_in1, *buffer_in2, *buffer_out;
    FUNC          *win;
    cs_float         *in1, *in2, *w;
    int32         size, overlap, hop, count;
} CON;

/* typedef struct{ */
/*   OPDS               h; */
/*   cs_float              *out, *as, *imp, *iwin, *ienv; */
/*   AUXCH              mem; */
/*   cs_float              *buffer_in, *buffer_out; */
/*   FUNC               *win; */
/*   cs_float              *in, *in2, norm; */
/*   int32               flen, count, count2; */
/* } CNV; */

/* typedef struct{ */
/*   OPDS               h; */
/*   cs_float              *out, *as, *str, *len, *ovlp, *iwin; */
/*   AUXCH              mem; */
/*   cs_float              *buffer_in, *buffer_out; */
/*   FUNC               *win; */
/*   cs_float              *in; */
/*   int32               m, count; */
/* } STCH; */

/* typedef struct{ */
/*   OPDS               h; */
/*   cs_float              *out, *as, *af, *len, *ovlp, *iwin, *bias, *peaks; */
/*   AUXCH              mem; */
/*   cs_float              *buffer_in, *buffer_in2, *buffer_out; */
/*   FUNC               *win; */
/*   cs_float              *in, *in2, *mor; */
/*   int32               *clx, *cly; */
/*   int32               m, count; */
/* } MRH; */

static void getmag(cs_float *x, int32 size);
static void mult(cs_float *x, cs_float *y, int32 size, cs_float w);
static void lineaprox(cs_float *x, int32 size, int32 m);
static void do_fht(cs_float *real, int32 n);
static void do_ifht(cs_float *real, int32 n);
static void pfht(cs_float *fz, int32 n);

