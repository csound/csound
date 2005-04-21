/*
    butter.c:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*              Butterworth filters coded by Paris Smaragdis 1994       */
/*              Berklee College of Music Csound development team        */
/*              Copyright (c) May 1994.  All rights reserved            */

#include "csdl.h"
#include "butter.h"
#include <math.h>
#define ROOT2 (FL(1.4142135623730950488))

static void butter_filter(long, MYFLT *, MYFLT *, MYFLT *);

int butset(ENVIRON *csound, BFIL *p)    /*      Hi/Lo pass set-up       */
{
    if (*p->istor==FL(0.0)) {
      p->a[6] = p->a[7] = FL(0.0);
      p->lkf = FL(0.0);
    }
    return OK;
}

int bbutset(ENVIRON *csound, BBFIL *p)  /*      Band set-up     */
{
    if (*p->istor==FL(0.0)) {
      p->a[6] = p->a[7] = FL(0.0);
      p->lkb = FL(0.0);
      p->lkf = FL(0.0);
    }
    return OK;
}

int hibut(ENVIRON *csound, BFIL *p)     /*      Hipass filter   */
{
    MYFLT       *out, *in;

    in = p->ain;
    out = p->sr;

    if (*p->kfc <= FL(0.0))     {
      long      n = csound->ksmps;
      do {
        *out++ = *in++;
      } while (--n);
      return OK;
    }

    if (*p->kfc != p->lkf)      {
      MYFLT     *a, c;

      a = p->a;
      p->lkf = *p->kfc;
      c = (MYFLT)tan((double)(csound->pidsr * p->lkf));

      a[1] = FL(1.0) / ( FL(1.0) + ROOT2 * c + c * c);
      a[2] = -(a[1] + a[1]);
      a[3] = a[1];
      a[4] = FL(2.0) * ( c*c - FL(1.0)) * a[1];
      a[5] = ( FL(1.0) - ROOT2 * c + c * c) * a[1];
    }
    butter_filter(csound->ksmps, in, out, p->a);
    return OK;
}

int lobut(ENVIRON *csound, BFIL *p)     /*      Lopass filter   */
{
    MYFLT       *out, *in;

    in = p->ain;
    out = p->sr;

    if (*p->kfc <= FL(0.0))     {
      long      n = csound->ksmps;

      do {
        *out++ = FL(0.0);
        } while (--n);

      return OK;
    }

    if (*p->kfc != p->lkf)      {
      MYFLT     *a, c;

      a = p->a;
      p->lkf = *p->kfc;
      c = FL(1.0) / (MYFLT)tan((double)(csound->pidsr * p->lkf));
      a[1] = FL(1.0) / ( FL(1.0) + ROOT2 * c + c * c);
      a[2] = a[1] + a[1];
      a[3] = a[1];
      a[4] = FL(2.0) * ( FL(1.0) - c*c) * a[1];
      a[5] = ( FL(1.0) - ROOT2 * c + c * c) * a[1];
    }

    butter_filter(csound->ksmps, in, out, p->a);
    return OK;
}

int bpbut(ENVIRON *csound, BBFIL *p)    /*      Bandpass filter */
{
    MYFLT       *out, *in;

    in = p->ain;
    out = p->sr;
    if (*p->kbw <= FL(0.0))     {
      long      n = csound->ksmps;
      do {
        *out++ = FL(0.0);
      } while (--n);
      return OK;
    }
    if (*p->kbw != p->lkb || *p->kfo != p->lkf) {
      MYFLT *a, c, d;
      a = p->a;
      p->lkf = *p->kfo;
      p->lkb = *p->kbw;
      c = FL(1.0) / (MYFLT)tan((double)(csound->pidsr * p->lkb));
      d = FL(2.0) * (MYFLT)cos((double)(csound->tpidsr * p->lkf));
      a[1] = FL(1.0) / ( FL(1.0) + c);
      a[2] = FL(0.0);
      a[3] = -a[1];
      a[4] = - c * d * a[1];
      a[5] = ( c - FL(1.0)) * a[1];
    }
    butter_filter(csound->ksmps, in, out, p->a);
    return OK;
}

int bcbut(ENVIRON *csound, BBFIL *p)    /*      Band reject filter      */
{
    MYFLT       *out, *in;

    in = p->ain;
    out = p->sr;

    if (*p->kbw <= FL(0.0))     {
      long      n = csound->ksmps;

      do {
        *out++ = *in++;
      } while (--n);
      return OK;
    }

    if (*p->kbw != p->lkb || *p->kfo != p->lkf) {
      MYFLT *a, c, d;

      a = p->a;
      p->lkf = *p->kfo;
      p->lkb = *p->kbw;
      c = (MYFLT)tan((double)(csound->pidsr * p->lkb));
      d = FL(2.0) * (MYFLT)cos((double)(csound->tpidsr * p->lkf));
      a[1] = FL(1.0) / ( FL(1.0) + c);
      a[2] = - d * a[1];
      a[3] = a[1];
      a[4] = a[2];
      a[5] = ( FL(1.0) - c) * a[1];
    }

    butter_filter(csound->ksmps, in, out, p->a);
    return OK;
}

#if defined(WIN32) && !defined(USE_DOUBLE)
#define IS_DENORMAL(f) (((*(unsigned int*)&f)&0x7f800000)==0)
#else
#define IS_DENORMAL(f) (0)
#endif

static void butter_filter(long n, MYFLT *in, MYFLT *out, MYFLT *a)  /*      Filter loop */
{
    MYFLT t, y;

    do {
      t = *in++ - a[4] * a[6] - a[5] * a[7];
      if (sizeof(MYFLT)==sizeof(float) && IS_DENORMAL(t)) t = 0.0f;
      y = t * a[1] + a[2] * a[6] + a[3] * a[7];
      a[7] = a[6];
      a[6] = t;
      *out++ = y;
    } while (--n);
}

#define S       sizeof

static OENTRY localops[] = {
{ "butterhp", S(BFIL),  5,  "a",    "ako",  (SUBR)butset,  NULL, (SUBR)hibut  },
{ "butterlp", S(BFIL),  5,  "a",    "ako",  (SUBR)butset,  NULL, (SUBR)lobut  },
{ "butterbp", S(BBFIL), 5,  "a",    "akko", (SUBR)bbutset, NULL, (SUBR)bpbut  },
{ "butterbr", S(BBFIL), 5,  "a",    "akko", (SUBR)bbutset, NULL, (SUBR)bcbut  },
{ "buthp",    S(BFIL),  5,  "a",    "ako",  (SUBR)butset,  NULL, (SUBR)hibut  },
{ "butlp",    S(BFIL),  5,  "a",    "ako",  (SUBR)butset,  NULL, (SUBR)lobut  },
{ "butbp",    S(BBFIL), 5,  "a",    "akko", (SUBR)bbutset, NULL, (SUBR)bpbut  },
{ "butbr",    S(BBFIL), 5,  "a",    "akko", (SUBR)bbutset, NULL, (SUBR)bcbut  }
};

LINKAGE

