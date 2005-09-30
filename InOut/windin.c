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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csoundCore.h"         /*                           WINDIN.C   */
#include "cwindow.h"
#include "windin.h"             /* real-time input control units        */
                                /* 26aug90 dpwe                         */

static int deinit_func(CSOUND *csound, void *p)
{
    csound->csoundKillXYinCallback_(csound, &(((XYIN*) p)->w));
    return OK;
}

int xyinset(CSOUND *csound, XYIN *p)
{
    MYFLT       f;
    MYFLT       iymax = *p->iymax;
    MYFLT       iymin = *p->iymin;
    MYFLT       ixmax = *p->ixmax;
    MYFLT       ixmin = *p->ixmin;
    MYFLT       iyinit = *p->iyinit;
    MYFLT       ixinit = *p->ixinit;

    if ((p->timcount = (int) (csound->ekr * *p->iprd)) <= 0) {
      return csound->InitError(csound, Str("illegal iprd"));
    }
    if (iymin > iymax) {                /* swap if wrong order */
      f = iymin; iymin = iymax; iymax = f;
    }
    if (iymin == iymax) { /* force some room (why?) */
      iymax = iymin + FL(1.0);          /* say.. */
      iymin -= FL(1.0);
    }
    if (iyinit < iymin)         iyinit = iymin;
    else if (iyinit > iymax)    iyinit = iymax;

    if (ixmin > ixmax) {        /* swap if wrong order */
      f = ixmin; ixmin = ixmax; ixmax = f;
    }
    if (ixmin == ixmax) {       /* force some room (why?) */
      ixmax = ixmin + FL(1.0);          /* say.. */
      ixmin -= FL(1.0);
    }
    if (ixinit < ixmin)
      ixinit = ixmin;
    else if (ixinit > ixmax)
      ixinit = ixmax;

    csound->csoundMakeXYinCallback_(csound, &p->w,
                                    (ixinit - ixmin) / (ixmax - ixmin),
                                    (iyinit - iymin) / (iymax - iymin));
    csound->RegisterDeinitCallback(csound, (void*) p, deinit_func);

    p->countdown = 1;           /* init counter to run xyin on first call */
    return OK;
}

int xyin(CSOUND *csound, XYIN *p)
{
    if (!(--p->countdown)) {                          /* at each countdown   */
      p->countdown = p->timcount;                     /*   reset counter &   */
      csound->csoundReadXYinCallback_(csound, &p->w); /*   read cursor postn */
      *(p->kxrslt) = *p->ixmin + p->w.x * (*p->ixmax - *p->ixmin);
      *(p->kyrslt) = *p->iymin + (FL(1.0) - p->w.y) * (*p->iymax - *p->iymin);
    }
    return OK;
}

