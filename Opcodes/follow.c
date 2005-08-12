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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

        /*      Envelope follower by Paris Smaragdis    */
        /*      Berklee College of Music Csound development team */
        /*      Copyright (c) August 1994.  All rights reserve */
        /*      Improvements 1999 John ffitch */

#include "csdl.h"
#include <math.h>
#include "follow.h"

int flwset(CSOUND *csound, FOL *p)
{
    p->wgh = p->max = FL(0.0);
    p->length = (long)(*p->len * csound->esr);
    if (p->length<=0L) {           /* RWD's suggestion */
      csound->Warning(csound, Str("follow - zero length!"));
      p->length = (long)csound->esr;
    }
    p->count = p->length;
    return OK;
}

                                /* Use absolute value rather than max/min */
int follow(CSOUND *csound, FOL *p)
{
    long        n = csound->ksmps;
    MYFLT       *in = p->in, *out = p->out;
    MYFLT       max = p->max;
    long        count = p->count;
    do {
      MYFLT sig = *in++;
      if (sig > max) max = sig;
      else if (sig < -max) max = -sig;
      if (--count == 0L) {
        p->wgh = max;
        max = FL(0.0);
        count = p->length;
      }
      *out++ = p->wgh;
    } while (--n);
    p->max = max;
    p->count = count;
    return OK;
}

/* The Jean-Marc Jot (IRCAM) envelope follower, from code by
   Bram.DeJong@rug.ac.be and James Maccartney posted on music-dsp;
   Transferred to csound by JPff, 2000 feb 12
*/
int envset(CSOUND *csound, ENV *p)
{
                                /* Note - 6.90775527898 -- log(0.001) */
    p->lastatt = *p->attack;
    if (p->lastatt<=FL(0.0))
      p->ga = (MYFLT) exp(- 69.0775527898*(double)csound->onedsr);
    else
      p->ga = (MYFLT) exp(- 6.90775527898/(double)(csound->esr* p->lastatt));
    p->lastrel = *p->release;
    if (p->lastrel<=FL(0.0))
      p->gr = (MYFLT) exp(- 69.0775527898*(double)csound->onedsr);
    else
      p->gr = (MYFLT) exp(- 6.90775527898/(double)(csound->esr* p->lastrel));
    p->envelope = FL(0.0);
    return OK;
}

int envext(CSOUND *csound, ENV *p)
{
    int n, nsmps = csound->ksmps;
    MYFLT       envelope = p->envelope;
    MYFLT       ga, gr;
    MYFLT       *in = p->in, *out = p->out;
    if (p->lastrel!=*p->attack) {
      p->lastatt = *p->attack;
      if (p->lastatt<=FL(0.0))
        ga = p->ga = (MYFLT) exp(-10000.0*(double)csound->onedsr);
      else
        ga = p->ga = (MYFLT) exp(-1.0/(double)(csound->esr* p->lastatt));
    }
    else ga = p->ga;
    if (p->lastrel!=*p->release) {
      p->lastrel = *p->release;
      if (p->lastrel<=FL(0.0))
        gr = p->gr = (MYFLT) exp(-100.0*(double)csound->onedsr);
      else
        gr = p->gr = (float) exp(-1.0/(double)(csound->esr* p->lastrel));
    }
    else gr = p->gr;
    for (n=0;n<nsmps;n++) {
      MYFLT inp = in[n];
      if (inp<FL(0.0)) inp = -inp; /* Absolute value */
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
#define S       sizeof

static OENTRY localops[] = {
{ "follow",   S(FOL),   5, "a",    "ai",   (SUBR)flwset,  NULL,  (SUBR)follow  },
{ "follow2",  S(ENV),   5, "a",    "akk",  (SUBR)envset,  NULL,  (SUBR)envext  },
};

LINKAGE

