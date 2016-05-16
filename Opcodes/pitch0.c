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
    int n;
    int onoff = (*p->onoff == FL(0.0) ? 0 : 1);

    if (csound->ISSTRCOD(*p->ins)) {
      char *ss = get_arg_string(csound,*p->ins);
      n = csound->strarg2insno(csound,ss,1);
    } else n = *p->ins;

    if (UNLIKELY(n < 1)) return NOTOK;
    if (onoff==0) {
      csound->Warning(csound, Str("Muting new instances of instr %d\n"), n);
    }
    else {
      csound->Warning(csound, Str("Allowing instrument %d to start\n"), n);
    }
    csound->engineState.instrtxtp[n]->muted = onoff;
    return OK;
}

int mute_inst_S(CSOUND *csound, MUTE *p)
{
    int n;
    int onoff = (*p->onoff == FL(0.0) ? 0 : 1);

    n = csound->strarg2insno(csound, ((STRINGDAT *)p->ins)->data, 1);

    if (UNLIKELY(n < 1)) return NOTOK;
    if (onoff==0) {
      csound->Warning(csound, Str("Muting new instances of instr %d\n"), n);
    }
    else {
      csound->Warning(csound, Str("Allowing instrument %d to start\n"), n);
    }
    csound->engineState.instrtxtp[n]->muted = onoff;
    return OK;
}

int instcount(CSOUND *csound, INSTCNT *p)
{
    int n;

    if (csound->ISSTRCOD(*p->ins)) {
      char *ss = get_arg_string(csound,*p->ins);
      n = csound->strarg2insno(csound,ss,1);
    }
    else n = *p->ins;

    if (n<0 || n > csound->engineState.maxinsno ||
        csound->engineState.instrtxtp[n] == NULL)
      *p->cnt = FL(0.0);
    else if (n==0) {  /* Count all instruments */
      int tot = 1;
      for (n=1; n<csound->engineState.maxinsno; n++)
        if (csound->engineState.instrtxtp[n]) /* If it exists */
          tot += ((*p->opt) ? csound->engineState.instrtxtp[n]->instcnt :
                              csound->engineState.instrtxtp[n]->active);
      *p->cnt = (MYFLT)tot;
    }
    else {
      //csound->Message(csound, "Instr %p \n", csound->engineState.instrtxtp[n]);
      *p->cnt = ((*p->opt) ?
                 (MYFLT) csound->engineState.instrtxtp[n]->instcnt :
                 (MYFLT) csound->engineState.instrtxtp[n]->active);
      if (*p->norel)
        *p->cnt -= csound->engineState.instrtxtp[n]->pending_release;
    }

    return OK;
}

int instcount_S(CSOUND *csound, INSTCNT *p)
{


    int n = csound->strarg2insno(csound, ((STRINGDAT *)p->ins)->data, 1);

    if (n<0 || n > csound->engineState.maxinsno ||
        csound->engineState.instrtxtp[n] == NULL)
      *p->cnt = FL(0.0);
    else if (n==0) {  /* Count all instruments */
      int tot = 1;
      for (n=1; n<csound->engineState.maxinsno; n++)
        if (csound->engineState.instrtxtp[n]) /* If it exists */
          tot += ((*p->opt) ? csound->engineState.instrtxtp[n]->instcnt :
                              csound->engineState.instrtxtp[n]->active);
      *p->cnt = (MYFLT)tot;
    }
    else {
      *p->cnt = ((*p->opt) ?
                 (MYFLT) csound->engineState.instrtxtp[n]->instcnt :
                 (MYFLT) csound->engineState.instrtxtp[n]->active);
      if (*p->norel)
        *p->cnt -= csound->engineState.instrtxtp[n]->pending_release;
    }

    return OK;
}

/* After gabriel maldonado */

int cpuperc(CSOUND *csound, CPU_PERC *p)
{
    int n;

    if (csound->ISSTRCOD(*p->instrnum)) {
      char *ss = get_arg_string(csound,*p->instrnum);
      n = csound->strarg2insno(csound,ss,1);
    } else n = *p->instrnum;

    if (n > 0 && n <= csound->engineState.maxinsno &&
        csound->engineState.instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->engineState.instrtxtp[n]->cpuload = *p->ipercent;
    return OK;
}

int cpuperc_S(CSOUND *csound, CPU_PERC *p)
{
    int n = csound->strarg2insno(csound, ((STRINGDAT *)p->instrnum)->data, 1);
    if (n > 0 && n <= csound->engineState.maxinsno &&
        csound->engineState.instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->engineState.instrtxtp[n]->cpuload = *p->ipercent;
    return OK;
}

int maxalloc(CSOUND *csound, CPU_PERC *p)
{
    int n;

    if (csound->ISSTRCOD(*p->instrnum)) {
      char *ss = get_arg_string(csound,*p->instrnum);
      n = csound->strarg2insno(csound,ss,1);
    }
    else n = *p->instrnum;
    if (n > 0 && n <= csound->engineState.maxinsno &&
        csound->engineState.instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->engineState.instrtxtp[n]->maxalloc = (int)*p->ipercent;
    return OK;
}

int maxalloc_S(CSOUND *csound, CPU_PERC *p)
{
    int n = csound->strarg2insno(csound, ((STRINGDAT *)p->instrnum)->data, 1);
    if (n > 0 && n <= csound->engineState.maxinsno &&
        csound->engineState.instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->engineState.instrtxtp[n]->maxalloc = (int)*p->ipercent;
    return OK;
}

int pfun(CSOUND *csound, PFUN *p)
{
    int n = (int)MYFLT2LONG(*p->pnum);
    MYFLT ans;
    if (n<1) ans = FL(0.0);
    else if (n<PMAX) ans = csound->currevent->p[n];
    else if (csound->currevent->c.extra && n<PMAX+csound->currevent->c.extra[0])
      ans = csound->currevent->c.extra[n-PMAX+1];
    else ans = FL(0.0);
    /*csound->Message(csound, "p(%d) %f\n", n,ans);*/
    *p->ans = ans;
    return OK;
}

int pfunk_init(CSOUND *csound, PFUNK *p)
{
    int i, n = (int)MYFLT2LONG(*p->pnum);
    MYFLT ans, *pfield;
    if (n<1 || n>PMAX) ans = FL(0.0);
    else ans = csound->currevent->p[n];
    /* save the pfields of the current event */
    csound->AuxAlloc(csound,
                     (csound->currevent->pcnt+1)*sizeof(MYFLT), &p->pfield);
    pfield = p->pfield.auxp;
    for (i=1; i<=csound->currevent->pcnt; i++)
      pfield[i] = csound->currevent->p[i];
    *p->ans = ans;
    return OK;
}

int pfunk(CSOUND *csound, PFUNK *p)
{
    int n = (int)MYFLT2LONG(*p->pnum);
    MYFLT ans, *pfield;
    if (n<1 || n>PMAX) {
      ans = FL(0.0);
    }
    else {
      pfield = p->pfield.auxp;
      ans = pfield[n];
    }
    *p->ans = ans;
    return OK;
}
