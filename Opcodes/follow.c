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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
    p->wgh = p->max = FL(0.0);
    p->length = (int32)(*p->len * CS_ESR);
    if (UNLIKELY(p->length<=0L)) {           /* RWD's suggestion */
      csound->Warning(csound, "%s", Str("follow - zero length!"));
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
static int32_t envset(CSOUND *csound, ENV *p)
{
                                /* Note - 6.90775527898 -- log(0.001) */
    p->lastatt = *p->attack;
    if (p->lastatt<=FL(0.0))
      p->ga = EXP(- FL(69.0775527898)*CS_ONEDSR);
    else
      p->ga = EXP(- FL(6.90775527898)/(CS_ESR* p->lastatt));
    p->lastrel = *p->release;
    if (p->lastrel<=FL(0.0))
      p->gr = EXP(- FL(69.0775527898)*CS_ONEDSR);
    else
      p->gr = EXP(- FL(6.90775527898)/(CS_ESR* p->lastrel));
    p->envelope = FL(0.0);
    return OK;
}

static int32_t envext(CSOUND *csound, ENV *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       envelope = p->envelope;
    MYFLT       ga, gr;
    MYFLT       *in = p->in, *out = p->out;
    if (p->lastatt!=*p->attack) {
      p->lastatt = *p->attack;
      if (p->lastatt<=FL(0.0))
        ga = p->ga = EXP(- FL(69.0775527898)*CS_ONEDSR);
      // EXP(-FL(10000.0)*CS_ONEDSR);
      else
        ga = p->ga = EXP(- FL(6.90775527898)/(CS_ESR* p->lastatt));
      //EXP(-FL(1.0)/(CS_ESR* p->lastatt));
    }
    else ga = p->ga;
    if (p->lastrel!=*p->release) {
      p->lastrel = *p->release;
      if (p->lastrel<=FL(0.0))
        gr = p->gr = EXP(- FL(69.0775527898)*CS_ONEDSR);
      //EXP(-FL(100.0)*CS_ONEDSR);
      else
        gr = p->gr = EXP(- FL(6.90775527898)/(CS_ESR* p->lastrel));
      //EXP(-FL(1.0)/(CS_ESR* p->lastrel));
    }
    else gr = p->gr;
    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      MYFLT inp = FABS(in[n]);  /* Absolute value */
      if (envelope < inp) {
        envelope = inp + ga*(envelope-inp);
      }
      else {
        envelope = inp + gr*(envelope-inp);
      }
      out[n] = envelope;
    }
    p->envelope = envelope;
    return OK;
}

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

