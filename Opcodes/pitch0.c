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

// #include "csdl.h"
#include "csoundCore.h"       /*                                  PITCH.C       */
#include "cwindow.h"
#include <limits.h>
#include "spectra.h"
#include "pitch.h"
#include "uggab.h"

int mute_inst(CSOUND *csound, MUTE *p)
{
    int n = (int) csound->strarg2insno(csound, p->ins, p->XSTRCODE);
    int onoff = (*p->onoff == FL(0.0) ? 0 : 1);
    if (UNLIKELY(n < 1)) return NOTOK;
    if (onoff==0) {
      csound->Warning(csound, Str("Muting new instances of instr %d\n"), n);
    }
    else {
      csound->Warning(csound, Str("Allowing instrument %d to start\n"), n);
    }
    csound->instrtxtp[n]->muted = onoff;
    return OK;
}

int instcount(CSOUND *csound, INSTCNT *p)
{
    int n = (int) csound->strarg2insno(csound, p->ins, p->XSTRCODE);
    if (n<0 || n > csound->maxinsno || csound->instrtxtp[n] == NULL)
      *p->cnt = FL(0.0);
    else {
      *p->cnt = ((*p->opt) ?
                 (MYFLT) csound->instrtxtp[n]->instcnt :
                 (MYFLT) csound->instrtxtp[n]->active);
    }


    return OK;
}

/* After gabriel maldonado */

int cpuperc(CSOUND *csound, CPU_PERC *p)
{
    int n = (int) csound->strarg2insno(csound, p->instrnum, p->XSTRCODE);
    if (n > 0 && n <= csound->maxinsno && csound->instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->instrtxtp[n]->cpuload = *p->ipercent;
    return OK;
}

int maxalloc(CSOUND *csound, CPU_PERC *p)
{
    int n = (int) csound->strarg2insno(csound, p->instrnum, p->XSTRCODE);
    if (n > 0 && n <= csound->maxinsno && csound->instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->instrtxtp[n]->maxalloc = (int)*p->ipercent;
    return OK;
}

int pfun(CSOUND *csound, PFUN *p)
{
    int n = (int)MYFLT2LONG(*p->pnum);
    MYFLT ans;
    if (n<1 || n>PMAX) ans = FL(0.0);
    else ans = csound->currevent->p[n];
/*     csound->Message(csound, "p(%d) %f\n", n,ans); */
    *p->ans = ans;
    return OK;
}

