/*
    schedule.c:

    Copyright (C) 1999, 2002 rasmus ekman, Istvan Varga, John ffitch, Gabriel Maldonado, matt ingalls

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

#include "cs.h"
#include "schedule.h"
#include <math.h>
#include "namedins.h"           /* IV - Oct 31 2002 */

extern INSDS   *instance(int);
extern void    showallocs(void);
extern void    deact(INSDS *), schedofftim(INSDS *);
extern INSDS *insert_event(MYFLT, MYFLT, MYFLT, int, MYFLT **, int);

typedef struct rsched {
  void          *parent;
  INSDS         *kicked;
  struct rsched *next;
} RSCHED;

static RSCHED *kicked = NULL;

/******************************************************************************/
/* triginstr - Ignite instrument events at k-rate from orchestra.             */
/* August 1999 by rasmus ekman.                                               */
/******************************************************************************/

/* Some global declarations we need */
/* extern INSDS actanchor; */  /* Chain of active instrument instances        */
extern void  infoff(MYFLT p1); /* Turn off an indef copy of instr p1          */
/*extern int   sensType;      0=score evt, 1=Linein, 2/3=midi,4=triginstr */

#define FZERO (FL(0.0))    /* (Shouldn't there be global decl's for these?) */

/* EVTNODE OrcTrigEvts;    List of started events, used in playevents() */



void queue_event(MYFLT instr,
                 MYFLT when,
                 MYFLT dur,
                 int narg,
                 MYFLT **args)
{
    int i, insno;
    MYFLT starttime;
    EVTNODE *evtlist, *newnode;
    EVTBLK  *newevt;

    printf("queue_event: %.0f %f %f ...\n", instr, when, dur);
    insno = (int)instr;
    if (instrtxtp[insno] == NULL) {
      printf(Str("schedule event ignored. instr %d undefined\n"), insno);
      perferrcnt++;
      return;
    }
    /* Create the new event */
    newnode = (EVTNODE *) mmalloc(&cenviron, (long)sizeof(EVTNODE));
    newevt = &newnode->evt;
    newevt->opcod = 'i';
    /* Set start time from kwhen */
    starttime = when;
    if (starttime < FZERO) {
      starttime = FZERO;
      if (O.msglevel & WARNMSG)
        printf(Str("WARNING: schedkwhen warning: negative kwhen reset to zero\n"));
    }
    /* Add current time (see note about kadjust in triginset() above) */
    newnode->kstart = (long)(starttime * global_ekr + FL(0.5));
    newevt->p2orig = starttime;
    newevt->p3orig = dur;
    /* Copy all arguments to the new event */
    newevt->pcnt = narg;
    for (i = 0; i < narg-3; i++)
      newevt->p[i+4] = *args[i];
    newevt->p[3] = dur;
    newevt->p[2] = starttime;    /* Set actual start time in p2 */
    newevt->p[1] = (float)(newnode->insno = insno);

/*      printf("newevt: %c %f %f %f %f %f...\n", newevt->opcod, newevt->p2orig, */
/*             newevt->p3orig, newevt->p[1], newevt->p[2], newevt->p[3]); */

    /* Insert eventnode in list of generated events */
    evtlist = &OrcTrigEvts;
    while (evtlist->nxtevt) {
      if (newnode->kstart < evtlist->nxtevt->kstart) break;
      evtlist = evtlist->nxtevt;
    }
    newnode->nxtevt = evtlist->nxtevt;
    evtlist->nxtevt = newnode;
    O.RTevents = 1;     /* Make sure kperf() looks for RT events */
    O.ksensing = 1;
    O.OrcEvts  = 1;     /* - of the appropriate type */
    return;
}

/* ********** Need to add turnoff stuff *************** */
int schedule(SCHED *p)
{
    RSCHED *rr = kicked;        /* First ensure any stragglers die */
    RSCHED *ss = NULL;          /* really incase of reinit */
    int which;
    while (rr!=NULL) {
      if (rr->parent==p) {
       if (rr->kicked->xtratim) { /*      if offtime delayed  */
        rr->kicked->relesing = 1;/*     enter reles phase     */
        rr->kicked->offtim = (global_kcounter + rr->kicked->xtratim) * global_onedkr;
        schedofftim(rr->kicked);        /*          & put in offqueue */
       }
       else deact(rr->kicked);  /*        else off now        */
       {
         RSCHED *tt = rr->next;
         free(rr);
         rr = tt;
         if (ss==NULL) kicked = rr;
       }
      }
      else {
        ss = rr; rr = rr->next;
      }
    }
    if (*p->which == SSTRCOD) {
      if (p->STRARG!=NULL) which = (int)named_instr_find(p->STRARG);
      else                 which = (int)named_instr_find(currevent->strarg);
    }
    else which = (int)(FL(0.5)+*p->which);
    if (which==0) {
      return initerror(Str("Instrument not defined"));
    }
    if (which >= curip->p1 || *p->when>0) {
      RSCHED *rr;
      /* if duration is zero assume MIDI schedule */
      MYFLT dur = *p->dur;
      printf("SCH: when = %f dur = %f\n", *p->when, dur);
      p->midi = (dur<=FL(0.0));
      if (p->midi) {
        int *xtra;
        printf("SCH: MIDI case\n");
        /* set 1 k-cycle of extratime in ordeto allow mtrnoff to
           recognize whether the note is turned off */
        if (*(xtra = &(p->h.insdshead->xtratim)) < 1 )
          *xtra = 1;
      }
      if (*p->when == 0.0) {
        p->kicked = insert_event((MYFLT)which, *p->when+global_kcounter*global_onedkr,
                                 dur, p->INOCOUNT, p->argums, p->midi);
        if (p->midi) {
          rr = (RSCHED*) malloc(sizeof(RSCHED));
          rr->parent = p; rr->kicked = p->kicked; rr->next = kicked;
          kicked = rr;
        }
      }
      else queue_event((MYFLT)which, *p->when+global_kcounter*global_onedkr, dur,
                       p->INOCOUNT, p->argums);
    }
    else
      if (O.odebug) printf("Cannot schedule instr %d at inc time %f\n",
                           which, *p->when);
    return OK;
}

int schedwatch(ENVIRON *csound, SCHED *p)
{                               /* If MIDI case watch for release */
    if (p->midi && p->h.insdshead->relesing) {
      p->midi = 0;
      if (p->kicked==NULL) return OK;
      if (p->kicked->xtratim) { /*        if offtime delayed  */
        p->kicked->relesing = 1; /*     enter reles phase */
        p->kicked->offtim = (global_kcounter + p->kicked->xtratim) * global_onedkr;
        schedofftim(p->kicked); /*              & put in offqueue */
      }
      else deact(p->kicked);    /*        else off now            */
      {
        RSCHED *rr = kicked;
        RSCHED *ss = NULL;
        while (rr!=NULL) {
          if (rr->parent==p) {
            RSCHED *tt = rr->next;
            free(rr);
            rr = tt;
            if (ss==NULL) kicked = rr;
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

int ifschedule(ENVIRON *csound, WSCHED *p)
{                       /* All we need to do is ensure the trigger is set */
    p->todo = 1;
    p->abs_when = curip->p2;            /* Adjust time */
    p->midi = 0;
    return OK;
}

int kschedule(ENVIRON *csound, WSCHED *p)
{
    if (p->todo && *p->trigger!=0.0) { /* If not done and trigger */
      RSCHED *rr;
      MYFLT dur = *p->dur;
      int which =
        (*p->which == SSTRCOD) ? (p->STRARG!=NULL ? named_instr_find(p->STRARG) :
          named_instr_find(currevent->strarg)) : (int)(FL(0.5)+*p->which);
      if (which==0) {
        return perferror(Str("Instrument not defined"));
      }
      p->midi = (dur<=FL(0.0));
      p->todo = 0;
                                /* Insert event */
      if (*p->when==0) {
        p->kicked = insert_event((MYFLT)which, p->abs_when, dur,
                                 p->INOCOUNT-4, p->argums, p->midi);
        if (p->midi) {
          rr = (RSCHED*) malloc(sizeof(RSCHED));
          rr->parent = p; rr->kicked = p->kicked; rr->next = kicked;
          kicked = rr;
        }
      }
      else queue_event((MYFLT)which, *p->when+p->abs_when, dur,
                       p->INOCOUNT-4, p->argums);
    }
    else if (p->midi && p->h.insdshead->relesing) {
                                /* If MIDI case watch for release */
      p->midi = 0;
      if (p->kicked==NULL) return OK;
      if (p->kicked->xtratim) { /*        if offtime delayed  */
        p->kicked->relesing = 1;/*      enter reles phase     */
        p->kicked->offtim = (global_kcounter + p->kicked->xtratim) * global_onedkr;
        schedofftim(p->kicked); /*          & put in offqueue */
      }
      else deact(p->kicked);    /*        else off now        */
      {
        RSCHED *rr = kicked;
        RSCHED *ss = NULL;
        while (rr!=NULL) {
          if (rr->parent==p) {
            RSCHED *tt = rr->next;
            free(rr);
            rr = tt;
            if (ss==NULL) kicked = rr;
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
int lfoset(ENVIRON *csound, LFO *p)
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
        auxalloc(csound, sizeof(MYFLT)*4097L, &p->auxd);
        p->sine = (MYFLT*)p->auxd.auxp;
      }
      for (i=0; i<4096; i++)
        p->sine[i] = (MYFLT)sin(TWOPI*(double)i/4096.0);
/*        printf("Table set up (max is %d)\n", MAXPHASE>>10); */
    }
    else if (type>5 || type<0) {
      sprintf(errmsg, Str("LFO: unknown oscilator type %d"), type);
      return initerror(errmsg);
    }
    p->lasttype = type;
    p->phs = 0;
    return OK;
}

int lfok(ENVIRON *csound, LFO *p)
{
    long        phs;
    MYFLT       fract;
    MYFLT       res;
    long        iphs;

    phs = p->phs;
    switch (p->lasttype) {
    default:
      sprintf(errmsg, Str("LFO: unknown oscilator type %d"), p->lasttype);
      return perferror(errmsg);
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
    phs += (long)(*p->xcps * MAXPHASE / ekr);
    phs &= MAXMASK;
    p->phs = phs;
    *p->res = *p->kamp * res;
    return OK;
}

int lfoa(ENVIRON *csound, LFO *p)
{
    int         nsmps = ksmps;
    long        phs;
    MYFLT       fract;
    MYFLT       res;
    long        iphs, inc;
    MYFLT       *ar, amp;

    phs = p->phs;
    inc = (long)((*p->xcps * (MYFLT)MAXPHASE)*onedsr);
    amp = *p->kamp;
    ar = p->res;
    do {
      switch (p->lasttype) {
      default:
        sprintf(errmsg, Str("LFO: unknown oscilator type %d"), p->lasttype);
        return perferror(errmsg);
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
      *ar++ = res * amp;
    } while (--nsmps);
    p->phs = phs;
    return OK;
}

/******************************************************************************/
/* triginstr - Ignite instrument events at k-rate from orchestra.             */
/* August 1999 by rasmus ekman.                                               */
/* Changes made also to Cs.h, Musmon.c and Insert.c; look for "(re Aug 1999)" */
/******************************************************************************/

/* Called from kperf() to see if any event to turn on */
int sensOrcEvent(void)
{
    if (OrcTrigEvts.nxtevt && OrcTrigEvts.nxtevt->kstart <= global_kcounter)
      return(4);    /* sensType value (0=score,1=line,2/3=midi) */
    else return(0);
}

int ktriginstr(ENVIRON *csound, TRIGINSTR *p);

int triginset(ENVIRON *csound, TRIGINSTR *p)
{
    p->prvmintim = *p->mintime;
    p->timrem = 0;
    /* An instrument is initialised before kcounter is incremented for
       this k-cycle, and begins playing after kcounter++.
       Therefore, if we should start something at the very first k-cycle of
       performance, we must thus do it now, lest it be one k-cycle late.
       But in ktriginstr() we'll need to use kcounter-1 to set the start time
       of new events. So add a separate variable for the kcounter offset (-1). */
    if (global_kcounter == 0 && *p->trigger != FZERO /*&& *p->args[1] <= FZERO*/) {
      p->kadjust = 0;   /* No kcounter offset at this time */
      ktriginstr(csound,p);
    }
    p->kadjust = -1;      /* Set kcounter offset for perf-time */
    /* Catch p3==0 (i-time only) event triggerings. */
    if (global_kcounter > 0 && *p->trigger != FZERO && p->h.insdshead->p3 == 0)
      ktriginstr(csound,p);
    return OK;
}


int ktriginstr(ENVIRON *csound, TRIGINSTR *p)
{         /* k-rate event generator */
    int i, absinsno, argnum;
    MYFLT insno, starttime;
    EVTNODE *evtlist, *newnode;
    EVTBLK  *newevt;

    if (*p->trigger == FZERO) /* Only do something if triggered */
      return OK;

    /* Get absolute instr num */
    /* IV - Oct 31 2002: allow string argument for named instruments */
    /* Instrument cannot be S and k so this does not work yet */
    if (*p->args[0] == SSTRCOD) {
      if (p->STRARG!=NULL) {
        if ((absinsno = (int) strarg2insno_p(p->STRARG)) < 1) return NOTOK;
      }
      else {
        if ((absinsno = (int) strarg2insno_p(currevent->strarg)) <1) return NOTOK;
      }
      insno = (MYFLT) absinsno;
    }
    else {
      absinsno = abs((int) (insno = *p->args[0]));
    /* Check that instrument is defined */
    if (absinsno > maxinsno || instrtxtp[absinsno] == NULL) {
      printf(Str("schedkwhen ignored. Instrument %d undefined\n"),
             absinsno);
      perferrcnt++;
      return NOTOK;
      }
    }

    /* On neg instrnum to turn off a held copy, skip mintime/maxinst tests */
    if (insno >= FZERO) {
      /* On neg instrnum, turn off a held copy.
         (Regardless of mintime/maxinst) */
      if (insno < FZERO) {
        infoff(-insno);
        return OK;
      }

      /* Check if mintime has changed */
      if (p->prvmintim != *p->mintime) {
        long timrem = (int)(*p->mintime * global_ekr + FL(0.5));
        if (timrem > 0) {
          /* Adjust countdown for new mintime */
          p->timrem += timrem - p->prvktim;
          p->prvktim = timrem;
        } else timrem = 0;
        p->prvmintim = *p->mintime;
      }

      /* Check for rate limit on event generation and count down */
      if (*p->mintime > FZERO && --p->timrem > 0)
        return OK;

      /* See if there are too many instances already */
      if (*p->maxinst >= FL(1.0)) {
        INSDS *ip;
        int numinst = 0;
        /* Count active instr instances */
        ip = &actanchor;
        while ((ip = ip->nxtact) != NULL)
          if (ip->insno == absinsno) numinst++;
        if (numinst >= (int)*p->maxinst)
          return OK;
      }
    } /* end test for non-turnoff events (pos insno) */

    /* Create the new event */
    newnode = (EVTNODE *) mmalloc(csound, (long)sizeof(EVTNODE));
    newevt = &newnode->evt;
    newevt->opcod = 'i';
    /* Set start time from kwhen */
    starttime = *p->args[1];
    if (starttime < FZERO) {
      starttime = FZERO;
      if (O.msglevel & WARNMSG)
        printf(Str("WARNING: schedkwhen warning: negative kwhen reset to zero\n"));
    }
    /* Add current time (see note about kadjust in triginset() above) */
    starttime += (MYFLT)(global_kcounter + p->kadjust) * global_onedkr;
    newnode->kstart = (long)(starttime * global_ekr + FL(0.5));
    newevt->p2orig = starttime;
    newevt->p3orig = *p->args[2];
    /* Copy all arguments to the new event */
    newevt->pcnt = argnum = p->INOCOUNT-3;
    for (i = 0; i < argnum; i++)
      newevt->p[i+1] = *p->args[i];
    newevt->p[2] = starttime;    /* Set actual start time in p2 */

    newnode->insno = absinsno;
    /* Set event activation time in k-cycles */
    newnode->kstart = (long)(starttime * global_ekr + FL(0.5));

    /* Insert eventnode in list of generated events */
    evtlist = &OrcTrigEvts;
    while (evtlist->nxtevt) {
      EVTNODE *nextlistevt = evtlist->nxtevt;
      if (newnode->kstart < nextlistevt->kstart) break;
      /* Sort events starting at same time by instr number (re dec 01) */
      if (newnode->kstart == nextlistevt->kstart &&
          newnode->insno == nextlistevt->insno &&
          newnode->evt.p[1] <= nextlistevt->evt.p[1])
        break;
      evtlist = evtlist->nxtevt;
    }
    newnode->nxtevt = evtlist->nxtevt;
    evtlist->nxtevt = newnode;
    O.RTevents = 1;     /* Make sure kperf() looks for RT events */
    O.ksensing = 1;
    O.OrcEvts  = 1;     /* - of the appropriate type */

    /* Reset min pause counter */
    if (*p->mintime > FZERO)
      p->timrem = (long)(*p->mintime * global_ekr + FL(0.5));
    else p->timrem = 0;
    return OK;
}

/* Maldonado triggering of events */

int trigseq_set(ENVIRON *csound, TRIGSEQ *p)     /* by G.Maldonado */
{
    FUNC *ftp;
    if ((ftp = ftfind(csound, p->kfn)) == NULL) {
      return initerror(Str("trigseq: incorrect table number"));
    }
    p->done=0;
    p->table = ftp->ftable;
    p->pfn = (long)*p->kfn;
    p->ndx = (long)*p->initndx;
    p->nargs = p->INOCOUNT-5;
    return OK;
}

int trigseq(ENVIRON *csound, TRIGSEQ *p)
{
    if (p->done) return OK;
    else {
      int j, nargs = p->nargs;
      long start = (long) *p->kstart, loop = (long) *p->kloop;
      long *ndx = &p->ndx;
      MYFLT *out = *p->outargs;

      if (p->pfn != (long)*p->kfn) {
        FUNC *ftp;
        if ((ftp = ftfindp(csound, p->kfn)) == NULL) {
          return perferror(Str("trigseq: incorrect table number"));
        }
        p->pfn = (long)*p->kfn;
        p->table = ftp->ftable;
      }
      if (*p->ktrig) {
        int nn = nargs * (int)*ndx;
        for (j=0; j < nargs; j++) {
          *out++ = p->table[nn+j];
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

