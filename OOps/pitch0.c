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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

// #include "csdl.h"
#include "csoundCore.h"       /*                                  PITCH.C       */
#include "cwindow.h"
#include <limits.h>
#include "pitch.h"

int32_t mute_inst(CSOUND *csound, MUTE *p)
{
    int32_t n;
    int32_t onoff = (*p->onoff == FL(0.0) ? 0 : 1);

    if (IsStringCode(*p->ins)) {
      char *ss = get_arg_string(csound,*p->ins);
      n = csound->StringArg2Insno(csound,ss,1);
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

int32_t mute_inst_S(CSOUND *csound, MUTE *p)
{
    int32_t n;
    int32_t onoff = (*p->onoff == FL(0.0) ? 0 : 1);

    n = csound->StringArg2Insno(csound, ((STRINGDAT *)p->ins)->data, 1);

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

int32_t instcount(CSOUND *csound, INSTCNT *p)
{
    int32_t n;

    if (IsStringCode(*p->ins)) {
      char *ss = get_arg_string(csound,*p->ins);
      n = csound->StringArg2Insno(csound,ss,1);
    }
    else n = *p->ins;

    if (n<0 || n > csound->engineState.maxinsno ||
        csound->engineState.instrtxtp[n] == NULL)
      *p->cnt = FL(0.0);
    else if (n==0) {  /* Count all instruments */
      int32_t tot = 1;
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

int32_t instcount_S(CSOUND *csound, INSTCNT *p)
{


    int32_t n = csound->StringArg2Insno(csound, ((STRINGDAT *)p->ins)->data, 1);

    if (n<0 || n > csound->engineState.maxinsno ||
        csound->engineState.instrtxtp[n] == NULL)
      *p->cnt = FL(0.0);
    else if (n==0) {  /* Count all instruments */
      int32_t tot = 1;
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

int32_t cpuperc(CSOUND *csound, CPU_PERC *p)
{
    int32_t n;

    if (IsStringCode(*p->instrnum)) {
      char *ss = get_arg_string(csound,*p->instrnum);
      n = csound->StringArg2Insno(csound,ss,1);
    } else n = *p->instrnum;

    if (n > 0 && n <= csound->engineState.maxinsno &&
        csound->engineState.instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->engineState.instrtxtp[n]->cpuload = *p->ipercent;
    return OK;
}

int32_t cpuperc_S(CSOUND *csound, CPU_PERC *p)
{
    int32_t n = csound->StringArg2Insno(csound, ((STRINGDAT *)p->instrnum)->data, 1);
    if (n > 0 && n <= csound->engineState.maxinsno &&
        csound->engineState.instrtxtp[n] != NULL)
      /* If instrument exists */
      csound->engineState.instrtxtp[n]->cpuload = *p->ipercent;
    return OK;
}

int32_t maxalloc(CSOUND *csound, CPU_MAXALLOC *p)
{
    int32_t n;
    if (IsStringCode(*p->instrnum)) {
      char *ss = get_arg_string(csound,*p->instrnum);
      n = csound->StringArg2Insno(csound,ss,1);
    }
    else n = *p->instrnum;
    if (n > 0 && n <= csound->engineState.maxinsno &&
        csound->engineState.instrtxtp[n] != NULL) {
      /* If instrument exists */
      csound->engineState.instrtxtp[n]->maxalloc = (int32_t)*p->icount;

	  int32_t mode = (int32_t)*p->iturnoff_mode;
	  if (UNLIKELY(mode < 0 || mode > 2)) {
		  csoundInitError(csound, Str("maxalloc: invalid mode parameter"));
	  }

      csound->engineState.instrtxtp[n]->turnoff_mode = mode;
    }
    return OK;
}

int32_t maxalloc_S(CSOUND *csound, CPU_MAXALLOC *p)
{
    int32_t n = csound->StringArg2Insno(csound, ((STRINGDAT *)p->instrnum)->data, 1);
    if (n > 0 && n <= csound->engineState.maxinsno &&
        csound->engineState.instrtxtp[n] != NULL) {
      /* If instrument exists */
      csound->engineState.instrtxtp[n]->maxalloc = (int32_t)*p->icount;

	  int32_t mode = (int32_t)*p->iturnoff_mode;
	  if (UNLIKELY(mode < 0 || mode > 2)) {
		  csoundInitError(csound, Str("maxalloc: invalid mode parameter"));
	  }

      csound->engineState.instrtxtp[n]->turnoff_mode = mode;
    }
    return OK;
}

int32_t pfun(CSOUND *csound, PFUN *p)
{
    int32_t n = (int32_t)MYFLT2LONG(*p->pnum);
    MYFLT ans;
    if (n<1) ans = FL(0.0);
    else if (n<PMAX) ans = csound->init_event->p[n];
    else if (csound->init_event->c.extra && n<PMAX+csound->init_event->c.extra[0])
      ans = csound->init_event->c.extra[n-PMAX+1];
    else ans = FL(0.0);
    /*csound->Message(csound, "p(%d) %f\n", n,ans);*/
    *p->ans = ans;
    return OK;
}

int32_t pfunk_init(CSOUND *csound, PFUNK *p)
{
    int32_t i, n = (int32_t)MYFLT2LONG(*p->pnum);
    MYFLT ans, *pfield;
    if (n<1 || n>PMAX) ans = FL(0.0);
    else ans = csound->init_event->p[n];
    /* save the pfields of the current event */
    csound->AuxAlloc(csound,
                     (csound->init_event->pcnt+1)*sizeof(MYFLT), &p->pfield);
    pfield = p->pfield.auxp;
    for (i=1; i<=csound->init_event->pcnt; i++)
      pfield[i] = csound->init_event->p[i];
    *p->ans = ans;
    return OK;
}

int32_t pfunk(CSOUND *csound, PFUNK *p)
{
    IGN(csound);
    int32_t n = (int32_t)MYFLT2LONG(*p->pnum);
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
