/*
    windin.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csoundCore.h"         /*                           WINDIN.C   */
#include "cwindow.h"
#include "windin.h"             /* real-time input control units        */
                                /* 26aug90 dpwe                         */

int xyinset(CSOUND *csound, XYIN *p)
{
    // This is not the way to do it; set _QQ in interlocks
    IGN(p);
    return csound->InitError(csound,
                             Str("xyin opcode has been deprecated in Csound6."));
}

/* static int deinit_func(CSOUND *csound, void *p) */
/* { */
/*     csound->csoundKillXYinCallback_(csound, &(((XYIN*) p)->w)); */
/*     return OK; */
/* } */

/* int xyinset(CSOUND *csound, XYIN *p) */
/* { */
/*     MYFLT   x, y; */
/*     MYFLT   iymax  = *p->iymax; */
/*     MYFLT   iymin  = *p->iymin; */
/*     MYFLT   ixmax  = *p->ixmax; */
/*     MYFLT   ixmin  = *p->ixmin; */
/*     MYFLT   iyinit = *p->iyinit; */
/*     MYFLT   ixinit = *p->ixinit; */

/*     if (UNLIKELY((p->timcount = (int)(CS_EKR * *p->iprd + FL(0.5)))<=0)) { */
/*       return csound->InitError(csound, Str("illegal iprd")); */
/*     } */
/*     if (UNLIKELY(iymin > iymax)) {        /\* swap if wrong order *\/ */
/*       y = iymin; iymin = iymax; iymax = y; */
/*     } */
/*     if (UNLIKELY(iyinit < iymin)) */
/*       iyinit = iymin; */
/*     else if (UNLIKELY(iyinit > iymax)) */
/*       iyinit = iymax; */
/*     *(p->kyrslt) = iyinit; */
/*     y = (*p->iymax != *p->iymin ? (*p->iymax-iyinit)/ (*p->iymax - *p->iymin) */
/*                                   : FL(0.5)); */
/*     p->w.y = y; */

/*     if (UNLIKELY(ixmin > ixmax)) {        /\* swap if wrong order *\/ */
/*       x = ixmin; ixmin = ixmax; ixmax = x; */
/*     } */
/*     if (UNLIKELY(ixinit < ixmin)) */
/*       ixinit = ixmin; */
/*     else if (UNLIKELY(ixinit > ixmax)) */
/*       ixinit = ixmax; */
/*     *(p->kxrslt) = ixinit; */
/*     x = (*p->ixmax != *p->ixmin ? (ixinit-*p->ixmin) / (*p->ixmax - *p->ixmin) */
/*                                   : FL(0.5)); */
/*     p->w.x = x; */

/*     csound->csoundMakeXYinCallback_(csound, &p->w, x, y); */
/*     csound->RegisterDeinitCallback(csound, (void*) p, deinit_func); */

/*     p->countdown = 1;           /\* init counter to run xyin on first call *\/ */
/*     return OK; */
/* } */

/* int xyin(CSOUND *csound, XYIN *p) */
/* { */
/*     if (UNLIKELY(!(--p->countdown))) {            /\* at each countdown   *\/ */
/*       p->countdown = p->timcount;                 /\*   reset counter &   *\/ */
/*       csound->csoundReadXYinCallback_(csound, &p->w); \*   read cursor postn  */
/*       *(p->kxrslt) = *p->ixmin + p->w.x * (*p->ixmax - *p->ixmin); */
/*       *(p->kyrslt) = *p->iymin + (FL(1.0) - p->w.y) * (*p->iymax - *p->iymin); */
/*     } */
/*     return OK; */
/* } */

