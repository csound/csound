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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

typedef struct{
    OPDS          h;
    MYFLT         *out, *as, *af, *len, *ovlp, *iwin, *bias;
    AUXCH         mem;
    MYFLT         *buffer_in1, *buffer_in2, *buffer_out;
    FUNC          *win;
    MYFLT         *in1, *in2, *w;
    long          m, count;
    MYFLT         s_ovlp;
} CON;

/* typedef struct{ */
/*   OPDS               h; */
/*   MYFLT              *out, *as, *imp, *iwin, *ienv; */
/*   AUXCH              mem; */
/*   MYFLT              *buffer_in, *buffer_out; */
/*   FUNC               *win; */
/*   MYFLT              *in, *in2, norm; */
/*   long               flen, count, count2; */
/* } CNV; */

/* typedef struct{ */
/*   OPDS               h; */
/*   MYFLT              *out, *as, *str, *len, *ovlp, *iwin; */
/*   AUXCH              mem; */
/*   MYFLT              *buffer_in, *buffer_out; */
/*   FUNC               *win; */
/*   MYFLT              *in; */
/*   long               m, count; */
/* } STCH; */

/* typedef struct{ */
/*   OPDS               h; */
/*   MYFLT              *out, *as, *af, *len, *ovlp, *iwin, *bias, *peaks; */
/*   AUXCH              mem; */
/*   MYFLT              *buffer_in, *buffer_in2, *buffer_out; */
/*   FUNC               *win; */
/*   MYFLT              *in, *in2, *mor; */
/*   long               *clx, *cly; */
/*   long               m, count; */
/* } MRH; */

void getmag(MYFLT *x, long size);
void makepolar(MYFLT *x, long size);
void makerect(MYFLT *x, long size);
void makelog(MYFLT *x, long size);
void unlog(MYFLT *x, long size);
void scalemod(MYFLT *y, long size);
void scalemag(MYFLT *x, long size);
void mult(MYFLT *x, MYFLT *y, long size, MYFLT w);
void lineaprox(MYFLT *x, long size, long m);
void do_fht(MYFLT *real, long n);
void do_ifht(MYFLT *real, long n);
void pfht(MYFLT *fz, long n);
