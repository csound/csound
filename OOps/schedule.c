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

extern INSDS  *insert_event(CSOUND*, MYFLT, MYFLT, MYFLT, int, MYFLT **, int);

typedef struct rsched {
  void          *parent;
  INSDS         *kicked;
  struct rsched *next;
} RSCHED;

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

static void queue_event(CSOUND *csound,
                        MYFLT instr, double when, MYFLT dur,
                        int narg, MYFLT **args)
{
    EVTBLK        evt;
    int           i;

    evt.strarg = NULL;
    evt.opcod = 'i';
    evt.pcnt = narg + 3;
    evt.p[1] = instr;
    evt.p[2] = FL(0.0);
    evt.p[3] = dur;
    for (i = 0; i < narg; i++)
      evt.p[i + 4] = *(args[i]);
    insert_score_event(csound, &evt, when);
}

/* ********** Need to add turnoff stuff *************** */
int schedule(CSOUND *csound, SCHED *p)
{
    RSCHED *rr = (RSCHED*) csound->schedule_kicked;
    RSCHED *ss = NULL;
    int which;
    /* First ensure any stragglers die really in case of reinit */
    while (rr!=NULL) {
      if (rr->parent==p) {
        xturnoff(csound, rr->kicked);
        {
          RSCHED *tt = rr->next;
          free(rr);
          rr = tt;
          if (ss == NULL)
            csound->schedule_kicked = (void*) rr;
        }
      }
      else {
        ss = rr; rr = rr->next;
      }
    }
    if (p->XSTRCODE)
      which = (int) named_instr_find(csound, (char*) p->which);
    else if (*p->which == SSTRCOD)
      which = (int) named_instr_find(csound, csound->currevent->strarg);
    else
      which = (int) (FL(0.5) + *p->which);
    if (UNLIKELY(which < 1 || which > csound->maxinsno ||
                 csound->instrtxtp[which] == NULL)) {
      return csound->InitError(csound, Str("Instrument not defined"));
    }
    {
      RSCHED *rr;
      /* if duration is zero assume MIDI schedule */
      MYFLT dur = *p->dur;
/*       csound->Message(csound,"SCH: when = %f dur = %f\n", *p->when, dur); */
      p->midi = (dur <= FL(0.0));
      if (UNLIKELY(p->midi)) {
        csound->Warning(csound,Str("schedule in MIDI mode is not "
                                   "implemented correctly, do not use it\n"));
        /* set 1 k-cycle of extratime in order to allow mtrnoff to
           recognize whether the note is turned off */
        if (UNLIKELY(p->h.insdshead->xtratim < 1))
          p->h.insdshead->xtratim = 1;
      }
      if (*p->when <= FL(0.0)) {
        p->kicked = insert_event(csound, (MYFLT) which,
                                 (MYFLT) (csound->icurTime/csound->esr -
                                          csound->timeOffs),
                                 dur, p->INOCOUNT - 3, p->argums, p->midi);
        if (UNLIKELY(p->midi)) {
          rr = (RSCHED*) malloc(sizeof(RSCHED));
          rr->parent = p; rr->kicked = p->kicked;
          rr->next = (RSCHED*) csound->schedule_kicked;
          csound->schedule_kicked = (void*) rr;
        }
      }
      else
        queue_event(csound, (MYFLT) which,
                    (double)*p->when + csound->icurTime/csound->esr,
                            dur, p->INOCOUNT - 3, p->argums);
    }
    return OK;
}

int schedwatch(CSOUND *csound, SCHED *p)
{                               /* If MIDI case watch for release */
    if (p->midi && p->h.insdshead->relesing) {
      p->midi = 0;
      if (p->kicked==NULL) return OK;
      xturnoff(csound, p->kicked);
      {
        RSCHED *rr = (RSCHED*) csound->schedule_kicked;
        RSCHED *ss = NULL;
        while (rr!=NULL) {
          if (rr->parent==p) {
            RSCHED *tt = rr->next;
            free(rr);
            rr = tt;
            if (ss == NULL)
              csound->schedule_kicked = (void*) rr;
          }
          else {
            ss = rr; rr = rr->next;
          }
        }
      }
      p->kicked = NULL;
    }
    return OK;
}

int ifschedule(CSOUND *csound, WSCHED *p)
{                       /* All we need to do is ensure the trigger is set */
    p->todo = 1;
    p->abs_when = p->h.insdshead->p2;
    p->midi = 0;
    return OK;
}

int kschedule(CSOUND *csound, WSCHED *p)
{
    if (p->todo && *p->trigger != FL(0.0)) { /* If not done and trigger */
      double starttime;
      RSCHED *rr;
      MYFLT dur = *p->dur;
      int which;
      if (p->XSTRCODE)
        which = (int) named_instr_find(csound, (char*) p->which);
      else if (*p->which == SSTRCOD)
        which = (int) named_instr_find(csound, csound->currevent->strarg);
      else
        which = (int) (FL(0.5) + *p->which);
      if (UNLIKELY(which < 1 || which > csound->maxinsno ||
                   csound->instrtxtp[which] == NULL)) {
        return csound->PerfError(csound, Str("Instrument not defined"));
      }
      p->midi = (dur <= FL(0.0));
      if (UNLIKELY(p->midi))
        csound->Warning(csound,
                        Str("schedule in MIDI mode is not "
                            "implemented correctly, do not use it\n"));
      p->todo = 0;
                                /* Insert event */
      starttime = (double)p->abs_when + (double)*(p->when) + csound->timeOffs;
      if (starttime*csound->esr <= csound->icurTime) {
        p->kicked = insert_event(csound, (MYFLT) which,
                                 (MYFLT) (csound->icurTime/csound->esr -
                                          csound->timeOffs),
                                 dur, p->INOCOUNT - 4, p->argums, p->midi);
        if (p->midi) {
          rr = (RSCHED*) malloc(sizeof(RSCHED));
          rr->parent = p; rr->kicked = p->kicked;
          rr->next = (RSCHED*) csound->schedule_kicked;
          csound->schedule_kicked = (void*) rr;
        }
      }
      else
        queue_event(csound, (MYFLT) which,
                    starttime, dur, p->INOCOUNT - 4, p->argums);
    }
    else if (p->midi && p->h.insdshead->relesing) {
                                /* If MIDI case watch for release */
      p->midi = 0;
      if (p->kicked==NULL) return OK;
      xturnoff(csound, p->kicked);
      {
        RSCHED *rr = (RSCHED*) csound->schedule_kicked;
        RSCHED *ss = NULL;
        while (rr!=NULL) {
          if (rr->parent==p) {
            RSCHED *tt = rr->next;
            free(rr);
            rr = tt;
            if (ss == NULL)
              csound->schedule_kicked = (void*) rr;
          }
          else {
            ss = rr; rr = rr->next;
          }
        }
      }
      p->kicked = NULL;
    }
    return OK;
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
      return csound->PerfError(csound, Str("LFO: unknown oscilator type %d"),
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
    phs += (int32)(*p->xcps * MAXPHASE * csound->onedkr);
    phs &= MAXMASK;
    p->phs = phs;
    *p->res = *p->kamp * res;
    return OK;
}

int lfoa(CSOUND *csound, LFO *p)
{
    int         n;
    int32       phs;
    MYFLT       fract;
    MYFLT       res;
    int32       iphs, inc;
    MYFLT       *ar, amp;

    phs = p->phs;
    inc = (int32)((*p->xcps * (MYFLT)MAXPHASE) * csound->onedsr);
    amp = *p->kamp;
    ar = p->res;
    for (n=0; n<csound->ksmps; n++) {
      switch (p->lasttype) {
      default:
        return csound->PerfError(csound, Str("LFO: unknown oscilator type %d"),
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

int ktriginstr(CSOUND *csound, TRIGINSTR *p);

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
      ktriginstr(csound, p);
    }
    p->kadjust = -1;      /* Set kcounter offset for perf-time */
    /* Catch p3==0 (i-time only) event triggerings. */
    if (csound->global_kcounter > 0 &&
        *p->trigger != FL(0.0) && p->h.insdshead->p3 == 0)
      ktriginstr(csound, p);
    return OK;
}

static int get_absinsno(CSOUND *csound, TRIGINSTR *p)
{
    int insno;

    /* Get absolute instr num */
    /* IV - Oct 31 2002: allow string argument for named instruments */
    if (p->XSTRCODE)
      insno = (int)strarg2insno_p(csound, (char*)p->args[0]);
    else if (*p->args[0] == SSTRCOD)
      insno = (int)strarg2insno_p(csound, csound->currevent->strarg);
    else
      insno = (int)FABS(*p->args[0]);
    /* Check that instrument is defined */
    if (UNLIKELY(insno < 1 || insno > csound->maxinsno ||
                 csound->instrtxtp[insno] == NULL)) {
      csound->Warning(csound, Str("schedkwhen ignored. "
                                  "Instrument %d undefined\n"), insno);
      csound->perferrcnt++;
      return -1;
    }
    return insno;
}

int ktriginstr(CSOUND *csound, TRIGINSTR *p)
{         /* k-rate event generator */
    long  starttime;
    int     i, argnum;
    EVTBLK  evt;
    char    name[512];

    if (p->timrem > 0)
      p->timrem--;
    if (*p->trigger == FL(0.0)) /* Only do something if triggered */
      return OK;

    /* Check if mintime has changed */
    if (p->prvmintim != *p->mintime) {
      int32 timrem = (int32) (*p->mintime * csound->global_ekr + FL(0.5));
      if (timrem > 0) {
        /* Adjust countdown for new mintime */
        p->timrem += timrem - p->prvktim;
        p->prvktim = timrem;
      }
      else
        p->timrem = 0;
      p->prvmintim = *p->mintime;
    }

    if (*p->args[0] >= FL(0.0) || *p->args[0] == SSTRCOD) {
      /* Check for rate limit on event generation */
      if (*p->mintime > FL(0.0) && p->timrem > 0)
        return OK;
      /* See if there are too many instances already */
      if (*p->maxinst >= FL(1.0)) {
        INSDS *ip;
        int absinsno, numinst = 0;
        /* Count active instr instances */
        absinsno = get_absinsno(csound, p);
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
    if (p->XSTRCODE) {
      evt.strarg = (char*) p->args[0];
      evt.p[1] = SSTRCOD;
    }
    else if (*p->args[0] == SSTRCOD) {
      unquote(name, csound->currevent->strarg, 512);
      evt.strarg = name;
      evt.p[1] = SSTRCOD;
    }
    else {
      evt.strarg = NULL;
      evt.p[1] = *p->args[0];
    }
    evt.opcod = 'i';
    evt.pcnt = argnum = p->INOCOUNT - 3;
    /* Add current time (see note about kadjust in triginset() above) */
    starttime = csound->ksmps*(csound->global_kcounter + p->kadjust);
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
      p->timrem = (int32) (*p->mintime * csound->global_ekr + FL(0.5));
    else
      p->timrem = 0;
    return
      (insert_score_event_at_sample(csound, &evt, starttime) == 0 ? OK : NOTOK);
}

/* Maldonado triggering of events */

int trigseq_set(CSOUND *csound, TRIGSEQ *p)      /* by G.Maldonado */
{
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL)) {
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
          return csound->PerfError(csound,
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

