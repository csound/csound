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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#pragma once

typedef struct{
    OPDS          h;
    MYFLT         *out, *as, *af, *len, *ovlp, *iwin, *bias;
    AUXCH         mem;
    MYFLT         *buffer_in1, *buffer_in2, *buffer_out;
    FUNC          *win;
    MYFLT         *in1, *in2, *w;
    int32         m, count;
    MYFLT         s_ovlp;
} CON;

/* typedef struct{ */
/*   OPDS               h; */
/*   MYFLT              *out, *as, *imp, *iwin, *ienv; */
/*   AUXCH              mem; */
/*   MYFLT              *buffer_in, *buffer_out; */
/*   FUNC               *win; */
/*   MYFLT              *in, *in2, norm; */
/*   int32               flen, count, count2; */
/* } CNV; */

/* typedef struct{ */
/*   OPDS               h; */
/*   MYFLT              *out, *as, *str, *len, *ovlp, *iwin; */
/*   AUXCH              mem; */
/*   MYFLT              *buffer_in, *buffer_out; */
/*   FUNC               *win; */
/*   MYFLT              *in; */
/*   int32               m, count; */
/* } STCH; */

/* typedef struct{ */
/*   OPDS               h; */
/*   MYFLT              *out, *as, *af, *len, *ovlp, *iwin, *bias, *peaks; */
/*   AUXCH              mem; */
/*   MYFLT              *buffer_in, *buffer_in2, *buffer_out; */
/*   FUNC               *win; */
/*   MYFLT              *in, *in2, *mor; */
/*   int32               *clx, *cly; */
/*   int32               m, count; */
/* } MRH; */

static void getmag(MYFLT *x, int32 size);
static void mult(MYFLT *x, MYFLT *y, int32 size, MYFLT w);
static void lineaprox(MYFLT *x, int32 size, int32 m);
static void do_fht(MYFLT *real, int32 n);
static void do_ifht(MYFLT *real, int32 n);
static void pfht(MYFLT *fz, int32 n);

