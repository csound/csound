/*
    pitch0.c:

    Copyright (C) 1999 John ffitch, Istvan Varga

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

#include "csdl.h"       /*                                  PITCH.C       */
#include <limits.h>
#include "spectra.h"
#include "pitch.h"
#include "uggab.h"
#include "namedins.h"   /* IV - Oct 31 2002 */
#include "csdl.h"

int mute_inst(ENVIRON *csound, MUTE *p)
{
    int n = (int) csound->strarg2insno(csound, p->ins, p->STRARG);
    int onoff = (*p->onoff == FL(0.0) ? 0 : 1);
    if (n < 1) return NOTOK;
    if (onoff==0) {
      csound->Message(csound, Str("Muting new instances of instr %d\n"), n);
    }
    else {
      csound->Message(csound, Str("Allowing instrument %d to start\n"), n);
    }
    csound->instrtxtp[n]->muted = onoff;
    return OK;
}

int instcount(ENVIRON *csound, INSTCNT *p)
{
    int n = (int)*p->ins;
    if (n<0 || n > csound->maxinsno || csound->instrtxtp[n] == NULL)
      *p->cnt = FL(0.0);
    else
      *p->cnt = (MYFLT) csound->instrtxtp[n]->active;
    return OK;
}

/* After gabriel maldonado */

int cpuperc(ENVIRON *csound, CPU_PERC *p)
{
    int n = (int) *p->instrnum;
    if (n > 0 && n <= csound->maxinsno && csound->instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->instrtxtp[n]->cpuload = *p->ipercent;
    return OK;
}

int maxalloc(ENVIRON *csound, CPU_PERC *p)
{
    int n = (int) *p->instrnum;
    if (n > 0 && n <= csound->maxinsno && csound->instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->instrtxtp[n]->maxalloc = (int)*p->ipercent;
    return OK;
}

int pfun(ENVIRON *csound, PFUN *p)
{
    int n = (int)(*p->pnum + FL(0.5));
    MYFLT ans;
    csound->Message(csound, "p(%d) %f\n", n,*p->pnum);
    if (n<1 || n>PMAX) ans = FL(0.0);
    else ans = csound->currevent->p[n];
    *p->ans = ans;
    return OK;
}

