/*
    follow.c:

    Copyright (C) 1994, 1999 Paris Smaragdis, John ffitch

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

        /*      Envelope follower by Paris Smaragdis    */
        /*      Berklee College of Music Csound development team */
        /*      Copyright (c) August 1994.  All rights reserve */
        /*      Improvements 1999 John ffitch */

#include "stdopcod.h"
#include <math.h>
#include "follow.h"

static int32_t flwset(CSOUND *csound, FOL *p)
{
    double sample_rate = (double)CS_ESR;
    /* Preserve MYFLT rounding before checking the integer conversion. */
    double length = (double)(*p->len * CS_ESR);

    p->wgh = p->max = FL(0.0);
    if (UNLIKELY(!(length >= (double)INT32_MIN &&
                   length <= (double)INT32_MAX)))
      return csound->InitError(csound, Str("follow: invalid period %f"),
                               *p->len);
    p->length = (int32)length;
    if (UNLIKELY(p->length<=0L)) {           /* RWD's suggestion */
      csound->Warning(csound, "%s", Str("follow - zero length!"));
      if (UNLIKELY(!(sample_rate >= 1.0 &&
                     sample_rate <= (double)INT32_MAX)))
        return csound->InitError(csound, "%s",
                                 Str("follow: sample rate is out of range"));
      p->length = (int32)CS_ESR;
    }
    p->count = p->length;
    return OK;
}

                                /* Use absolute value rather than max/min */
static int32_t follow(CSOUND *csound, FOL *p)
{
     IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *in = p->in, *out = p->out;
    MYFLT       max = p->max;
    int32       count = p->count;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT sig = FABS(in[n]);
      if (sig > max) max = sig;
      if (UNLIKELY(--count == 0L)) {
        p->wgh = max;
        max = FL(0.0);
        count = p->length;
      }
      out[n] = p->wgh;
    }
    p->max = max;
    p->count = count;
    return OK;
}

/* The Jean-Marc Jot (IRCAM) envelope follower, from code by
   Bram.DeJong@rug.ac.be and James Maccartney posted on music-dsp;
   Transferred to csound by JPff, 2000 feb 12
*/
/* Use double coefficients even with float samples. Nonpositive times
   retain the existing 0.1-second fallback. */
#define FOLLOW2_COEFFICIENT(time) \
    exp(-6.90775527898 / ((double)CS_ESR * \
                         ((time) <= FL(0.0) ? 0.1 : (double)(time))))

static int32_t envset(CSOUND *csound, ENV *p)
{
    p->lastatt = *p->attack;
    p->ga = FOLLOW2_COEFFICIENT(p->lastatt);
    p->lastrel = *p->release;
    p->gr = FOLLOW2_COEFFICIENT(p->lastrel);
    p->envelope = 0.0;
    return OK;
}

static int32_t envext(CSOUND *csound, ENV *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    /* Keep small updates in the state; only round the output sample. */
    double      envelope = p->envelope;
    double      ga, gr;
    MYFLT       *in = p->in, *out = p->out;
    if (p->lastatt!=*p->attack) {
      p->lastatt = *p->attack;
      ga = p->ga = FOLLOW2_COEFFICIENT(p->lastatt);
    }
    else ga = p->ga;
    if (p->lastrel!=*p->release) {
      p->lastrel = *p->release;
      gr = p->gr = FOLLOW2_COEFFICIENT(p->lastrel);
    }
    else gr = p->gr;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      double inp = (double)FABS(in[n]);  /* Absolute value */
      if (envelope < inp) {
        envelope = inp + ga*(envelope-inp);
      }
      else {
        envelope = inp + gr*(envelope-inp);
      }
      out[n] = (MYFLT)envelope;
    }
    p->envelope = envelope;
    return OK;
}

#undef FOLLOW2_COEFFICIENT

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "follow",   S(FOL),   0,  "a",    "ai",   (SUBR)flwset,  (SUBR)follow  },
{ "follow2",  S(ENV),   0,  "a",    "akk",  (SUBR)envset,  (SUBR)envext  }
};

int32_t follow_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}

