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

typedef struct  {
        OPDS    h;
        MYFLT   *sr, *ain, *kfc, *istor;
        MYFLT   lkf;
        double  a[8];
} BFIL;

typedef struct  {
        OPDS    h;
        MYFLT   *sr, *ain, *kfo, *kbw, *istor;
        MYFLT   lkf, lkb;
        double  a[8];
} BBFIL;

#include <math.h>
#define ROOT2 (1.4142135623730950488)

static void butter_filter(int32, MYFLT *, MYFLT *, double *);

static int butset(CSOUND *csound, BFIL *p)      /*      Hi/Lo pass set-up   */
{
    if (*p->istor==FL(0.0)) {
      p->a[6] = p->a[7] = 0.0;
      p->lkf = FL(0.0);
    }
    return OK;
}

static int bbutset(CSOUND *csound, BBFIL *p)    /*      Band set-up         */
{
    if (*p->istor==FL(0.0)) {
      p->a[6] = p->a[7] = 0.0;
      p->lkb = FL(0.0);
      p->lkf = FL(0.0);
    }
    return OK;
}

static int hibut(CSOUND *csound, BFIL *p)       /*      Hipass filter       */
{
    MYFLT       *out, *in;

    in = p->ain;
    out = p->sr;

    if (*p->kfc <= FL(0.0))     {
      int32      n = csound->ksmps;
      memcpy(out, in, n*sizeof(MYFLT));
      return OK;
    }

    if (*p->kfc != p->lkf)      {
      double    *a, c;

      a = p->a;
      p->lkf = *p->kfc;
      c = tan((double)(csound->pidsr * p->lkf));

      a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
      a[2] = -(a[1] + a[1]);
      a[3] = a[1];
      a[4] = 2.0 * ( c*c - 1.0) * a[1];
      a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];
    }
    butter_filter(csound->ksmps, in, out, p->a);
    return OK;
}

static int lobut(CSOUND *csound, BFIL *p)       /*      Lopass filter       */
{
    MYFLT       *out, *in;

    in = p->ain;
    out = p->sr;

    if (*p->kfc <= FL(0.0))     {
      memset(out, 0, csound->ksmps*sizeof(MYFLT));
      return OK;
    }

    if (*p->kfc != p->lkf)      {
      double     *a, c;

      a = p->a;
      p->lkf = *p->kfc;
      c = 1.0 / tan((double)(csound->pidsr * p->lkf));
      a[1] = 1.0 / ( 1.0 + ROOT2 * c + c * c);
      a[2] = a[1] + a[1];
      a[3] = a[1];
      a[4] = 2.0 * ( 1.0 - c*c) * a[1];
      a[5] = ( 1.0 - ROOT2 * c + c * c) * a[1];
    }

    butter_filter(csound->ksmps, in, out, p->a);
    return OK;
}

static int bpbut(CSOUND *csound, BBFIL *p)      /*      Bandpass filter     */
{
    MYFLT       *out, *in;

    in = p->ain;
    out = p->sr;
    if (*p->kbw <= FL(0.0))     {
      memset(out, 0, csound->ksmps*sizeof(MYFLT));
      return OK;
    }
    if (*p->kbw != p->lkb || *p->kfo != p->lkf) {
      double *a, c, d;
      a = p->a;
      p->lkf = *p->kfo;
      p->lkb = *p->kbw;
      c = 1.0 / tan((double)(csound->pidsr * p->lkb));
      d = 2.0 * cos((double)(csound->tpidsr * p->lkf));
      a[1] = 1.0 / (1.0 + c);
      a[2] = 0.0;
      a[3] = -a[1];
      a[4] = - c * d * a[1];
      a[5] = (c - 1.0) * a[1];
    }
    butter_filter(csound->ksmps, in, out, p->a);
    return OK;
}

static int bcbut(CSOUND *csound, BBFIL *p)      /*      Band reject filter  */
{
    MYFLT       *out, *in;

    in = p->ain;
    out = p->sr;

    if (*p->kbw <= FL(0.0))     {
      memcpy(out, in, csound->ksmps*sizeof(MYFLT));
      return OK;
    }

    if (*p->kbw != p->lkb || *p->kfo != p->lkf) {
      double *a, c, d;

      a = p->a;
      p->lkf = *p->kfo;
      p->lkb = *p->kbw;
      c = tan((double)(csound->pidsr * p->lkb));
      d = 2.0 * cos((double)(csound->tpidsr * p->lkf));
      a[1] = 1.0 / (1.0 + c);
      a[2] = - d * a[1];
      a[3] = a[1];
      a[4] = a[2];
      a[5] = (1.0 - c) * a[1];
    }

    butter_filter(csound->ksmps, in, out, p->a);
    return OK;
}

/* Filter loop */

static void butter_filter(int32 n, MYFLT *in, MYFLT *out, double *a)
{
    double t, y;
    int nn;
    for (nn=0; nn<n; nn++) {
      t = (double)in[nn] - a[4] * a[6] - a[5] * a[7];
      t = csoundUndenormalizeDouble(t); /* Not needed on AMD */
      y = t * a[1] + a[2] * a[6] + a[3] * a[7];
      a[7] = a[6];
      a[6] = t;
      out[nn] = (MYFLT)y;
    }
}

#define S(x)    sizeof(x)

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

int butter_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

