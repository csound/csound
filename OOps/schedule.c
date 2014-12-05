/*
    schedule.c:

    Copyright (C) 1999, 2002 rasmus ekman, Istvan Varga,
                             John ffitch, Gabriel Maldonado, matt ingalls

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

#include "csoundCore.h"
#include "schedule.h"
#include <math.h>
#include "namedins.h"
#include "linevent.h"


int eventOpcodeI_(CSOUND *csound, LINEVENT *p, int s, char p1);
int eventOpcode_(CSOUND *csound, LINEVENT *p, int s, char p1);

int schedule(CSOUND *csound, SCHED *p)
{
    LINEVENT pp;
    int i;
    pp.h = p->h;
    char c[2] = "i";
    pp.args[0] = (MYFLT *) c;
    pp.args[1] = p->which;
    pp.args[2] = p->when;
    pp.args[3] = p->dur;
    pp.argno = p->INOCOUNT+1;
    for (i=4; i < pp.argno ; i++) {
      pp.args[i] = p->argums[i-4];
    }
    pp.flag = 1;
    return eventOpcodeI_(csound, &pp, 0, 'i');
}

int schedule_S(CSOUND *csound, SCHED *p)
{
    LINEVENT pp;
    int i;
    pp.h = p->h;
    char c[2] = "i";
    pp.args[0] = (MYFLT *) c;
    pp.args[1] = p->which;
    pp.args[2] = p->when;
    pp.args[3] = p->dur;
    pp.argno = p->INOCOUNT+1;
    for (i=4; i < pp.argno ; i++) {
      pp.args[i] = p->argums[i-4];
    }
    pp.flag = 1;
    return eventOpcodeI_(csound, &pp, 1, 'i');
}



int ifschedule(CSOUND *csound, WSCHED *p)
{                       /* All we need to do is ensure the trigger is set */
    IGN(csound);
    p->todo = 1;
    return OK;
}

int kschedule(CSOUND *csound, WSCHED *p)
{
    if (p->todo && *p->trigger != FL(0.0)) {
      LINEVENT pp;
      int i;
      pp.h = p->h;
      char c[2] = "i";
      pp.args[0] = (MYFLT *) c;
      pp.args[1] = p->which;
      pp.args[2] = p->when;
      pp.args[3] = p->dur;
      pp.argno = p->INOCOUNT+1;
      for(i=4; i < pp.argno ; i++) {
        pp.args[i] = p->argums[i-4];
      }
      p->todo =0;
      pp.flag = 1;
       if(IS_STR_ARG(p->which)){
          return eventOpcode_(csound, &pp, 1, 'i');
       }
       else {
         pp.flag = 0;
         return eventOpcode_(csound, &pp, 0, 'i');
       }
    }
    else return OK;
}

/* tables are 4096 entries always */

#define MAXPHASE 0x1000000
#define MAXMASK  0x0ffffff
int lfoset(CSOUND *csound, LFO *p)
{
  /* Types: 0:  sine
            1:  triangles
            2:  square (biplar)
            3:  square (unipolar)
            4:  saw-tooth
            5:  saw-tooth(down)
            */
    int type = (int)*p->type;
    if (type == 0) {            /* Sine wave so need to create */
      int i;
      if (p->auxd.auxp==NULL) {
        csound->AuxAlloc(csound, sizeof(MYFLT)*4097L, &p->auxd);
        p->sine = (MYFLT*)p->auxd.auxp;
      }
      for (i=0; i<4096; i++)
        p->sine[i] = SIN(TWOPI_F*(MYFLT)i/FL(4096.0));
/*        csound->Message(csound,"Table set up (max is %d)\n", MAXPHASE>>10); */
    }
    else if (UNLIKELY(type>5 || type<0)) {
      return csound->InitError(csound, Str("LFO: unknown oscilator type %d"),
                                       type);
    }
    p->lasttype = type;
    p->phs = 0;
    return OK;
}

int lfok(CSOUND *csound, LFO *p)
{
    int32        phs;
    MYFLT       fract;
    MYFLT       res;
    int32        iphs;

    phs = p->phs;
    switch (p->lasttype) {
    default:
      return csound->PerfError(csound, p->h.insdshead,
                               Str("LFO: unknown oscilator type %d"),
                               p->lasttype);
    case 0:
      iphs = phs >> 12;
      fract = (MYFLT)(phs & 0xfff)/FL(4096.0);
      res = p->sine[iphs];
      res = res + (p->sine[iphs+1]-res)*fract;
      break;
    case 1:                     /* Trangular */
      res = (MYFLT)((phs<<2)&MAXMASK)/(MYFLT)MAXPHASE;
      if (phs < MAXPHASE/4) {}
      else if (phs < MAXPHASE/2)
        res = FL(1.0) - res;
      else if (phs < 3*MAXPHASE/4)
        res = - res;
      else
        res = res - FL(1.0);
      break;
    case 2:                     /* Bipole square wave */
      if (phs<MAXPHASE/2) res = FL(1.0);
      else res = -FL(1.0);
      break;
    case 3:                     /* Unipolar square wave */
      if (phs<MAXPHASE/2) res = FL(1.0);
      else res = FL(0.0);
      break;
    case 4:                     /* Saw Tooth */
      res = (MYFLT)phs/(MYFLT)MAXPHASE;
      break;
    case 5:                     /* Reverse Saw Tooth */
      res = FL(1.0) - (MYFLT)phs/(MYFLT)MAXPHASE;
      break;
    }
    phs += (int32)(*p->xcps * MAXPHASE * CS_ONEDKR);
    phs &= MAXMASK;
    p->phs = phs;
    *p->res = *p->kamp * res;
    return OK;
}

int lfoa(CSOUND *csound, LFO *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32       phs;
    MYFLT       fract;
    MYFLT       res;
    int32       iphs, inc;
    MYFLT       *ar, amp;

    phs = p->phs;
    inc = (int32)((*p->xcps * (MYFLT)MAXPHASE) * csound->onedsr);
    amp = *p->kamp;
    ar = p->res;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      switch (p->lasttype) {
      default:
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("LFO: unknown oscilator type %d"),
                                 p->lasttype);
      case 0:
        iphs = phs >> 12;
        fract = (MYFLT)(phs & 0xfff)/FL(4096.0);
        res = p->sine[iphs];
        res = res + (p->sine[iphs+1]-res)*fract;
        break;
      case 1:                   /* Triangular */
        res = (MYFLT)((phs<<2)&MAXMASK)/(MYFLT)MAXPHASE;
        if (phs < MAXPHASE/4) {}
        else if (phs < MAXPHASE/2)
          res = FL(1.0) - res;
        else if (phs < 3*MAXPHASE/4)
          res = - res;
        else
          res = res - FL(1.0);
        break;
      case 2:                   /* Bipole square wave */
        if (phs<MAXPHASE/2) res = FL(1.0);
        else res = -FL(1.0);
        break;
      case 3:                   /* Unipolar square wave */
        if (phs<MAXPHASE/2) res = FL(1.0);
        else res = FL(0.0);
        break;
      case 4:                   /* Saw Tooth */
        res = (MYFLT)phs/(MYFLT)MAXPHASE;
        break;
      case 5:                   /* Reverse Saw Tooth */
        res = FL(1.0) - (MYFLT)phs/(MYFLT)MAXPHASE;
        break;
      }
      phs += inc;
      phs &= MAXMASK;
      ar[n] = res * amp;
    }
    p->phs = phs;
    return OK;
}

/******************************************************************************/
/* triginstr - Ignite instrument events at k-rate from orchestra.             */
/* August 1999 by rasmus ekman.                                               */
/* Changes made also to Cs.h, Musmon.c and Insert.c; look for "(re Aug 1999)" */
/******************************************************************************/

/******************************************************************************/
/* triginstr - Ignite instrument events at k-rate from orchestra.             */
/* August 1999 by rasmus ekman.                                               */
/******************************************************************************/

static void unquote(char *dst, char *src, int maxsize)
{
    if (src[0] == '"') {
      int len = (int) strlen(src) - 2;
      strncpy(dst, src + 1, maxsize-1);
      if (len >= 0 && dst[len] == '"')
        dst[len] = '\0';
    }
    else
      strncpy(dst, src, maxsize);
}

static int ktriginstr_(CSOUND *csound, TRIGINSTR *p, int stringname);

int triginset(CSOUND *csound, TRIGINSTR *p)
{
    p->prvmintim = *p->mintime;
    p->timrem = 0;
    /* An instrument is initialised before kcounter is incremented for
       this k-cycle, and begins playing after kcounter++.
       Therefore, if we should start something at the very first k-cycle of
       performance, we must thus do it now, lest it be one k-cycle late.
       But in ktriginstr() we'll need to use kcounter-1 to set the start time
       of new events. So add a separate variable for the kcounter offset (-1) */
    if (csound->global_kcounter == 0 &&
        *p->trigger != FL(0.0) /*&& *p->args[1] <= FL(0.0)*/) {
      p->kadjust = 0;   /* No kcounter offset at this time */
      ktriginstr_(csound, p, 0);
    }
    p->kadjust = -1;      /* Set kcounter offset for perf-time */
    /* Catch p3==0 (i-time only) event triggerings. */
    if (csound->global_kcounter > 0 &&
        *p->trigger != FL(0.0) && p->h.insdshead->p3.value == 0)
      ktriginstr_(csound, p,0);
    return OK;
}

int triginset_S(CSOUND *csound, TRIGINSTR *p)
{
    p->prvmintim = *p->mintime;
    p->timrem = 0;
    /* An instrument is initialised before kcounter is incremented for
       this k-cycle, and begins playing after kcounter++.
       Therefore, if we should start something at the very first k-cycle of
       performance, we must thus do it now, lest it be one k-cycle late.
       But in ktriginstr() we'll need to use kcounter-1 to set the start time
       of new events. So add a separate variable for the kcounter offset (-1) */
    if (csound->global_kcounter == 0 &&
        *p->trigger != FL(0.0) /*&& *p->args[1] <= FL(0.0)*/) {
      p->kadjust = 0;   /* No kcounter offset at this time */
      ktriginstr_(csound, p, 1);
    }
    p->kadjust = -1;      /* Set kcounter offset for perf-time */
    /* Catch p3==0 (i-time only) event triggerings. */
    if (csound->global_kcounter > 0 &&
        *p->trigger != FL(0.0) && p->h.insdshead->p3.value == 0)
      ktriginstr_(csound, p, 1);
    return OK;
}


static int get_absinsno(CSOUND *csound, TRIGINSTR *p, int stringname)
{
    int insno;

    /* Get absolute instr num */
    /* IV - Oct 31 2002: allow string argument for named instruments */
    if (stringname)
      insno = (int)strarg2insno_p(csound, ((STRINGDAT*)p->args[0])->data);
    else if (ISSTRCOD(*p->args[0])) {
      char *ss = get_arg_string(csound, *p->args[0]);
      insno = (int)strarg2insno_p(csound, ss);
    }
    else
      insno = (int)FABS(*p->args[0]);
    /* Check that instrument is defined */
    if (UNLIKELY(insno < 1 || insno > csound->engineState.maxinsno ||
                 csound->engineState.instrtxtp[insno] == NULL)) {
      csound->Warning(csound, Str("schedkwhen ignored. "
                                  "Instrument %d undefined\n"), insno);
      csound->perferrcnt++;
      return -1;
    }
    return insno;
}

static int ktriginstr_(CSOUND *csound, TRIGINSTR *p, int stringname)
{         /* k-rate event generator */
    long  starttime;
    int     i, argnum;
    EVTBLK  evt;
    char    name[512];
    memset(&evt, 0, sizeof(EVTBLK));

    if (p->timrem > 0)
      p->timrem--;
    if (*p->trigger == FL(0.0)) /* Only do something if triggered */
      return OK;

    /* Check if mintime has changed */
    if (p->prvmintim != *p->mintime) {
      int32 timrem = (int32) (*p->mintime * CS_EKR + FL(0.5));
      if (timrem > 0) {
        /* Adjust countdown for new mintime */
        p->timrem += timrem - p->prvktim;
        p->prvktim = timrem;
      }
      else
        p->timrem = 0;
      p->prvmintim = *p->mintime;
    }

    if (*p->args[0] >= FL(0.0) || ISSTRCOD(*p->args[0])) {
      /* Check for rate limit on event generation */
      if (*p->mintime > FL(0.0) && p->timrem > 0)
        return OK;
      /* See if there are too many instances already */
      if (*p->maxinst >= FL(1.0)) {
        INSDS *ip;
        int absinsno, numinst = 0;
        /* Count active instr instances */
        absinsno = get_absinsno(csound, p, stringname);
        if (UNLIKELY(absinsno < 1))
          return NOTOK;
        ip = &(csound->actanchor);
        while ((ip = ip->nxtact) != NULL)
          if (ip->insno == absinsno) numinst++;
        if (numinst >= (int) *p->maxinst)
          return OK;
      }
    }

    /* Create the new event */
    if (stringname) {
      evt.p[1] = csound->strarg2insno(csound,((STRINGDAT *)p->args[0])->data, 1);
      evt.strarg = NULL; evt.scnt = 0;
      /*evt.strarg = ((STRINGDAT*)p->args[0])->data;
        evt.p[1] = SSTRCOD;*/
    }
    else if (ISSTRCOD(*p->args[0])) {
      unquote(name, get_arg_string(csound, *p->args[0]), 512);
      evt.p[1] = csound->strarg2insno(csound,name, 1);
      evt.strarg = NULL;
      /* evt.strarg = name; */
      evt.scnt = 0;
      /* evt.p[1] = SSTRCOD; */
    }
    else {
      evt.strarg = NULL; evt.scnt = 0;
      evt.p[1] = *p->args[0];
    }
    evt.opcod = 'i';
    evt.pcnt = argnum = p->INOCOUNT - 3;
    /* Add current time (see note about kadjust in triginset() above) */
    starttime = CS_KSMPS*(csound->global_kcounter + p->kadjust);
    /* Copy all arguments to the new event */
    for (i = 1; i < argnum; i++)
      evt.p[i + 1] = *p->args[i];
    /* Set start time from kwhen */
    if (UNLIKELY(evt.p[2] < FL(0.0))) {
      evt.p[2] = FL(0.0);
      csound->Warning(csound,
                      Str("schedkwhen warning: negative kwhen reset to zero"));
    }
    /* Reset min pause counter */
    if (*p->mintime > FL(0.0))
      p->timrem = (int32) (*p->mintime * CS_EKR + FL(0.5));
    else
      p->timrem = 0;
    return
      (insert_score_event_at_sample(csound, &evt, starttime) == 0 ? OK : NOTOK);
}

int ktriginstr_S(CSOUND *csound, TRIGINSTR *p){
  return ktriginstr_(csound,p,1);
}

int ktriginstr(CSOUND *csound, TRIGINSTR *p){
  return ktriginstr_(csound,p,0);
}

/* Maldonado triggering of events */

int trigseq_set(CSOUND *csound, TRIGSEQ *p)      /* by G.Maldonado */
{
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->kfn)) == NULL)) {
      return csound->InitError(csound, Str("trigseq: incorrect table number"));
    }
    p->done  = 0;
    p->table = ftp->ftable;
    p->pfn   = (int32)*p->kfn;
    p->ndx   = (int32)*p->initndx;
    p->nargs = p->INOCOUNT-5;
    return OK;
}

int trigseq(CSOUND *csound, TRIGSEQ *p)
{
    if (p->done) return OK;
    else {
      int j, nargs = p->nargs;
      int32 start = (int32) *p->kstart, loop = (int32) *p->kloop;
      int32 *ndx = &p->ndx;
      MYFLT *out = *p->outargs;

      if (p->pfn != (int32)*p->kfn) {
        FUNC *ftp;
        if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL)) {
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("trigseq: incorrect table number"));
        }
        p->pfn = (int32)*p->kfn;
        p->table = ftp->ftable;
      }
      if (*p->ktrig) {
        int nn = nargs * (int)*ndx;
        for (j=0; j < nargs; j++) {
          out[j] = p->table[nn+j];
        }
        if (loop > 0) {
          (*ndx)++;
          *ndx %= loop;
          if (*ndx == 0) {
            if (start == loop) {
              p->done = 1;      /* Was bug here -- JPff 2000 May 28*/
              return OK;
            }
            *ndx += start;
          }
        }
        else if (loop < 0) {
          (*ndx)--;
          while (*ndx < start) {
            if (start == loop) {
              p->done = 1;
              return OK;
            }
            *ndx -= loop + start;
          }
        }
      }
    }
    return OK;
}
