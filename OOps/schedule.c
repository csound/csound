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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include <math.h>
#include "csoundCore.h"
#include "namedins.h"
#include "linevent.h"
/* Keep Microsoft's schedule.h from being used instead of our schedule.h. */
#ifdef _MSC_VER
#include "H/schedule.h"
#else
#include "schedule.h"
#endif

extern void csoundInputMessageInternal(CSOUND *, const char *);
int32_t eventOpcodeI_(CSOUND *csound, LINEVENT *p, int32_t s, char p1);
int32_t eventOpcode_(CSOUND *csound, LINEVENT *p, int32_t s, char p1);


int32_t schedule_array(CSOUND *csound, SCHED *p)
{
    LINEVENT pp;
    ARRAYDAT *pfields = (ARRAYDAT *) p->which;
    MYFLT *args = pfields->data;
    int32_t i;
    pp.h = p->h;
    char c[2] = "i";
    pp.args[0] = (MYFLT *) c;
    pp.argno = pfields->sizes[0] + 1;
    for (i=1; i < pp.argno ; i++) {
      pp.args[i] = args+i-1;
    }
    pp.flag = 1;
    return eventOpcodeI_(csound, &pp, 0, 'i');
}



int32_t schedule(CSOUND *csound, SCHED *p)
{
    LINEVENT pp;
    int32_t i;
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

static void add_string_arg(char *s, const char *arg) {
  int32_t offs = strlen(s) ;
  //char *c = s;
  s += offs;
  *s++ = ' ';

  *s++ ='\"';
  while(*arg != '\0') {
    if(*arg == '\"')
      *s++ = '\\';
    *s++ = *arg++;
  }

  *s++ = '\"';
  *s = '\0';
  //printf("%s \n", c);
}


int32_t schedule_N(CSOUND *csound, SCHED *p)
{
    int32_t i;
    int32_t argno = p->INOCOUNT+1;
    char s[16384], sf[64];
    sprintf(s, "i %f %f %f", *p->which, *p->when, *p->dur);
    for (i=4; i < argno ; i++) {
       MYFLT *arg = p->argums[i-4];
       if (csoundGetTypeForArg(arg) == &CS_VAR_TYPE_S) {
           add_string_arg(s, ((STRINGDAT *)arg)->data);
           //sprintf(s, "%s \"%s\" ", s, ((STRINGDAT *)arg)->data);
       }
       else {
         sprintf(sf, " %f", *arg);
         if(strlen(s) < 16384)
          strncat(s, sf, 16384-strlen(s));
         //sprintf(s, "%s %f", s,  *arg);
       }
    }

    csoundInputMessageInternal(csound, s);
    return OK;
}

int32_t schedule_SN(CSOUND *csound, SCHED *p)
{
    int32_t i;
    int32_t argno = p->INOCOUNT+1;
    char s[16384], sf[64];
    sprintf(s, "i \"%s\" %f %f", ((STRINGDAT *)p->which)->data, *p->when, *p->dur);
    for (i=4; i < argno ; i++) {
       MYFLT *arg = p->argums[i-4];
         if (csoundGetTypeForArg(arg) == &CS_VAR_TYPE_S)
           //sprintf(s, "%s \"%s\" ", s, ((STRINGDAT *)arg)->data);
           add_string_arg(s, ((STRINGDAT *)arg)->data);
        else {
         sprintf(sf, " %f", *arg);
         if(strlen(s) < 16384)
          strncat(s, sf, 16384-strlen(s));
         //sprintf(s, "%s %f", s,  *arg);
       }
    }
    //printf("%s\n", s);
    csoundInputMessageInternal(csound, s);
    return OK;
}


int32_t schedule_S(CSOUND *csound, SCHED *p)
{
    LINEVENT pp;
    int32_t i;
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



int32_t ifschedule(CSOUND *csound, WSCHED *p)
{                       /* All we need to do is ensure the trigger is set */
    IGN(csound);
    p->todo = 1;
    return OK;
}

int32_t kschedule(CSOUND *csound, WSCHED *p)
{
    if (p->todo && *p->trigger != FL(0.0)) {
      LINEVENT pp;
      int32_t i;
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
      if (IS_STR_ARG(p->which)){
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
int32_t lfoset(CSOUND *csound, LFO *p)
{
  /* Types: 0:  sine
            1:  triangles
            2:  square (biplar)
            3:  square (unipolar)
            4:  saw-tooth
            5:  saw-tooth(down)
            */
    int32_t type = (int32_t)*p->type;
    if (type == 0) {            /* Sine wave so need to create */
      int32_t i;
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

int32_t lfok(CSOUND *csound, LFO *p)
{
    int32_t     phs;
    MYFLT       fract;
    MYFLT       res;
    int32_t     iphs;

    phs = p->phs;
    switch (p->lasttype) {
    default:
      return csound->PerfError(csound, &(p->h),
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
    phs += (int32_t)(*p->xcps * MAXPHASE * CS_ONEDKR);
    phs &= MAXMASK;
    p->phs = phs;
    *p->res = *p->kamp * res;
    return OK;
}

int32_t lfoa(CSOUND *csound, LFO *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     phs;
    MYFLT       fract;
    MYFLT       res;
    int32_t     iphs, inc;
    MYFLT       *ar, amp;

    phs = p->phs;
    inc = (int32_t)((*p->xcps * (MYFLT)MAXPHASE) * csound->onedsr);
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
        return csound->PerfError(csound, &(p->h),
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

static void unquote(char *dst, char *src, int32_t maxsize)
{
    if (src[0] == '"') {
      //int32_t len = (int32_t) strlen(src) - 2;
      strNcpy(dst, src + 1, maxsize-1);
      //if (len >= 0 && dst[len] == '"') dst[len] = '\0';
    }
    else
      strNcpy(dst, src, maxsize);
}

static int32_t ktriginstr_(CSOUND *csound, TRIGINSTR *p, int32_t stringname);

int32_t triginset(CSOUND *csound, TRIGINSTR *p)
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

int32_t triginset_S(CSOUND *csound, TRIGINSTR *p)
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


static int32_t get_absinsno(CSOUND *csound, TRIGINSTR *p, int32_t stringname)
{
    int32_t insno;

    /* Get absolute instr num */
    /* IV - Oct 31 2002: allow string argument for named instruments */
    if (stringname)
      insno = (int32_t)strarg2insno_p(csound, ((STRINGDAT*)p->args[0])->data);
    else if (csound->ISSTRCOD(*p->args[0])) {
      char *ss = get_arg_string(csound, *p->args[0]);
      insno = (int32_t)strarg2insno_p(csound, ss);
    }
    else
      insno = (int32_t)FABS(*p->args[0]);
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

static int32_t ktriginstr_(CSOUND *csound, TRIGINSTR *p, int32_t stringname)
{         /* k-rate event generator */
    int64_t  starttime;
    int32_t     i, argnum;
    EVTBLK  evt;
    char    name[512];
    memset(&evt, 0, sizeof(EVTBLK));

    if (p->timrem > 0)
      p->timrem--;
    if (*p->trigger == FL(0.0)) /* Only do something if triggered */
      return OK;

    /* Check if mintime has changed */
    if (p->prvmintim != *p->mintime) {
      int32_t timrem = (int32_t) (*p->mintime * CS_EKR + FL(0.5));
      if (timrem > 0) {
        /* Adjust countdown for new mintime */
        p->timrem += timrem - p->prvktim;
        p->prvktim = timrem;
      }
      else
        p->timrem = 0;
      p->prvmintim = *p->mintime;
    }

    if (*p->args[0] >= FL(0.0) || csound->ISSTRCOD(*p->args[0])) {
      /* Check for rate limit on event generation */
      if (*p->mintime > FL(0.0) && p->timrem > 0)
        return OK;
      /* See if there are too many instances already */
      if (*p->maxinst >= FL(1.0)) {
        INSDS *ip;
        int32_t absinsno, numinst = 0;
        /* Count active instr instances */
        absinsno = get_absinsno(csound, p, stringname);
        if (UNLIKELY(absinsno < 1))
          return NOTOK;
        ip = &(csound->actanchor);
        while ((ip = ip->nxtact) != NULL)
          if (ip->insno == absinsno) numinst++;
        if (numinst >= (int32_t) *p->maxinst)
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
    else if (csound->ISSTRCOD(*p->args[0])) {
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
      p->timrem = (int32_t) (*p->mintime * CS_EKR + FL(0.5));
    else
      p->timrem = 0;
    return
      (insert_score_event_at_sample(csound, &evt, starttime) == 0 ? OK : NOTOK);
}

int32_t ktriginstr_S(CSOUND *csound, TRIGINSTR *p){
  return ktriginstr_(csound,p,1);
}

int32_t ktriginstr(CSOUND *csound, TRIGINSTR *p){
  return ktriginstr_(csound,p,0);
}

/* Maldonado triggering of events */

int32_t trigseq_set(CSOUND *csound, TRIGSEQ *p)      /* by G.Maldonado */
{
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTnp2Find(csound, p->kfn)) == NULL)) {
      return csound->InitError(csound, Str("trigseq: incorrect table number"));
    }
    p->done  = 0;
    p->table = ftp->ftable;
    p->pfn   = (int32_t)*p->kfn;
    p->ndx   = (int32_t)*p->initndx;
    p->nargs = p->INOCOUNT-5;
    return OK;
}

int32_t trigseq(CSOUND *csound, TRIGSEQ *p)
{
    if (p->done) return OK;
    else {
      int32_t j, nargs = p->nargs;
      int32_t start = (int32_t) *p->kstart, loop = (int32_t) *p->kloop;
      int32_t *ndx = &p->ndx;
      MYFLT **out = p->outargs;

      if (p->pfn != (int32_t)*p->kfn) {
        FUNC *ftp;
        if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL)) {
          return csound->PerfError(csound, &(p->h),
                                   Str("trigseq: incorrect table number"));
        }
        p->pfn = (int32_t)*p->kfn;
        p->table = ftp->ftable;
      }
      if (*p->ktrig) {
        int32_t nn = nargs * (int32_t)*ndx;
        for (j=0; j < nargs; j++) {
          *out[j] = p->table[nn+j];
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
