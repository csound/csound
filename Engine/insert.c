/*
  insert.c:

  Copyright (C) 1991, 1997, 1999, 2002, 2005
  Barry Vercoe, Istvan Varga, John ffitch,
  Gabriel Maldonado, matt ingalls

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

#include "csoundCore.h" /*                              INSERT.C        */
#include "oload.h"
#include "insert.h"     /* for goto's */
#include "aops.h"       /* for cond's */
#include "midiops.h"
#include "namedins.h"   /* IV - Oct 31 2002 */
#include "pstream.h"
#include "interlocks.h"

static  void    showallocs(CSOUND *);
static  void    deact(CSOUND *, INSDS *);
static  void    schedofftim(CSOUND *, INSDS *);
void    beatexpire(CSOUND *, double);
void    timexpire(CSOUND *, double);
static  void    instance(CSOUND *, int);
extern int argsRequired(char* argString);

int init0(CSOUND *csound)
{
    INSTRTXT  *tp = csound->engineState.instrtxtp[0];
    INSDS     *ip;

    instance(csound, 0);                            /* allocate instr 0     */
    csound->curip = ip = tp->act_instance;
    tp->act_instance = ip->nxtact;
    csound->ids = (OPDS*) ip;
    tp->active++;
    ip->actflg++;
    ip->ksmps = csound->ksmps;
    ip->ekr = csound->ekr;
    ip->kcounter = csound->kcounter;
    ip->onedksmps = csound->onedksmps;
    ip->onedkr = csound->onedkr;
    ip->kicvt = csound->kicvt;
    csound->inerrcnt = 0;
    while ((csound->ids = csound->ids->nxti) != NULL) {
      (*csound->ids->iopadr)(csound, csound->ids);  /*   run all i-code     */
    }
    return csound->inerrcnt;                        /*   return errcnt      */
}

static void putop(CSOUND *csound, TEXT *tp)
{
    int n, nn;

    if ((n = tp->outlist->count) != 0) {
      nn = 0;
      while (n--)
        csound->Message(csound, "%s\t", tp->outlist->arg[nn++]);
    }
    else
      csound->Message(csound, "\t");
    csound->Message(csound, "%s\t", tp->opcod);
    if ((n = tp->inlist->count) != 0) {
      nn = 0;
      while (n--)
        csound->Message(csound, "%s\t", tp->inlist->arg[nn++]);
    }
    csound->Message(csound, "\n");
}

static void set_xtratim(CSOUND *csound, INSDS *ip)
{
    if (UNLIKELY(ip->relesing))
      return;
    ip->offtim = (csound->icurTime +
                  ip->ksmps * (double) ip->xtratim)/csound->esr;
    ip->offbet = csound->curBeat + (csound->curBeat_inc * (double) ip->xtratim);
    ip->relesing = 1;
    csound->engineState.instrtxtp[ip->insno]->pending_release++;
}

/* insert an instr copy into active list */
/*      then run an init pass            */

int insert(CSOUND *csound, int insno, EVTBLK *newevtp)
{
    INSTRTXT  *tp;
    INSDS     *ip, *prvp, *nxtp;
    OPARMS    *O = csound->oparms;
    int tie=0;

    if (UNLIKELY(csound->advanceCnt))
      return 0;
    if (UNLIKELY(O->odebug)) {
      char *name = csound->engineState.instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("activating instr %s at %d\n"),
                        name, csound->icurTime);
      else
        csound->Message(csound, Str("activating instr %d at %d\n"),
                        insno, csound->icurTime);
    }
    csound->inerrcnt = 0;
    tp = csound->engineState.instrtxtp[insno];
    if (UNLIKELY(tp->muted == 0)) {
      char *name = csound->engineState.instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Warning(csound, Str("Instrument %s muted\n"), name);
      else
        csound->Warning(csound, Str("Instrument %d muted\n"), insno);
      return 0;
    }
    /* if (UNLIKELY(tp->mdepends & MO)) { */
    /*   char *name = csound->engineState.instrtxtp[insno]->insname; */
    /*   if (UNLIKELY(name)) */
    /*     csound->Message(csound, Str("instr %s expects midi event data, " */
    /*                                 "cannot run from score\n"), name); */
    /*   else */
    /*   csound->Message(csound, Str("instr %d expects midi event data, " */
    /*                               "cannot run from score\n"), insno); */
    /*   return(1); */
    /* } */
    if (tp->cpuload > FL(0.0)) {
      csound->cpu_power_busy += tp->cpuload;
      /* if there is no more cpu processing time*/
      if (UNLIKELY(csound->cpu_power_busy > FL(100.0))) {
        csound->cpu_power_busy -= tp->cpuload;
        csoundWarning(csound, Str("cannot allocate last note because "
                                  "it exceeds 100%% of cpu time"));
        return(0);
      }
    }
    if (UNLIKELY(tp->maxalloc > 0 && tp->active >= tp->maxalloc)) {
      csoundWarning(csound, Str("cannot allocate last note because it exceeds "
                                "instr maxalloc"));
      return(0);
    }
    /* if find this insno, active, with indef (tie) & matching p1 */
    for (ip = tp->instance; ip != NULL; ip = ip->nxtinstance) {
      if (ip->actflg && ip->offtim < 0.0 && ip->p1 == newevtp->p[1]) {
        csound->tieflag++;
        ip->tieflag = 1;
        tie = 1;
        goto init;                      /*     continue that event */
      }
    }
    /* alloc new dspace if needed */
    if (tp->act_instance == NULL || tp->isNew) {
      if (UNLIKELY(O->msglevel & RNGEMSG)) {
        char *name = csound->engineState.instrtxtp[insno]->insname;
        if (UNLIKELY(name))
          csound->Message(csound, Str("new alloc for instr %s:\n"), name);
        else
          csound->Message(csound, Str("new alloc for instr %d:\n"), insno);
      }
      instance(csound, insno);
      tp->isNew=0;
    }
    /* **** COVERITY: note that call to instance fills in structure to
       **** which tp points.  This is a false positive **** */
     /* pop from free instance chain */
    if (UNLIKELY(csound->oparms->odebug))
      csoundMessage(csound, "insert(): tp->act_instance = %p \n", tp->act_instance);
    ip = tp->act_instance;
    tp->act_instance = ip->nxtact;
    ip->insno = (int16) insno;
    ip->ksmps = csound->ksmps;
    ip->ekr = csound->ekr;
    ip->kcounter = csound->kcounter;
    ip->onedksmps = csound->onedksmps;
    ip->onedkr = csound->onedkr;
    ip->kicvt = csound->kicvt;
    ip->pds = NULL;
    /* Add an active instrument */
    tp->active++;
    tp->instcnt++;
    csound->dag_changed++;      /* Need to remake DAG */
    //printf("**** dag changed by insert\n");
    nxtp = &(csound->actanchor);    /* now splice into activ lst */
    while ((prvp = nxtp) && (nxtp = prvp->nxtact) != NULL) {
      if (nxtp->insno > insno ||
          (nxtp->insno == insno && nxtp->p1 > newevtp->p[1])) {
        nxtp->prvact = ip;
        break;
      }
    }
    ip->nxtact = nxtp;
    ip->prvact = prvp;
    prvp->nxtact = ip;
    ip->actflg++;                   /*    and mark the instr active */
    {
      int    n;
      MYFLT  *flp, *fep;

    init:
      if (tp->psetdata) {
        MYFLT *pdat = tp->psetdata + 2;
        int32 nn = tp->pmax - 2;             /*   put cur vals in pflds */
        memcpy(&ip->p3, pdat, sizeof(MYFLT)*nn);
      }
      if (UNLIKELY((n = tp->pmax) != newevtp->pcnt && !tp->psetdata)) {
        char *name = csound->engineState.instrtxtp[insno]->insname;
        if (UNLIKELY(name))
          csoundWarning(csound, Str("instr %s uses %d p-fields but is given %d"),
                        name, n, newevtp->pcnt);
        else
          csoundWarning(csound, Str("instr %d uses %d p-fields but is given %d"),
                        insno, n, newevtp->pcnt);
      }
      if (newevtp->p3orig >= FL(0.0))
        ip->offbet = csound->beatOffs
          + (double) newevtp->p2orig + (double) newevtp->p3orig;
      else
        ip->offbet = -1.0;
      flp = &ip->p1;
      fep = &newevtp->p[1];
      if (UNLIKELY(O->odebug))
        csound->Message(csound, "psave beg at %p\n", (void*) flp);
      if (n > newevtp->pcnt) n = newevtp->pcnt; /* IV - Oct 20 2002 */
      memcpy(flp, fep, n * sizeof(MYFLT)); flp += n;
      if (n < tp->pmax && tp->psetdata==NULL)
        memset(flp, 0, (tp->pmax - n) * sizeof(MYFLT));
      if (UNLIKELY(O->odebug))
        csound->Message(csound, "   ending at %p\n", (void*) flp);
    }

#ifdef HAVE_ATOMIC_BUILTIN
    __sync_lock_test_and_set((int*)&ip->init_done,1);
#else
    ip->init_done = 1;
#endif
    if (O->Beatmode)
      ip->p2 = (MYFLT) (csound->icurTime/csound->esr - csound->timeOffs);
    ip->offtim       = (double) ip->p3;         /* & duplicate p3 for now */
    ip->m_chnbp      = (MCHNBLK*) NULL;
    ip->xtratim      = 0;
    ip->relesing     = 0;
    ip->m_sust       = 0;
    ip->nxtolap      = NULL;
    ip->opcod_iobufs = NULL;
    ip->strarg = newevtp->strarg;  /* copy strarg so it does not get lost */
#ifdef HAVE_ATOMIC_BUILTIN
    __sync_lock_test_and_set((int*)&ip->init_done,0);
#else
    ip->init_done = 0;
#endif


    if (csound->realtime_audio_flag == 0) {
     csound->curip    = ip;
     csound->ids      = (OPDS *)ip;
      /* do init pass for this instr */
      while ((csound->ids = csound->ids->nxti) != NULL) {
        if (UNLIKELY(O->odebug))
          csound->Message(csound, "init %s:\n",
                          csound->ids->optext->t.oentry->opname);
        (*csound->ids->iopadr)(csound, csound->ids);
      }
      ip->init_done = 1;
      ip->tieflag  = 0;
      ip->reinitflag = 0;
      csound->tieflag = csound->reinitflag = 0;
    }

    if (UNLIKELY(csound->inerrcnt || ip->p3 == FL(0.0))) {
      xturnoff_now(csound, ip);
      return csound->inerrcnt;
    }

    /* new code for sample-accurate timing, not for tied notes */
    if (O->sampleAccurate & !tie) {
      int64_t start_time_samps, start_time_kcycles;
      double duration_samps;
      start_time_samps = (int64_t) (ip->p2 * csound->esr);
      duration_samps =  ip->p3 * csound->esr;
      start_time_kcycles = start_time_samps/csound->ksmps;
      ip->ksmps_offset = start_time_samps - start_time_kcycles*csound->ksmps;
      //printf("ksmps offset = %d \n",  ip->ksmps_offset);
      ip->no_end = csound->ksmps -
        ((int)duration_samps+ip->ksmps_offset)%csound->ksmps;
      /* the ksmps_no_end field is initially 0, set to no_end in the last
         perf cycle */
      ip->ksmps_no_end = 0;
      /* if (ip->no_end) { */
      /*   //        printf(">>>> %d\n",((int)duration_samps+ip->ksmps_offset)); */
      /*   printf(">>>> no_end=%d (%ld,%d,%f,%d)\n", */
      /*          ip->no_end, (long)duration_kcycles, csound->ksmps, */
      /*          duration_samps, ip->ksmps_offset); */
      /*   //printf("   > p2=%f p3=%f\n", ip->p2, ip->p3); */
      /* } */
    }
    else {
      /* ksmps_offset = */
      ip->ksmps_offset = 0;
      ip->ksmps_no_end = 0;
      ip->no_end = 0;
    }

#ifdef BETA
    if (UNLIKELY(O->odebug))
      csound->Message(csound, "In insert:  %d %lf %lf\n",
                      __LINE__, ip->p3, ip->offtim); /* *********** */
#endif
    if (ip->p3 > FL(0.0) && ip->offtim > 0.0) { /* if still finite time, */
      double p2 = (double) ip->p2 + csound->timeOffs;
      ip->offtim = p2 + (double) ip->p3;
      //csound->Message(csound, "%lf\n", ip->offtim);
      /* csound->Message(csound, "ip->offtim = %lf -> ", ip->offtim); */
      if (O->sampleAccurate && !tie) /* ceil for sample-accurate ending */
        ip->offtim = CEIL(ip->offtim*csound->ekr) / csound->ekr;
      else /* normal : round */
        ip->offtim = FLOOR(ip->offtim * csound->ekr +0.5)/csound->ekr;
      // csound->Message(csound, "%lf\n", ip->offtim);
      if (O->Beatmode) {
        p2 = ((p2*csound->esr - csound->icurTime) / csound->ibeatTime)
          + csound->curBeat;
        ip->offbet = p2 + ((double) ip->p3*csound->esr / csound->ibeatTime);
      }
#ifdef BETA
      if (UNLIKELY(O->odebug))
        csound->Message(csound,
                        "Calling schedofftim line %d; offtime= %lf (%lf)\n",
                        __LINE__, ip->offtim, ip->offtim*csound->ekr);
#endif
      schedofftim(csound, ip);                  /*   put in turnoff list */
    }
    else {
      ip->offbet = -1.0;
      ip->offtim = -1.0;                        /*   else mark indef     */
    }
    if (UNLIKELY(O->odebug)) {
      char *name = csound->engineState.instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("instr %s now active:\n"), name);
      else
        csound->Message(csound, Str("instr %d now active:\n"), insno);
      showallocs(csound);
    }
    if (newevtp->pinstance != NULL) {
      *((MYFLT *)newevtp->pinstance) = (MYFLT) ((long) ip);
    }
    return 0;
}

/* insert a MIDI instr copy into active list */
/*  then run an init pass                    */

int MIDIinsert(CSOUND *csound, int insno, MCHNBLK *chn, MEVENT *mep)
{
    INSTRTXT  *tp;
    INSDS     *ip, **ipp, *prvp, *nxtp;
    OPARMS    *O = csound->oparms;

    if (UNLIKELY(csound->advanceCnt))
      return 0;
    if (UNLIKELY(insno <= 0 || csound->engineState.instrtxtp[insno]->muted == 0))
      return 0;     /* muted */

    tp = csound->engineState.instrtxtp[insno];
    if (tp->cpuload > FL(0.0)) {
      csound->cpu_power_busy += tp->cpuload;
      if (UNLIKELY(csound->cpu_power_busy > FL(100.0))) {
        /* if there is no more cpu time */
        csound->cpu_power_busy -= tp->cpuload;
        csoundWarning(csound, Str("cannot allocate last note because "
                                  "it exceeds 100%% of cpu time"));
        return(0);
      }
    }
    if (UNLIKELY(tp->maxalloc > 0 && tp->active >= tp->maxalloc)) {
      csoundWarning(csound, Str("cannot allocate last note because it exceeds "
                                "instr maxalloc"));
      return(0);
    }
    tp->active++;
    tp->instcnt++;
    if (UNLIKELY(O->odebug)) {
      char *name = csound->engineState.instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("MIDI activating instr %s\n"), name);
      else
        csound->Message(csound, Str("MIDI activating instr %d\n"), insno);
    }
    csound->inerrcnt = 0;
    ipp = &chn->kinsptr[mep->dat1];       /* key insptr ptr           */
    /* alloc new dspace if needed */
    if (tp->act_instance == NULL || tp->isNew) {
      if (UNLIKELY(O->msglevel & RNGEMSG)) {
        char *name = csound->engineState.instrtxtp[insno]->insname;
        if (UNLIKELY(name))
          csound->Message(csound, Str("new MIDI alloc for instr %s:\n"), name);
        else
          csound->Message(csound, Str("new MIDI alloc for instr %d:\n"), insno);
      }
      instance(csound, insno);
      tp->isNew = 0;
    }
    /* pop from free instance chain */
     /* **** COVERITY: note that call to instance fills in structure to
       **** which tp points.  This is a false positive **** */
    ip = tp->act_instance;
    tp->act_instance = ip->nxtact;
    ip->insno = (int16) insno;

    if (UNLIKELY(O->odebug))
      csound->Message(csound, "Now %d active instr %d\n", tp->active, insno);
    if (UNLIKELY((prvp = *ipp) != NULL)) {          /*   if key currently activ */
      csoundWarning(csound,
                    Str("MIDI note overlaps with key %d on same channel"),
                    (int) mep->dat1);
      while (prvp->nxtolap != NULL)       /*   append to overlap list */
        prvp = prvp->nxtolap;
      prvp->nxtolap = ip;
    }
    else
      *ipp = ip;
    /* of overlapping notes, the one that was turned on first will be */
    /* turned off first as well */
    ip->nxtolap = NULL;

    nxtp = &(csound->actanchor);          /* now splice into activ lst */
    while ((prvp = nxtp) && (nxtp = prvp->nxtact) != NULL) {
      if (nxtp->insno > insno) {
        nxtp->prvact = ip;
        break;
      }
    }
    ip->nxtact       = nxtp;
    ip->prvact       = prvp;
    prvp->nxtact     = ip;
    ip->actflg++;                         /* and mark the instr active */
    ip->m_chnbp      = chn;               /* rec address of chnl ctrl blk */
    ip->m_pitch      = (unsigned char) mep->dat1;    /* rec MIDI data   */
    ip->m_veloc      = (unsigned char) mep->dat2;
    ip->xtratim      = 0;
    ip->m_sust       = 0;
    ip->relesing     = 0;
    ip->offbet       = -1.0;
    ip->offtim       = -1.0;              /* set indef duration */
    ip->opcod_iobufs = NULL;              /* IV - Sep 8 2002:            */
    ip->p1           = (MYFLT) insno;     /* set these required p-fields */
    ip->p2           = (MYFLT) (csound->icurTime/csound->esr - csound->timeOffs);
    ip->p3           = FL(-1.0);
    ip->ksmps = csound->ksmps;
    ip->ekr = csound->ekr;
    ip->kcounter = csound->kcounter;
    ip->onedksmps = csound->onedksmps;
    ip->onedkr = csound->onedkr;
    ip->kicvt = csound->kicvt;
    //#endif
    ip->pds = NULL;
    if (tp->psetdata != NULL) {
      MYFLT *pfld = &ip->p3;              /* if pset data present */
      MYFLT *pdat = tp->psetdata + 2;
      int32 nn = tp->pmax - 2;             /*   put cur vals in pflds */
      memcpy(pfld, pdat, nn*sizeof(MYFLT));
    }

    /* MIDI channel message note on routing overrides pset: */

    if (O->midiKey) {
      int pfield = O->midiKey;
      int index = pfield - 1;
      MYFLT *pfields = &ip->p1;
      MYFLT value = (MYFLT) ip->m_pitch;
      pfields[index] = value;
      if (UNLIKELY(O->msglevel & WARNMSG)) {
        csound->Message(csound, "  midiKey:         pfield: %3d  value: %3d\n",
                        pfield, (int) pfields[index]);
      }
    }
    else if (O->midiKeyCps) {
      int pfield = O->midiKeyCps;
      int index = pfield - 1;
      MYFLT *pfields = &ip->p1;
      MYFLT value = (MYFLT) ip->m_pitch;
      value = value / FL(12.0) + FL(3.0);
      value = value * OCTRES;
      value = (MYFLT) CPSOCTL((int32) value);
      pfields[index] = value;
      if (UNLIKELY(O->msglevel & WARNMSG)) {
        csound->Message(csound, "  midiKeyCps:      pfield: %3d  value: %3d\n",
                        pfield, (int) pfields[index]);
      }
    }
    else if (O->midiKeyOct) {
      int pfield = O->midiKeyOct;
      int index = pfield - 1;
      MYFLT *pfields = &ip->p1;
      MYFLT value = (MYFLT) ip->m_pitch;
      value = value / FL(12.0) + FL(3.0);
      pfields[index] = value;
      if (UNLIKELY(O->msglevel & WARNMSG)) {
        csound->Message(csound, "  midiKeyOct:      pfield: %3d  value: %3d\n",
                        pfield, (int) pfields[index]);
      }
    }
    else if (O->midiKeyPch) {
      int pfield = O->midiKeyPch;
      int index = pfield - 1;
      MYFLT *pfields = &ip->p1;
      MYFLT value = (MYFLT) ip->m_pitch;
      double octave = 0;
      double fraction = 0.0;
      value = value / FL(12.0) + FL(3.0);
      fraction = modf(value, &octave);
      fraction *= 0.12;
      value = octave + fraction;
      pfields[index] = value;
      if (UNLIKELY(O->msglevel & WARNMSG)) {
        csound->Message(csound, "  midiKeyPch:      pfield: %3d  value: %3d\n",
                        pfield, (int) pfields[index]);
      }
    }
    if (O->midiVelocity) {
      int pfield = O->midiVelocity;
      int index = pfield - 1;
      MYFLT *pfields = &ip->p1;
      MYFLT value = (MYFLT) ip->m_veloc;
      pfields[index] = value;
      if (UNLIKELY(O->msglevel & WARNMSG)) {
        csound->Message(csound, "  midiVelocity:    pfield: %3d  value: %3d\n",
                        pfield, (int) pfields[index]);
      }
    }
    else if (O->midiVelocityAmp) {
      int pfield = O->midiVelocityAmp;
      int index = pfield - 1;
      MYFLT *pfields = &ip->p1;
      MYFLT value = (MYFLT) ip->m_veloc;
      value = value * value / FL(16239.0);
      value = value * csound->e0dbfs;
      pfields[index] = value;
      if (UNLIKELY(O->msglevel & WARNMSG)) {
        csound->Message(csound, "  midiVelocityAmp: pfield: %3d  value: %3d\n",
                        pfield, (int)pfields[index]);
      }
    }
#ifdef HAVE_ATOMIC_BUILTIN
    __sync_lock_test_and_set((int*)&ip->init_done,0);
#else
    ip->init_done = 0;
#endif
    csound->curip    = ip;
    if (csound->realtime_audio_flag == 0) {
     csound->ids      = (OPDS *)ip;
      /* do init pass for this instr  */
      while ((csound->ids = csound->ids->nxti) != NULL) {
        if (UNLIKELY(O->odebug))
          csound->Message(csound, "init %s:\n",
                          csound->ids->optext->t.oentry->opname);
        (*csound->ids->iopadr)(csound, csound->ids);
      }
      ip->init_done = 1;
      ip->tieflag = ip->reinitflag = 0;
      csound->tieflag = csound->reinitflag = 0;
    }

    if (UNLIKELY(csound->inerrcnt)) {
      xturnoff_now(csound, ip);
      return csound->inerrcnt;
    }
    if (UNLIKELY(O->odebug)) {
      char *name = csound->engineState.instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("instr %s now active:\n"), name);
      else
        csound->Message(csound, Str("instr %d now active:\n"), insno);
      showallocs(csound);
    }
    return 0;
}

static void showallocs(CSOUND *csound)      /* debugging aid */
{
    INSTRTXT *txtp;
    INSDS   *p;

    csound->Message(csound, "insno\tinstanc\tnxtinst\tprvinst\tnxtact\t"
                    "prvact\tnxtoff\tactflg\tofftim\n");
    for (txtp = &(csound->engineState.instxtanchor);
         txtp != NULL;
         txtp = txtp->nxtinstxt)

      if ((p = txtp->instance) != NULL) {
        /*
         * On Alpha, we print pointers as pointers.  heh 981101
         * and now on all platforms (JPff)
         */
        do {
          csound->Message(csound, "%d\t%p\t%p\t%p\t%p\t%p\t%p\t%d\t%3.1f\n",
                          (int) p->insno, (void*) p,
                          (void*) p->nxtinstance, (void*) p->prvinstance,
                          (void*) p->nxtact, (void*) p->prvact,
                          (void*) p->nxtoff, p->actflg, p->offtim);
        } while ((p = p->nxtinstance) != NULL);
      }
}

static void schedofftim(CSOUND *csound, INSDS *ip)
{                               /* put an active instr into offtime list  */
    INSDS *prvp, *nxtp;         /* called by insert() & midioff + xtratim */

    if ((nxtp = csound->frstoff) == NULL ||
        nxtp->offtim > ip->offtim) {            /*   set into       */
      csound->frstoff = ip;                     /*   firstoff chain */
      ip->nxtoff = nxtp;
      /* IV - Feb 24 2006: check if this note already needs to be turned off */
      /* the following comparisons must match those in sensevents() */
#ifdef BETA
      if (UNLIKELY(csound->oparms->odebug))
        csound->Message(csound,"schedofftim: %lf %lf %f\n",
                        ip->offtim, csound->icurTime/csound->esr,
                        csound->curTime_inc);

#endif
      if (csound->oparms_.Beatmode) {
        double  tval = csound->curBeat + (0.505 * csound->curBeat_inc);
        if (ip->offbet <= tval) beatexpire(csound, tval);
      }
      else {
        double  tval = (csound->icurTime + (0.505 * csound->ksmps))/csound->esr;
        if (ip->offtim <= tval) timexpire(csound, tval);
      }
#ifdef BETA
      if (UNLIKELY(csound->oparms->odebug))
        csound->Message(csound,"schedofftim: %lf %lf\n", ip->offtim,
                        (csound->icurTime + (0.505 * csound->ksmps))/csound->esr,
                        csound->ekr*((csound->icurTime +
                                      (0.505 * csound->ksmps))/csound->esr));
#endif
    }
    else {
      while ((prvp = nxtp)
             && (nxtp = nxtp->nxtoff) != NULL
             && ip->offtim >= nxtp->offtim);
      prvp->nxtoff = ip;
      ip->nxtoff = nxtp;
    }
}

/* csound.c */
extern  int     csoundDeinitialiseOpcodes(CSOUND *csound, INSDS *ip);
int     useropcd(CSOUND *, UOPCODE*);

static void deact(CSOUND *csound, INSDS *ip)
{                               /* unlink single instr from activ chain */
    INSDS  *nxtp;               /*      and mark it inactive            */
    /*   close any files in fd chain        */

    if (ip->nxtd != NULL)
      csoundDeinitialiseOpcodes(csound, ip);
    /* remove an active instrument */
    csound->engineState.instrtxtp[ip->insno]->active--;
    if (ip->xtratim > 0)
      csound->engineState.instrtxtp[ip->insno]->pending_release--;
    csound->cpu_power_busy -= csound->engineState.instrtxtp[ip->insno]->cpuload;
    /* IV - Sep 8 2002: free subinstr instances */
    /* that would otherwise result in a memory leak */
    if (ip->opcod_deact) {
      UOPCODE *p = (UOPCODE*) ip->opcod_deact;          /* IV - Oct 26 2002 */
      deact(csound, p->ip);     /* deactivate */
      p->ip = NULL;
      /* IV - Oct 26 2002: set perf routine to "not initialised" */
      p->h.opadr = (SUBR) useropcd;
      ip->opcod_deact = NULL;
    }
    if (ip->subins_deact) {
      deact(csound, ((SUBINST*) ip->subins_deact)->ip); /* IV - Oct 24 2002 */
      ((SUBINST*) ip->subins_deact)->ip = NULL;
      ip->subins_deact = NULL;
    }
    if (UNLIKELY(csound->oparms->odebug)) {
      char *name = csound->engineState.instrtxtp[ip->insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("removed instance of instr %s\n"), name);
      else
        csound->Message(csound, Str("removed instance of instr %d\n"), ip->insno);
    }
    /* IV - Oct 24 2002: ip->prvact may be NULL, so need to check */
    if (ip->prvact && (nxtp = ip->prvact->nxtact = ip->nxtact) != NULL)
      nxtp->prvact = ip->prvact;
    ip->actflg = 0;
    /* link into free instance chain */
    /* This also destroys ip->nxtact causing loops */
    if (csound->engineState.instrtxtp[ip->insno] == ip->instr){
      ip->nxtact = csound->engineState.instrtxtp[ip->insno]->act_instance;
      csound->engineState.instrtxtp[ip->insno]->act_instance = ip;
    }
    if (ip->fdchp != NULL)
      fdchclose(csound, ip);
    csound->dag_changed++;
    //printf("**** dag changed by deact\n");
}

int kill_instance(CSOUND *csound, KILLOP *p) {
    if (LIKELY(*p->inst)) xturnoff(csound, (INSDS *) ((long)*p->inst));
  else csound->Warning(csound, "instance not valid \n");
  return OK;
}

/* Turn off a particular insalloc, also remove from list of active */
/* MIDI notes. Allows for releasing if ip->xtratim > 0. */

void xturnoff(CSOUND *csound, INSDS *ip)  /* turnoff a particular insalloc  */
{                                         /* called by inexclus on ctrl 111 */
    MCHNBLK *chn;

    if (UNLIKELY(ip->relesing))
      return;                             /* already releasing: nothing to do */

    chn = ip->m_chnbp;
    if (chn != NULL) {                    /* if this was a MIDI note */
      INSDS *prvip;
      prvip = chn->kinsptr[ip->m_pitch];  /*    remov from activ lst */
      if (ip->m_sust && chn->ksuscnt)
        chn->ksuscnt--;
      ip->m_sust = 0;                     /* force turnoff even if sustaining */
      if (prvip != NULL) {
        if (prvip == ip)
          chn->kinsptr[ip->m_pitch] = ip->nxtolap;
        else {
          while (prvip != NULL && prvip->nxtolap != ip)
            prvip = prvip->nxtolap;
          if (prvip != NULL)
            prvip->nxtolap = ip->nxtolap;
        }
      }
    }
    /* remove from schedoff chain first if finite duration */
    if (csound->frstoff != NULL && ip->offtim >= 0.0) {
      INSDS *prvip;
      prvip = csound->frstoff;
      if (prvip == ip)
        csound->frstoff = ip->nxtoff;
      else {
        while (prvip != NULL && prvip->nxtoff != ip)
          prvip = prvip->nxtoff;
        if (prvip != NULL)
          prvip->nxtoff = ip->nxtoff;
      }
    }
    /* if extra time needed: schedoff at new time */
    if (ip->xtratim > 0) {
      set_xtratim(csound, ip);
#ifdef BETA
      if (UNLIKELY(csound->oparms->odebug))
        csound->Message(csound, "Calling schedofftim line %d\n", __LINE__);
#endif
      schedofftim(csound, ip);
    }
    else {
      /* no extra time needed: deactivate immediately */
      deact(csound, ip);
      csound->dag_changed++;      /* Need to remake DAG */
      //printf("**** dag changed by xturnoff\n");
    }
}

/* Turn off instrument instance immediately, without releasing. */
/* Removes alloc from list of active MIDI notes. */

void xturnoff_now(CSOUND *csound, INSDS *ip)
{
    ip->xtratim = 0;
    ip->relesing = 0;
    xturnoff(csound, ip);
}

extern void free_instrtxt(CSOUND *csound, INSTRTXT *instrtxt);

void orcompact(CSOUND *csound)          /* free all inactive instr spaces */
{
    INSTRTXT  *txtp;
    INSDS     *ip, *nxtip, *prvip, **prvnxtloc;
    int       cnt = 0;
    for (txtp = &(csound->engineState.instxtanchor);
         txtp != NULL;  txtp = txtp->nxtinstxt) {
      // csound->Message(csound, "txp=%p \n", txtp);
      if ((ip = txtp->instance) != NULL) {        /* if instance exists */

        prvip = NULL;
        prvnxtloc = &txtp->instance;
        do {
          if (!ip->actflg) {
            // csound->Message(csound, "ip=%p \n", ip);
            cnt++;
            if (ip->opcod_iobufs && ip->insno > csound->engineState.maxinsno)
              csound->Free(csound, ip->opcod_iobufs);          /* IV - Nov 10 2002 */
            if (ip->fdchp != NULL)
              fdchclose(csound, ip);
            if (ip->auxchp != NULL)
              auxchfree(csound, ip);
            if ((nxtip = ip->nxtinstance) != NULL)
              nxtip->prvinstance = prvip;
            *prvnxtloc = nxtip;

            csound->Free(csound, (char *)ip);

          }
          else {
            prvip = ip;
            prvnxtloc = &ip->nxtinstance;
          }
        }
        while ((ip = *prvnxtloc) != NULL);
      }

      /* IV - Oct 31 2002 */
      if (!txtp->instance)
        txtp->lst_instance = NULL;              /* find last alloc */
      else {
        ip = txtp->instance;
        while (ip->nxtinstance) ip = ip->nxtinstance;
        txtp->lst_instance = ip;
      }

      txtp->act_instance = NULL;                /* no free instances */
    }
    /* check current items in deadpool to see if they need deleting */
    {
      int i;
      for(i=0; i < csound->dead_instr_no; i++){
        if (csound->dead_instr_pool[i] != NULL) {
          INSDS *active = csound->dead_instr_pool[i]->instance;
          while (active != NULL) {
            if (active->actflg) {
              // add_to_deadpool(csound,csound->dead_instr_pool[i]);
              break;
            }
            active = active->nxtinstance;
          }
          /* no active instances */
          if (active == NULL) {
            free_instrtxt(csound, csound->dead_instr_pool[i]);
            csound->dead_instr_pool[i] = NULL;
          }
        }
      }
    }
    if (UNLIKELY(cnt))
      csound->Message(csound, Str("inactive allocs returned to freespace\n"));
}

void infoff(CSOUND *csound, MYFLT p1)   /* turn off an indef copy of instr p1 */
{                                       /*      called by musmon              */
    INSDS *ip;
    int   insno;

    insno = (int) p1;
    if (LIKELY((ip = (csound->engineState.instrtxtp[insno])->instance) != NULL)) {
      do {
        if (ip->insno == insno          /* if find the insno */
            && ip->actflg               /*      active       */
            && ip->offtim < 0.0         /*  but indef, VL: currently this condition
                                            cannot be removed, as it breaks turning
                                            off extratime instances */
            && ip->p1 == p1) {
          if (UNLIKELY(csound->oparms->odebug))
            csound->Message(csound, "turning off inf copy of instr %d\n",
                            insno);
          xturnoff(csound, ip);
          return;                       /*      turn it off  */
        }
      } while ((ip = ip->nxtinstance) != NULL);
    }
    csound->Message(csound,
                    Str("could not find playing instr %f\n"),
                    p1);
}

int csoundInitError(CSOUND *csound, const char *s, ...)
{
    va_list args;
    INSDS   *ip;
    char    buf[512];

    /* RWD: need this! */
    if (UNLIKELY(csound->ids == NULL)) {
      va_start(args, s);
      csoundErrMsgV(csound, Str("\nINIT ERROR: "), s, args);
      va_end(args);
      csound->LongJmp(csound, 1);
    }
    /* IV - Oct 16 2002: check for subinstr and user opcode */
    ip = csound->ids->insdshead;
    if (ip->opcod_iobufs) {
      OPCODINFO *op = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->opcode_info;
      /* find top level instrument instance */
      do {
        ip = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->parent_ip;
      } while (ip->opcod_iobufs);
      if (op)
        snprintf(buf, 512, Str("INIT ERROR in instr %d (opcode %s): "),
                ip->insno, op->name);
      else
        snprintf(buf, 512, Str("INIT ERROR in instr %d (subinstr %d): "),
                ip->insno, csound->ids->insdshead->insno);
    }
    else
      snprintf(buf, 512, Str("INIT ERROR in instr %d: "), ip->insno);
    va_start(args, s);
    csoundErrMsgV(csound, buf, s, args);
    va_end(args);
    putop(csound, &(csound->ids->optext->t));

    return ++(csound->inerrcnt);
}

int csoundPerfError(CSOUND *csound, INSDS *ip, const char *s, ...)
{
    va_list args;
    char    buf[512];

    if (ip->opcod_iobufs) {
      OPCODINFO *op = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->opcode_info;
      /* find top level instrument instance */
      do {
        ip = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->parent_ip;
      } while (ip->opcod_iobufs);
      if (op)
        snprintf(buf, 512, Str("PERF ERROR in instr %d (opcode %s): "),
                ip->insno, op->name);
      else
        snprintf(buf, 512, Str("PERF ERROR in instr %d (subinstr %d): "),
                ip->insno, ip->insno);
    }
    else
      snprintf(buf, 512, Str("PERF ERROR in instr %d: "), ip->insno);
    va_start(args, s);
    csoundErrMsgV(csound, buf, s, args);
    va_end(args);
    if (ip->pds)
     putop(csound, &(ip->pds->optext->t));
    csoundMessage(csound, Str("   note aborted\n"));
    csound->perferrcnt++;
    xturnoff_now((CSOUND*) csound, ip);       /* rm ins fr actlist */
    return csound->perferrcnt;                /* contin from there */
}

int subinstrset_(CSOUND *csound, SUBINST *p, int instno)
{
    OPDS    *saved_ids = csound->ids;
    INSDS   *saved_curip = csound->curip;
    MYFLT   *flp;
    int     n, init_op, inarg_ofs;
    INSDS  *pip = p->h.insdshead;

    init_op = (p->h.opadr == NULL ? 1 : 0);
    inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
    if (UNLIKELY(instno < 0)) return NOTOK;
    /* IV - Oct 9 2002: need this check */
    if (UNLIKELY(!init_op && p->OUTOCOUNT > csound->nchnls)) {
      return csoundInitError(csound, Str("subinstr: number of output "
                                         "args greater than nchnls"));
    }
    /* IV - Oct 9 2002: copied this code from useropcdset() to fix some bugs */
    if (!(pip->reinitflag | pip->tieflag)) {
      /* get instance */
      if (csound->engineState.instrtxtp[instno]->act_instance == NULL)
        instance(csound, instno);
      p->ip = csound->engineState.instrtxtp[instno]->act_instance;
      csound->engineState.instrtxtp[instno]->act_instance = p->ip->nxtact;
      p->ip->insno = (int16) instno;
      p->ip->actflg++;                  /*    and mark the instr active */
      csound->engineState.instrtxtp[instno]->active++;
      csound->engineState.instrtxtp[instno]->instcnt++;
      p->ip->p1 = (MYFLT) instno;
      p->ip->opcod_iobufs = (void*) &p->buf;
      /* link into deact chain */
      p->ip->subins_deact = saved_curip->subins_deact;
      p->ip->opcod_deact = NULL;
      saved_curip->subins_deact = (void*) p;
      p->parent_ip = p->buf.parent_ip = saved_curip;
    }

    /* set the local ksmps values */
//    if (local_ksmps != CS_KSMPS) {
//        /* this is the case when p->ip->ksmps != p->h.insdshead->ksmps */
//        p->ip->ksmps = local_ksmps;
//        ksmps_scale = CS_KSMPS / local_ksmps;
//        lcurip->onedksmps =  FL(1.0) / (MYFLT) local_ksmps;
//        lcurip->ekr = csound->esr / (MYFLT) local_ksmps;
//        lcurip->onedkr = FL(1.0) / lcurip->ekr;
//        lcurip->kicvt = (MYFLT) FMAXLEN /lcurip->ekr;
//        lcurip->kcounter *= ksmps_scale;
//    } else {
        p->ip->ksmps = CS_KSMPS;
        p->ip->kcounter = CS_KCNT;
        p->ip->ekr = CS_EKR;
        p->ip->onedkr = CS_ONEDKR;
        p->ip->onedksmps = CS_ONEDKSMPS;
        p->ip->kicvt = CS_KICVT;
//    }

    /* copy parameters from this instrument into our subinstrument */
    p->ip->xtratim  = saved_curip->xtratim;
    p->ip->m_sust   = 0;
    p->ip->relesing = saved_curip->relesing;
    p->ip->offbet   = saved_curip->offbet;
    p->ip->offtim   = saved_curip->offtim;
    p->ip->nxtolap  = NULL;
    p->ip->p2       = saved_curip->p2;
    p->ip->p3       = saved_curip->p3;
    p->ip->ksmps = CS_KSMPS;

    /* IV - Oct 31 2002 */
    p->ip->m_chnbp  = saved_curip->m_chnbp;
    p->ip->m_pitch  = saved_curip->m_pitch;
    p->ip->m_veloc  = saved_curip->m_veloc;

    p->ip->ksmps_offset =  saved_curip->ksmps_offset;
    p->ip->ksmps_no_end =  saved_curip->ksmps_no_end;
    p->ip->tieflag = saved_curip->tieflag;
    p->ip->reinitflag = saved_curip->reinitflag;

    /* copy remainder of pfields */
    flp = &p->ip->p3 + 1;
    /* by default all inputs are i-rate mapped to p-fields */
    if (UNLIKELY(p->INOCOUNT >
                 (unsigned int)(csound->engineState.instrtxtp[instno]->pmax + 1)))
      return csoundInitError(csound, Str("subinstr: too many p-fields"));
    for (n = 1; (unsigned int) n < p->INOCOUNT; n++)
      *flp++ = *p->ar[inarg_ofs + n];

    /* allocate memory for a temporary store of spout buffers */
    if (!init_op && !(pip->reinitflag | pip->tieflag))
      csoundAuxAlloc(csound,
                     (int32) csound->nspout * sizeof(MYFLT), &p->saved_spout);

    /* do init pass for this instr */
    csound->curip = p->ip;
    csound->ids = (OPDS *)p->ip;
    p->ip->init_done = 0;
    while ((csound->ids = csound->ids->nxti) != NULL) {
      (*csound->ids->iopadr)(csound, csound->ids);
    }
    p->ip->init_done = 1;

    /* copy length related parameters back to caller instr */
    saved_curip->xtratim = csound->curip->xtratim;
    saved_curip->relesing = csound->curip->relesing;
    saved_curip->offbet = csound->curip->offbet;
    saved_curip->offtim = csound->curip->offtim;
    saved_curip->p3 = csound->curip->p3;

    /* restore globals */
    csound->ids = saved_ids;
    csound->curip = saved_curip;
    return OK;
}

int subinstrset_S(CSOUND *csound, SUBINST *p){
    int instno, init_op, inarg_ofs;
    /* check if we are using subinstrinit or subinstr */
    init_op = (p->h.opadr == NULL ? 1 : 0);
    inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
    instno = strarg2insno(csound, ((STRINGDAT *)p->ar[inarg_ofs])->data, 1);
    return subinstrset_(csound,p,instno);
}


int subinstrset(CSOUND *csound, SUBINST *p){
    int instno, init_op, inarg_ofs;
    /* check if we are using subinstrinit or subinstr */
    init_op = (p->h.opadr == NULL ? 1 : 0);
    inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
    instno = (int) *(p->ar[inarg_ofs]);
    return subinstrset_(csound,p,instno);
}

/* IV - Sep 8 2002: new functions for user defined opcodes (based */
/* on Matt J. Ingalls' subinstruments, but mostly rewritten) */

/*
  UDOs now use the local ksmps stored in lcurip->ksmps
  all the other dependent parameters are calculated in relation to
  this.

  lcurip->ksmps is set to the caller ksmps (CS_KSMPS), unless a new
  local ksmps is used, in which case it is set to that value.
  If local ksmps differs from CS_KSMPS, we set useropcd1() to
  deal with the perf-time code. Otherwise useropcd2() is used.

  For recursive calls when the local ksmps is set to differ from
  the calling instrument ksmps, the top-level call
  will use useropcd1(), whereas all the other recursive calls
  will use useropdc2(), since their local ksmps will be the same
  as the caller.

  Also in case of a local ksmps that differs from the caller,
  the local kcounter value, obtained from the caller is
  scaled to denote the correct kcount in terms of local
  kcycles.

*/
int useropcd1(CSOUND *, UOPCODE*), useropcd2(CSOUND *, UOPCODE*);

int useropcdset(CSOUND *csound, UOPCODE *p)
{
    OPDS         *saved_ids = csound->ids;
    INSDS        *parent_ip = csound->curip, *lcurip;
    INSTRTXT     *tp;
    unsigned int instno;
    unsigned int pcnt;
    unsigned int i, n;
    OPCODINFO    *inm;
    OPCOD_IOBUFS *buf = NULL;
    MYFLT ksmps_scale;
    unsigned int local_ksmps;

    /* default ksmps */
    local_ksmps = CS_KSMPS;
    ksmps_scale = 1;
    /* look up the 'fake' instr number, and opcode name */
    inm = (OPCODINFO*) p->h.optext->t.oentry->useropinfo;
    instno = inm->instno;
    tp = csound->engineState.instrtxtp[instno];
    /* set local ksmps if defined by user */
    n = p->OUTOCOUNT + p->INOCOUNT - 1;

    if (*(p->ar[n]) != FL(0.0)) {
      i = (unsigned int) *(p->ar[n]);
      if (UNLIKELY(i < 1 || i > csound->ksmps ||
                   ((CS_KSMPS / i) * i) != CS_KSMPS)) {
        return csoundInitError(csound, Str("%s: invalid local ksmps value: %d"),
                               inm->name, i);
      }
      local_ksmps = i;
    }

    if (!p->ip) {
      /* search for already allocated, but not active instance */
      /* if none was found, allocate a new instance */

      if (!tp->act_instance)
        instance(csound, instno);
    /* **** COVERITY: note that call to instance fills in structure to
       **** which tp points.  This is a false positive **** */
      lcurip = tp->act_instance;            /* use free intance, and  */
      tp->act_instance = lcurip->nxtact;    /* remove from chain      */
      lcurip->actflg++;                     /*    and mark the instr active */
      tp->active++;
      tp->instcnt++;
      /* link into deact chain */
      lcurip->opcod_deact = parent_ip->opcod_deact;
      lcurip->subins_deact = NULL;
      parent_ip->opcod_deact = (void*) p;
      p->ip = lcurip;
      /* IV - Nov 10 2002: set up pointers to I/O buffers */
      buf = p->buf = (OPCOD_IOBUFS*) lcurip->opcod_iobufs;
      buf->opcode_info = inm;
      /* initialise perf time address lists */
      buf->iobufp_ptrs[0] = buf->iobufp_ptrs[1] = NULL;
      buf->iobufp_ptrs[2] = buf->iobufp_ptrs[3] = NULL;
      buf->iobufp_ptrs[4] = buf->iobufp_ptrs[5] = NULL;
      buf->iobufp_ptrs[6] = buf->iobufp_ptrs[7] = NULL;
      buf->iobufp_ptrs[8] = buf->iobufp_ptrs[9] = NULL;
      buf->iobufp_ptrs[10] = buf->iobufp_ptrs[11] = NULL;

      /* store parameters of input and output channels, and parent ip */
      buf->uopcode_struct = (void*) p;
      buf->parent_ip = p->parent_ip = parent_ip;

    }

   /* copy parameters from the caller instrument into our subinstrument */
    lcurip = p->ip;

    /* set the local ksmps values */
    if (local_ksmps != CS_KSMPS) {
      /* this is the case when p->ip->ksmps != p->h.insdshead->ksmps */
      lcurip->ksmps = local_ksmps;
      ksmps_scale = CS_KSMPS / local_ksmps;
      lcurip->onedksmps =  FL(1.0) / (MYFLT) local_ksmps;
      lcurip->ekr = csound->esr / (MYFLT) local_ksmps;
      lcurip->onedkr = FL(1.0) / lcurip->ekr;
      lcurip->kicvt = (MYFLT) FMAXLEN /lcurip->ekr;
      lcurip->kcounter *= ksmps_scale;
    } else {
      lcurip->ksmps = CS_KSMPS;
      lcurip->kcounter = CS_KCNT;
      lcurip->ekr = CS_EKR;
      lcurip->onedkr = CS_ONEDKR;
      lcurip->onedksmps = CS_ONEDKSMPS;
      lcurip->kicvt = CS_KICVT;
    }

    /* VL 13-12-13 */
    /* this sets ksmps and kr local variables */
    /* create local ksmps variable and init with ksmps */
    if (lcurip->lclbas != NULL) {
    CS_VARIABLE *var =
       csoundFindVariableWithName(csound, lcurip->instr->varPool, "ksmps");
       *((MYFLT *)(var->memBlockIndex + lcurip->lclbas)) = lcurip->ksmps;
     /* same for kr */
      var =
       csoundFindVariableWithName(csound, lcurip->instr->varPool, "kr");
     *((MYFLT *)(var->memBlockIndex + lcurip->lclbas)) = lcurip->ekr;
    }

    lcurip->m_chnbp = parent_ip->m_chnbp;       /* MIDI parameters */
    lcurip->m_pitch = parent_ip->m_pitch;
    lcurip->m_veloc = parent_ip->m_veloc;
    lcurip->xtratim = parent_ip->xtratim * ksmps_scale;
    lcurip->m_sust = 0;
    lcurip->relesing = parent_ip->relesing;
    lcurip->offbet = parent_ip->offbet;
    lcurip->offtim = parent_ip->offtim;
    lcurip->nxtolap = NULL;
    lcurip->ksmps_offset = parent_ip->ksmps_offset;
    lcurip->ksmps_no_end = parent_ip->ksmps_no_end;
    lcurip->tieflag = parent_ip->tieflag;
    lcurip->reinitflag = parent_ip->reinitflag;
    /* copy all p-fields, including p1 (will this work ?) */
    if (tp->pmax > 3) {         /* requested number of p-fields */
      n = tp->pmax; pcnt = 0;
      while (pcnt < n) {
        if ((i = csound->engineState.instrtxtp[parent_ip->insno]->pmax) > pcnt) {
          if (i > n) i = n;
          /* copy next block of p-fields */
          memcpy(&(lcurip->p1) + pcnt, &(parent_ip->p1) + pcnt,
                 (size_t) ((i - pcnt) * sizeof(MYFLT)));
          pcnt = i;
        }
        /* top level instr reached */
        if (parent_ip->opcod_iobufs == NULL) break;
        parent_ip = ((OPCOD_IOBUFS*) parent_ip->opcod_iobufs)->parent_ip;
      }
    }
    else
      memcpy(&(lcurip->p1), &(parent_ip->p1), 3 * sizeof(MYFLT));

    /* do init pass for this instr */
    p->ip->init_done = 0;
    csound->curip = lcurip;
    csound->ids = (OPDS *) (lcurip->nxti);
    while (csound->ids != NULL) {
      (*csound->ids->iopadr)(csound, csound->ids);
      csound->ids = csound->ids->nxti;
      }
    p->ip->init_done = 1;

    /* copy length related parameters back to caller instr */
    parent_ip->relesing = lcurip->relesing;
    parent_ip->offbet = lcurip->offbet;
    parent_ip->offtim = lcurip->offtim;
    parent_ip->p3 = lcurip->p3;
    local_ksmps = lcurip->ksmps;

    /* restore globals */
    csound->ids = saved_ids;
    csound->curip = parent_ip;

    /* select perf routine and scale xtratim accordingly */
    if (local_ksmps != CS_KSMPS) {
       ksmps_scale = CS_KSMPS / local_ksmps;
       parent_ip->xtratim = lcurip->xtratim / ksmps_scale;
      p->h.opadr = (SUBR) useropcd1;
    }
    else {
      parent_ip->xtratim = lcurip->xtratim;
      p->h.opadr = (SUBR) useropcd2;
    }
    if (UNLIKELY(csound->oparms->odebug))
    csound->Message(csound, "EXTRATIM=> cur(%p): %d, parent(%p): %d\n",
            lcurip, lcurip->xtratim, parent_ip, parent_ip->xtratim);
    return OK;
}

/* IV - Sep 17 2002: dummy user opcode function for not initialised case */

int useropcd(CSOUND *csound, UOPCODE *p)
{

    if (UNLIKELY(p->h.nxtp))
      return csoundPerfError(csound, p->h.insdshead, Str("%s: not initialised"),
                             p->h.optext->t.opcod);
    else
      return OK;
}

/* IV - Sep 1 2002: new opcodes: xin, xout */

int xinset(CSOUND *csound, XIN *p)
{
    OPCOD_IOBUFS  *buf;
    OPCODINFO   *inm;
    MYFLT **bufs, **tmp;
    int i;
    CS_VARIABLE* current;

    (void) csound;
    buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
    inm = buf->opcode_info;
    bufs = ((UOPCODE*) buf->uopcode_struct)->ar + inm->outchns;
    tmp = buf->iobufp_ptrs; // this is used to record the UDO's internal vars
                            // for copying at perf-time
    current = inm->in_arg_pool->head;

    for (i = 0; i < inm->inchns; i++) {
      void* in = (void*)bufs[i];
      void* out = (void*)p->args[i];
      tmp[i + inm->outchns] = out;
      current->varType->copyValue(csound, out, in);
      current = current->next;
    }

    return OK;
}

int xoutset(CSOUND *csound, XOUT *p)
{
    OPCOD_IOBUFS  *buf;
    OPCODINFO   *inm;
    MYFLT       **bufs, **tmp;
    CS_VARIABLE* current;
    int i;

    (void) csound;
    buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
    inm = buf->opcode_info;
    bufs = ((UOPCODE*) buf->uopcode_struct)->ar;
    tmp = buf->iobufp_ptrs; // this is used to record the UDO's internal vars
                            // for copying at perf-time
    current = inm->out_arg_pool->head;

    for (i = 0; i < inm->outchns; i++) {
      void* in = (void*)p->args[i];
      void* out = (void*)bufs[i];
      tmp[i] = in;
      current->varType->copyValue(csound, out, in);
      current = current->next;
    }

    return OK;
}

/* IV - Sep 8 2002: new opcode: setksmps */

/*
  This opcode sets the local ksmps for an instrument
  it can be used on any instrument with the implementation
  of a mechanism to perform at local ksmps (in kperf etc)
*/
//#include "typetabl.h"
#include "csound_standard_types.h"
int setksmpsset(CSOUND *csound, SETKSMPS *p)
{

    unsigned int  l_ksmps, n;

    l_ksmps = (unsigned int) *(p->i_ksmps);
    if (!l_ksmps) return OK;       /* zero: do not change */
    if (UNLIKELY(l_ksmps < 1 || l_ksmps > CS_KSMPS ||
                 ((CS_KSMPS / l_ksmps) * l_ksmps != CS_KSMPS))) {
      return csoundInitError(csound,
                             Str("setksmps: invalid ksmps value: %d, original: %d"),
                             l_ksmps, CS_KSMPS);
    }

    n = CS_KSMPS / l_ksmps;
    p->h.insdshead->xtratim *= n;
    CS_KSMPS = l_ksmps;
    CS_ONEDKSMPS = FL(1.0) / (MYFLT) CS_KSMPS;
    CS_EKR = csound->esr / (MYFLT) CS_KSMPS;
    CS_ONEDKR = FL(1.0) / CS_EKR;
    CS_KICVT = (MYFLT) FMAXLEN / CS_EKR;
    CS_KCNT *= n;

    /* VL 13-12-13 */
    /* this sets ksmps and kr local variables */
    /* lookup local ksmps variable and init with ksmps */
     INSTRTXT *ip = p->h.insdshead->instr;
     CS_VARIABLE *var =
       csoundFindVariableWithName(csound, ip->varPool, "ksmps");
     MYFLT *varmem = p->h.insdshead->lclbas + var->memBlockIndex;
     *varmem = CS_KSMPS;

     /* same for kr */
     var =
       csoundFindVariableWithName(csound, ip->varPool, "kr");
     varmem = p->h.insdshead->lclbas + var->memBlockIndex;
     *varmem = CS_EKR;

    return OK;
}

/* IV - Oct 16 2002: nstrnum opcode (returns the instrument number of a */
/* named instrument) */

int nstrnumset(CSOUND *csound, NSTRNUM *p)
{
    /* IV - Oct 31 2002 */
    *(p->i_insno) = (MYFLT) strarg2insno(csound, p->iname, 0);
    return (*(p->i_insno) > FL(0.0) ? OK : NOTOK);
}

int nstrnumset_S(CSOUND *csound, NSTRNUM *p)
{
    /* IV - Oct 31 2002 */
    *(p->i_insno) = (MYFLT) strarg2insno(csound,
                                         ((STRINGDAT *)p->iname)->data, 1);
    return (*(p->i_insno) > FL(0.0) ? OK : NOTOK);
}


/* unlink expired notes from activ chain */
/*      and mark them inactive           */
/*    close any files in each fdchain    */

/* IV - Feb 05 2005: changed to double */

void beatexpire(CSOUND *csound, double beat)
{
    INSDS  *ip;
 strt:
    if ((ip = csound->frstoff) != NULL && ip->offbet <= beat) {
      do {
        if (!ip->relesing && ip->xtratim) {
          /* IV - Nov 30 2002: */
          /*   allow extra time for finite length (p3 > 0) score notes */
          set_xtratim(csound, ip);      /* enter release stage */
          csound->frstoff = ip->nxtoff; /* update turnoff list */
#ifdef BETA
          if (UNLIKELY(csound->oparms->odebug))
            csound->Message(csound, "Calling schedofftim line %d\n", __LINE__);
#endif
          schedofftim(csound, ip);
          goto strt;                    /* and start again */
        }
        else
          deact(csound, ip);    /* IV - Sep 5 2002: use deact() as it also */
      }                         /* deactivates subinstrument instances */
      while ((ip = ip->nxtoff) != NULL && ip->offbet <= beat);
      csound->frstoff = ip;
      if (UNLIKELY(csound->oparms->odebug)) {
        csound->Message(csound, "deactivated all notes to beat %7.3f\n", beat);
        csound->Message(csound, "frstoff = %p\n", (void*) csound->frstoff);
      }
    }
}

/* unlink expired notes from activ chain */
/*      and mark them inactive           */
/*    close any files in each fdchain    */

/* IV - Feb 05 2005: changed to double */

void timexpire(CSOUND *csound, double time)
{
    INSDS  *ip;

 strt:
    if ((ip = csound->frstoff) != NULL && ip->offtim <= time) {
      do {
        if (!ip->relesing && ip->xtratim) {
          /* IV - Nov 30 2002: */
          /*   allow extra time for finite length (p3 > 0) score notes */
          set_xtratim(csound, ip);      /* enter release stage */
          csound->frstoff = ip->nxtoff; /* update turnoff list */
#ifdef BETA
          if (UNLIKELY(csound->oparms->odebug))
            csound->Message(csound, "Calling schedofftim line %d\n", __LINE__);
#endif
          schedofftim(csound, ip);
          goto strt;                    /* and start again */
        }
        else
          deact(csound, ip);    /* IV - Sep 5 2002: use deact() as it also */
      }                         /* deactivates subinstrument instances */
      while ((ip = ip->nxtoff) != NULL && ip->offtim <= time);
      csound->frstoff = ip;
      if (UNLIKELY(csound->oparms->odebug)) {
        csound->Message(csound, "deactivated all notes to time %7.3f\n", time);
        csound->Message(csound, "frstoff = %p\n", (void*) csound->frstoff);
      }
    }
}

/**
   this was rewritten for Csound 6 to allow
   PARCS and local ksmps instruments
*/

int subinstr(CSOUND *csound, SUBINST *p)
{
    OPDS    *saved_pds = CS_PDS;
    MYFLT   *pbuf;
    uint32_t frame, chan;
    unsigned int nsmps = CS_KSMPS;
    INSDS *ip = p->ip;

    if (UNLIKELY(p->ip == NULL)) {                /* IV - Oct 26 2002 */
      return csoundPerfError(csound, p->h.insdshead,
                             Str("subinstr: not initialised"));
    }
    /* copy current spout buffer and clear it */
    ip->spout = (MYFLT*) p->saved_spout.auxp;
    memset(ip->spout, 0, csound->nspout*sizeof(MYFLT));
    csound->spoutactive = 0;

    /* update release flag */
    ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */

    /*  run each opcode  */
    if (csound->ksmps == ip->ksmps) {
      if ((CS_PDS = (OPDS *) (ip->nxtp)) != NULL) {
        CS_PDS->insdshead->pds = NULL;
        do {
          (*CS_PDS->opadr)(csound, CS_PDS);
          if (CS_PDS->insdshead->pds != NULL) {
            CS_PDS = CS_PDS->insdshead->pds;
            CS_PDS->insdshead->pds = NULL;
          }
        }while ((CS_PDS = CS_PDS->nxtp));
      }
      ip->kcounter++;
    }
    else {
      int i, n = csound->nspout, start = 0;
      int lksmps = ip->ksmps;
      int incr = csound->nchnls*lksmps;
      int offset =  ip->ksmps_offset;
      int early = ip->ksmps_no_end;
      ip->spin = csound->spin;
      ip->kcounter =  csound->kcounter*csound->ksmps/lksmps;

      /* we have to deal with sample-accurate code
         whole CS_KSMPS blocks are offset here, the
         remainder is left to each opcode to deal with.
      */
      while (offset >= lksmps) {
        offset -= lksmps;
        start += csound->nchnls;
      }
      ip->ksmps_offset = offset;
      if (early){
        n -= (early*csound->nchnls);
        ip->ksmps_no_end = early % lksmps;
      }

      for (i=start; i < n; i+=incr, ip->spin+=incr, ip->spout+=incr) {
        if ((CS_PDS = (OPDS *) (ip->nxtp)) != NULL) {
          CS_PDS->insdshead->pds = NULL;
          do {
            (*CS_PDS->opadr)(csound, CS_PDS);
            if (CS_PDS->insdshead->pds != NULL) {
              CS_PDS = CS_PDS->insdshead->pds;
              CS_PDS->insdshead->pds = NULL;
            }
          }while ((CS_PDS = CS_PDS->nxtp));
        }
        ip->kcounter++;
      }
      ip->spout = (MYFLT*) p->saved_spout.auxp;
    }
    /* copy outputs */
    for (chan = 0; chan < p->OUTOCOUNT; chan++) {
      for (pbuf = ip->spout + chan, frame = 0;
           frame < nsmps; frame++) {
        p->ar[chan][frame] = *pbuf;
        pbuf += csound->nchnls;
      }
    }

    CS_PDS = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip)                                         /* loop to last opds */
      while (CS_PDS->nxtp)CS_PDS = CS_PDS->nxtp;
    return OK;
}

/* IV - Sep 17 2002 -- case 1: local ksmps is used */

int useropcd1(CSOUND *csound, UOPCODE *p)
{
    OPDS    *saved_pds = CS_PDS;
    int    g_ksmps, ofs, early, offset, i;
    OPCODINFO   *inm;
    CS_VARIABLE* current;
    INSDS    *this_instr = p->ip;
    MYFLT** internal_ptrs = p->buf->iobufp_ptrs;
    MYFLT** external_ptrs = p->ar;

    p->ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */
    early = p->h.insdshead->ksmps_no_end;
    offset = p->h.insdshead->ksmps_offset;
    this_instr->spin = csound->spin;
    this_instr->spout = csound->spout;
    inm = p->buf->opcode_info;

    /* global ksmps is the caller instr ksmps minus sample-accurate end */
    g_ksmps = CS_KSMPS - early;

    /* sample-accurate offset */
    ofs = offset;

    /* clear offsets, since with CS_KSMPS=1
       they don't apply to opcodes, but to the
       calling code (ie. this code)
    */
    this_instr->ksmps_offset = 0;
    this_instr->ksmps_no_end = 0;


    /* copy inputs */
    current = inm->in_arg_pool->head;
    for (i = 0; i < inm->inchns; i++) {
      // this hardcoded type check for non-perf time vars needs to change
      //to use generic code...
      // skip a-vars for now, handle uniquely within performance loop
      if (current->varType != &CS_VAR_TYPE_I &&
          current->varType != &CS_VAR_TYPE_b &&
          current->varType != &CS_VAR_TYPE_A &&
          current->subType != &CS_VAR_TYPE_I &&
          current->subType != &CS_VAR_TYPE_A) {
        // This one checks if an array has a subtype of 'i'
        void* in = (void*)external_ptrs[i + inm->outchns];
        void* out = (void*)internal_ptrs[i + inm->outchns];
        current->varType->copyValue(csound, out, in);
      }
      current = current->next;
    }

    if (this_instr->ksmps == 1) {           /* special case for local kr == sr */
      do {

        /* copy a-sig inputs, accounting for offset */
        current = inm->in_arg_pool->head;
        for (i = 0; i < inm->inchns; i++) {
          if (current->varType == &CS_VAR_TYPE_A) {
            MYFLT* in = (void*)external_ptrs[i + inm->outchns];
            MYFLT* out = (void*)internal_ptrs[i + inm->outchns];
            *out = *(in + ofs);
          } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                     current->subType == &CS_VAR_TYPE_A) {
            ARRAYDAT* src = (ARRAYDAT*)external_ptrs[i + inm->outchns];
            ARRAYDAT* target = (ARRAYDAT*)internal_ptrs[i + inm->outchns];
            int count = src->sizes[0];
            int j;
            if(src->dimensions > 1) {
                for (j = 0; j < src->dimensions; j++) {
                    count *= src->sizes[j];
                }
            }

            for (j = 0; j < count; j++) {
                int memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
              MYFLT* in = src->data + memberOffset;
              MYFLT* out = target->data + memberOffset;
              *out = *(in + ofs);
            }
          }
          current = current->next;
        }

        if ((CS_PDS = (OPDS *) (this_instr->nxtp)) != NULL) {
          CS_PDS->insdshead->pds = NULL;
          do {
            (*CS_PDS->opadr)(csound, CS_PDS);
            if (CS_PDS->insdshead->pds != NULL &&
                CS_PDS->insdshead->pds->insdshead) {
              CS_PDS = CS_PDS->insdshead->pds;
              CS_PDS->insdshead->pds = NULL;
            }
          } while ((CS_PDS = CS_PDS->nxtp));
        }

        /* copy a-sig outputs, accounting for offset */
        current = inm->out_arg_pool->head;
        for (i = 0; i < inm->outchns; i++) {
          if (current->varType == &CS_VAR_TYPE_A) {
            MYFLT* in = (void*)internal_ptrs[i];
            MYFLT* out = (void*)external_ptrs[i];
            *(out + ofs) = *in;
          } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                     current->subType == &CS_VAR_TYPE_A) {
            ARRAYDAT* src = (ARRAYDAT*)internal_ptrs[i];
            ARRAYDAT* target = (ARRAYDAT*)external_ptrs[i];
            int count = src->sizes[0];
            int j;
            if(src->dimensions > 1) {
              for (j = 0; j < src->dimensions; j++) {
                count *= src->sizes[j];
              }
            }

            for (j = 0; j < count; j++) {
              int memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
              MYFLT* in = src->data + memberOffset;
              MYFLT* out = target->data + memberOffset;
              *(out + ofs) = *in;
            }
          }

          current = current->next;
        }


        this_instr->kcounter++;
        this_instr->spout += csound->nchnls;
        this_instr->spin  += csound->nchnls;
      } while (++ofs < g_ksmps);
    }
    else {
      /* generic case for local kr != sr */
      /* we have to deal with sample-accurate code
         whole CS_KSMPS blocks are offset here, the
         remainder is left to each opcode to deal with.
      */
      int start = 0;
      int lksmps = this_instr->ksmps;
      while (ofs >= lksmps) {
        ofs -= lksmps;
        start++;
      }
      this_instr->ksmps_offset = ofs;
      ofs = start;
      if (UNLIKELY(early)) this_instr->ksmps_no_end = early % lksmps;

      do {
        /* copy a-sig inputs, accounting for offset */
        size_t asigSize = (this_instr->ksmps * sizeof(MYFLT));
        current = inm->in_arg_pool->head;
        for (i = 0; i < inm->inchns; i++) {
          if (current->varType == &CS_VAR_TYPE_A) {
            MYFLT* in = (void*)external_ptrs[i + inm->outchns];
            MYFLT* out = (void*)internal_ptrs[i + inm->outchns];
            memcpy(out, in + ofs, asigSize);
          } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                     current->subType == &CS_VAR_TYPE_A) {
            ARRAYDAT* src = (ARRAYDAT*)external_ptrs[i + inm->outchns];
            ARRAYDAT* target = (ARRAYDAT*)internal_ptrs[i + inm->outchns];
            int count = src->sizes[0];
            int j;
            if(src->dimensions > 1) {
                for (j = 0; j < src->dimensions; j++) {
                    count *= src->sizes[j];
                }
            }

            for (j = 0; j < count; j++) {
                int memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
              MYFLT* in = src->data + memberOffset;
              MYFLT* out = target->data + memberOffset;
              memcpy(out, in + ofs, asigSize);
            }
          }
          current = current->next;
        }

        /*  run each opcode  */
        if ((CS_PDS = (OPDS *) (this_instr->nxtp)) != NULL) {
          CS_PDS->insdshead->pds = NULL;
          do {
            (*CS_PDS->opadr)(csound, CS_PDS);
            if (CS_PDS->insdshead->pds != NULL &&
                CS_PDS->insdshead->pds->insdshead) {
              CS_PDS = CS_PDS->insdshead->pds;
              CS_PDS->insdshead->pds = NULL;
            }
          }while ((CS_PDS = CS_PDS->nxtp));
        }

        /* copy a-sig outputs, accounting for offset */
        current = inm->out_arg_pool->head;
        for (i = 0; i < inm->outchns; i++) {
          if (current->varType == &CS_VAR_TYPE_A) {
            MYFLT* in = (void*)internal_ptrs[i];
            MYFLT* out = (void*)external_ptrs[i];
            memcpy(out + ofs, in, asigSize);
          } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                     current->subType == &CS_VAR_TYPE_A) {
              ARRAYDAT* src = (ARRAYDAT*)internal_ptrs[i];
              ARRAYDAT* target = (ARRAYDAT*)external_ptrs[i];
              int count = src->sizes[0];
              int j;
              if(src->dimensions > 1) {
                  for (j = 0; j < src->dimensions; j++) {
                      count *= src->sizes[j];
                  }
              }

              for (j = 0; j < count; j++) {
                  int memberOffset = j * (src->arrayMemberSize / sizeof(MYFLT));
                  MYFLT* in = src->data + memberOffset;
                  MYFLT* out = target->data + memberOffset;
                  memcpy(out + ofs, in, asigSize);
              }

          }

          current = current->next;
        }

        this_instr->spout += csound->nchnls*lksmps;
        this_instr->spin  += csound->nchnls*lksmps;
        this_instr->kcounter++;
      } while ((ofs += this_instr->ksmps) < g_ksmps);
    }


    /* copy outputs */
    current = inm->out_arg_pool->head;
    for (i = 0; i < inm->outchns; i++) {
      // this hardcoded type check for non-perf time vars needs to change
      // to use generic code...
      if (current->varType != &CS_VAR_TYPE_I &&
          current->varType != &CS_VAR_TYPE_b &&
          current->subType != &CS_VAR_TYPE_I) {
        void* in = (void*)internal_ptrs[i];
        void* out = (void*)external_ptrs[i];

        if (current->varType == &CS_VAR_TYPE_A) {
          /* clear the beginning portion of outputs for sample accurate end */
          if (offset) {
            memset(out, '\0', sizeof(MYFLT) * offset);
          }

          /* clear the end portion of outputs for sample accurate end */
          if (early) {
            memset(out + g_ksmps, '\0', sizeof(MYFLT) * early);
          }
        } else if (current->varType == &CS_VAR_TYPE_ARRAY &&
                   current->subType == &CS_VAR_TYPE_A) {
          if (offset || early) {
            ARRAYDAT* outDat = (ARRAYDAT*)out;
            int count = outDat->sizes[0];
            int j;
            if(outDat->dimensions > 1) {
                for (j = 0; j < outDat->dimensions; j++) {
                    count *= outDat->sizes[j];
                }
            }

            if(offset) {
              for (j = 0; j < count; j++) {
                int memberOffset = j * (outDat->arrayMemberSize / sizeof(MYFLT));
                MYFLT* outMem = outDat->data + memberOffset;
                memset(outMem, '\0', sizeof(MYFLT) * offset);
              }
            }

            if(early) {
              for (j = 0; j < count; j++) {
                int memberOffset = j * (outDat->arrayMemberSize / sizeof(MYFLT));
                MYFLT* outMem = outDat->data + memberOffset;
                memset(outMem + g_ksmps, '\0', sizeof(MYFLT) * early);
              }
            }
          }

        } else {
          current->varType->copyValue(csound, out, in);
        }
      }
      current = current->next;
    }

    CS_PDS = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip)                                         /* loop to last opds */
      while (CS_PDS->nxtp) CS_PDS = CS_PDS->nxtp;
    return OK;
}

/* IV - Sep 17 2002 -- case 2: simplified routine for no local ksmps */

int useropcd2(CSOUND *csound, UOPCODE *p)
{
    OPDS    *saved_pds = CS_PDS;
    MYFLT   **tmp;
    INSDS    *this_instr = p->ip;
    OPCODINFO   *inm;
    CS_VARIABLE* current;
    int i;

    p->ip->spin = csound->spin;
    p->ip->spout = csound->spout;

    if (UNLIKELY(!(CS_PDS = (OPDS*) (p->ip->nxtp))))
      goto endop; /* no perf code */

    /* IV - Nov 16 2002: update release flag */
    p->ip->relesing = p->parent_ip->relesing;
    tmp = p->buf->iobufp_ptrs;
    inm = p->buf->opcode_info;

    MYFLT** internal_ptrs = tmp;
    MYFLT** external_ptrs = p->ar;

    /* copy inputs */
    current = inm->in_arg_pool->head;
    for (i = 0; i < inm->inchns; i++) {
      // this hardcoded type check for non-perf time vars needs to
      //change to use generic code...
      if (current->varType != &CS_VAR_TYPE_I &&
          current->varType != &CS_VAR_TYPE_b &&
          current->subType != &CS_VAR_TYPE_I) {
        if (current->varType == &CS_VAR_TYPE_A && CS_KSMPS == 1) {
          *internal_ptrs[i + inm->outchns] = *external_ptrs[i + inm->outchns];
        } else {
          void* in = (void*)external_ptrs[i + inm->outchns];
          void* out = (void*)internal_ptrs[i + inm->outchns];
          current->varType->copyValue(csound, out, in);
          //                memcpy(out, in, p->buf->in_arg_sizes[i]);
        }
      }
      current = current->next;
      }

    /*  run each opcode  */
    CS_PDS->insdshead->pds = NULL;
    do {
      (*CS_PDS->opadr)(csound, CS_PDS);
      if (CS_PDS->insdshead->pds != NULL &&
          CS_PDS->insdshead->pds->insdshead) {
        CS_PDS = CS_PDS->insdshead->pds;
        CS_PDS->insdshead->pds = NULL;
      }
    } while ((CS_PDS = CS_PDS->nxtp));

    this_instr->kcounter++;

    /* copy outputs */
    current = inm->out_arg_pool->head;
    for (i = 0; i < inm->outchns; i++) {
      // this hardcoded type check for non-perf time vars needs to change to
      // use generic code...
      if (current->varType != &CS_VAR_TYPE_I &&
          current->varType != &CS_VAR_TYPE_b &&
          current->subType != &CS_VAR_TYPE_I) {
        if (current->varType == &CS_VAR_TYPE_A && CS_KSMPS == 1) {
          *external_ptrs[i] = *internal_ptrs[i];
        } else {
          void* in = (void*)internal_ptrs[i];
          void* out = (void*)external_ptrs[i];
//            memcpy(out, in, p->buf->out_arg_sizes[i]);
          current->varType->copyValue(csound, out, in);
        }
      }
      current = current->next;
    }

 endop:
    /* restore globals */
    CS_PDS = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip) {                                    /* loop to last opds */
      while (CS_PDS->nxtp) CS_PDS = CS_PDS->nxtp;
    }
    return OK;
}

/* UTILITY FUNCTIONS FOR LABELS */

int findLabelMemOffset(CSOUND* csound, INSTRTXT* ip, char* labelName) {
    OPTXT* optxt = (OPTXT*) ip;
    int offset = 0;

    while ((optxt = optxt->nxtop) != NULL) {
      TEXT* t = &optxt->t;
      if (strcmp(t->oentry->opname, "$label") == 0 &&
          strcmp(t->opcod, labelName) == 0) {
        break;
      }
      offset += t->oentry->dsblksiz;
    }

    return offset;
}

/* create instance of an instr template */
/*   allocates and sets up all pntrs    */

static void instance(CSOUND *csound, int insno)
{
    INSTRTXT  *tp;
    INSDS     *ip;
    OPTXT     *optxt;
    OPDS      *opds, *prvids, *prvpds;
    const OENTRY  *ep;
    int       n, /*cnt, */pextent, pextra;
    char      *nxtopds, *opdslim;
    MYFLT     **argpp, *lclbas, /* *gbloffbas,*/ *lcloffbas;
    char*     opMemStart;

    OPARMS    *O = csound->oparms;
    int       odebug = O->odebug;
    ARG*      arg;
    int       argStringCount;

    tp = csound->engineState.instrtxtp[insno];
    n = 3;
    if (O->midiKey>n) n = O->midiKey;
    if (O->midiKeyCps>n) n = O->midiKeyCps;
    if (O->midiKeyOct>n) n = O->midiKeyOct;
    if (O->midiKeyPch>n) n = O->midiKeyPch;
    if (O->midiVelocity>n) n = O->midiVelocity;
    if (O->midiVelocityAmp>n) n = O->midiVelocityAmp;
    pextra = n-3;
    /* alloc new space,  */
    pextent = sizeof(INSDS) + tp->pextrab + pextra*sizeof(MYFLT *);
    ip = (INSDS*) csound->Calloc(csound,
                          (size_t) pextent + tp->varPool->poolSize + tp->opdstot);
    ip->csound = csound;
    ip->m_chnbp = (MCHNBLK*) NULL;
    ip->instr = tp;
    /* IV - Oct 26 2002: replaced with faster version (no search) */
    ip->prvinstance = tp->lst_instance;
    if (tp->lst_instance)
      tp->lst_instance->nxtinstance = ip;
    else
      tp->instance = ip;
    tp->lst_instance = ip;
    /* link into free instance chain */
    ip->nxtact = tp->act_instance;
    tp->act_instance = ip;
    ip->insno = insno;
    if (UNLIKELY(csound->oparms->odebug))
      csoundMessage(csound,"instance(): tp->act_instance = %p \n", tp->act_instance);


    if (insno > csound->engineState.maxinsno) {
//      size_t pcnt = (size_t) tp->opcode_info->perf_incnt;
//      pcnt += (size_t) tp->opcode_info->perf_outcnt;
        OPCODINFO* info = tp->opcode_info;
      size_t pcnt = sizeof(OPCOD_IOBUFS) +
                    sizeof(MYFLT*) * (info->inchns + info->outchns);
      ip->opcod_iobufs = (void*) csound->Malloc(csound, pcnt);
    }

    /* gbloffbas = csound->globalVarPool; */
    lcloffbas = &ip->p0;
    lclbas = (MYFLT*) ((char*) ip + pextent);   /* split local space */
    initializeVarPool(lclbas, tp->varPool);

    opMemStart = nxtopds = (char*) lclbas + tp->varPool->poolSize;
    opdslim = nxtopds + tp->opdstot;
    if (UNLIKELY(odebug))
      csound->Message(csound,
                      Str("instr %d allocated at %p\n\tlclbas %p, opds %p\n"),
                      insno, ip, lclbas, nxtopds);
    optxt = (OPTXT*) tp;
    prvids = prvpds = (OPDS*) ip;
    while ((optxt = optxt->nxtop) != NULL) {    /* for each op in instr */
      TEXT *ttp = &optxt->t;
      ep = ttp->oentry;
      opds = (OPDS*) nxtopds;                   /*   take reqd opds */
      nxtopds += ep->dsblksiz;
      if (UNLIKELY(strcmp(ep->opname, "endin") == 0         /*  (until ENDIN)  */
                   || strcmp(ep->opname, "endop") == 0))    /*  (or ENDOP)     */
        break;

      if (UNLIKELY(strcmp(ep->opname, "pset") == 0)) {
        ip->p1 = (MYFLT) insno;
        continue;
      }
      if (UNLIKELY(odebug))
        csound->Message(csound, Str("op (%s) allocated at %p\n"),
                        ep->opname, opds);
      opds->optext = optxt;                     /* set common headata */
      opds->insdshead = ip;
      if (strcmp(ep->opname, "$label") == 0) {     /* LABEL:       */
        LBLBLK  *lblbp = (LBLBLK *) opds;
        lblbp->prvi = prvids;                   /*    save i/p links */
        lblbp->prvp = prvpds;
        continue;                               /*    for later refs */
      }
      if ((ep->thread & 07) == 0) {             /* thread 1 OR 2:  */
        if (ttp->pftype == 'b') {
          prvids = prvids->nxti = opds;
          opds->iopadr = ep->iopadr;
        }
        else {
          prvpds = prvpds->nxtp = opds;
          opds->opadr = ep->kopadr;
        }
        goto args;
      }
      if ((ep->thread & 01) != 0) {             /* thread 1:        */
        prvids = prvids->nxti = opds;           /* link into ichain */
        opds->iopadr = ep->iopadr;              /*   & set exec adr */
        if (UNLIKELY(opds->iopadr == NULL))
          csoundDie(csound, Str("null iopadr"));
      }
      if ((n = ep->thread & 06) != 0) {         /* thread 2 OR 4:   */
        prvpds = prvpds->nxtp = opds;           /* link into pchain */
        if (!(n & 04) ||
            ((ttp->pftype == 'k' || ttp->pftype == 'c') && ep->kopadr != NULL))
          opds->opadr = ep->kopadr;             /*      krate or    */
        else opds->opadr = ep->aopadr;          /*      arate       */
        if (UNLIKELY(odebug))
          csound->Message(csound, "opadr = %p\n", (void*) opds->opadr);
        if (UNLIKELY(opds->opadr == NULL))
          csoundDie(csound, Str("null opadr"));
      }
    args:
      if (ep->useropinfo == NULL)
        argpp = (MYFLT **) ((char *) opds + sizeof(OPDS));
      else          /* user defined opcodes are a special case */
        argpp = &(((UOPCODE *) ((char *) opds))->ar[0]);

      arg = ttp->outArgs;
      for (n = 0; arg != NULL; n++) {
        MYFLT *fltp;
        CS_VARIABLE* var = (CS_VARIABLE*)arg->argPtr;
        if (arg->type == ARG_GLOBAL) {
          fltp = (MYFLT *) var->memBlock; /* gbloffbas + var->memBlockIndex; */
        }
        else if (arg->type == ARG_LOCAL) {
          fltp = lclbas + var->memBlockIndex;
        }
        else if (arg->type == ARG_PFIELD) {
          /* VL 1.1.13 - changed lclbas to
             lcloffbas so p-fields can be assigned to */
          fltp = lcloffbas + arg->index;
        }
        else {
          csound->Message(csound, "FIXME: Unhandled out-arg type: %d\n",
                          arg->type);
          fltp = NULL;
        }
        argpp[n] = fltp;
        arg = arg->next;
      }

      for (argStringCount = argsRequired(ep->outypes);
           n < argStringCount;
           n++)  /* if more outypes, pad */
        argpp[n] = NULL;

      arg = ttp->inArgs;
      ip->lclbas = lclbas;
      for (; arg != NULL; n++, arg = arg->next) {
        CS_VARIABLE* var = (CS_VARIABLE*)(arg->argPtr);
        if (arg->type == ARG_CONSTANT) {
          argpp[n] = csound->engineState.constantsPool->values + arg->index;
        }
        else if (arg->type == ARG_STRING) {
          argpp[n] = (MYFLT*)(arg->argPtr);
        }
        else if (arg->type == ARG_PFIELD) {
          argpp[n] = lcloffbas + arg->index;
        }
        else if (arg->type == ARG_GLOBAL) {
          argpp[n] =  (MYFLT *) var->memBlock; /*gbloffbas + var->memBlockIndex; */
        }
        else if (arg->type == ARG_LOCAL){
          argpp[n] = lclbas + var->memBlockIndex;
        }
        else if (arg->type == ARG_LABEL) {
          argpp[n] = (MYFLT*)(opMemStart +
                              findLabelMemOffset(csound, tp, (char*)arg->argPtr));
        }
        else {
          csound->Message(csound, "FIXME: instance unexpected arg: %d\n",
                          arg->type);
        }
      }

    }

    /* VL 13-12-13: point the memory to the local ksmps & kr variables,
       and initialise them */
    CS_VARIABLE* var = csoundFindVariableWithName(csound,
                                                  ip->instr->varPool, "ksmps");
    if (var) {
      var->memBlock = lclbas + var->memBlockIndex;
      *((MYFLT *)(var->memBlock)) = csound->ksmps;
    }
    var = csoundFindVariableWithName(csound, ip->instr->varPool, "kr");
    if (var) {
      var->memBlock = lclbas + var->memBlockIndex;
      *((MYFLT *)(var->memBlock)) = csound->ekr;
    }

    if (UNLIKELY(nxtopds > opdslim))
      csoundDie(csound, Str("inconsistent opds total"));

}



int prealloc_(CSOUND *csound, AOP *p, int instname)
{
    int     n, a;

    if (instname)
      n = (int) strarg2opcno(csound, ((STRINGDAT*)p->r)->data, 1,
                             (*p->b == FL(0.0) ? 0 : 1));
    else {
      if (ISSTRCOD(*p->r))
        n = (int) strarg2opcno(csound, get_arg_string(csound,*p->r), 1,
                               (*p->b == FL(0.0) ? 0 : 1));
      else n = *p->r;
    }

    if (UNLIKELY(n < 1))
      return NOTOK;
    a = (int) *p->a - csound->engineState.instrtxtp[n]->active;
    for ( ; a > 0; a--)
      instance(csound, n);
    return OK;
}

int prealloc(CSOUND *csound, AOP *p){
    return prealloc_(csound,p,0);
}

int prealloc_S(CSOUND *csound, AOP *p){
    return prealloc_(csound,p,1);
}

int delete_instr(CSOUND *csound, DELETEIN *p)
{
    int       n;
    INSTRTXT  *ip;
    INSDS     *active;
    INSTRTXT  *txtp;
    int isNamedInstr = (int) csound->GetInputArgSMask(p);

    if (isNamedInstr)
      n = csound->strarg2insno(csound, ((STRINGDAT *)p->insno)->data, isNamedInstr);
    else
      n = (int) (*p->insno + FL(0.5));

    if (UNLIKELY(n < 1 ||
                 n > csound->engineState.maxinsno ||
                 csound->engineState.instrtxtp[n] == NULL))
      return OK;                /* Instrument does not exist so noop */
    ip = csound->engineState.instrtxtp[n];
    active = ip->instance;
    while (active != NULL) {    /* Check there are no active instances */
      INSDS   *nxt = active->nxtinstance;
      if (UNLIKELY(active->actflg)) { /* Can only remove non-active instruments */
        char *name = csound->engineState.instrtxtp[n]->insname;
        if (name)
          return csound->InitError(csound,
                                   Str("Instrument %s is still active"), name);
        else
          return csound->InitError(csound,
                                   Str("Instrument %d is still active"), n);
      }
#if 0
      if (active->opcod_iobufs && active->insno > csound->engineState.maxinsno)
        csound->Free(csound, active->opcod_iobufs);            /* IV - Nov 10 2002 */
#endif
      if (active->fdchp != NULL)
        fdchclose(csound, active);
      if (active->auxchp != NULL)
        auxchfree(csound, active);
      csound->Free(csound, active);
      active = nxt;
    }
    csound->engineState.instrtxtp[n] = NULL;
    /* Now patch it out */
    for (txtp = &(csound->engineState.instxtanchor);
         txtp != NULL;
         txtp = txtp->nxtinstxt)
      if (txtp->nxtinstxt == ip) {
        OPTXT *t = ip->nxtop;
        txtp->nxtinstxt = ip->nxtinstxt;
        while (t) {
          OPTXT *s = t->nxtop;
          csound->Free(csound, t);
          t = s;
        }
        csound->Free(csound, ip);
        return OK;
      }
    return NOTOK;
}

PUBLIC int csoundKillInstance(CSOUND *csound, MYFLT instr, char *instrName,
                              int mode, int allow_release)
{
    INSDS *ip, *ip2, *nip;
    int   insno;

    csoundLockMutex(csound->API_lock);
    if (instrName) {
      insno = named_instr_find(csound, instrName);
      instr = (MYFLT) insno;
    } else insno = instr;

    if (UNLIKELY(insno < 1 || insno > (int) csound->engineState.maxinsno ||
                 csound->engineState.instrtxtp[insno] == NULL)) {
      csoundLockMutex(csound->API_lock);
      return CSOUND_ERROR;
    }

    if (UNLIKELY(mode < 0 || mode > 15 || (mode & 3) == 3)) {
      csoundLockMutex(csound->API_lock);
      return CSOUND_ERROR;
    }
    ip = &(csound->actanchor);
    ip2 = NULL;

    while ((ip = ip->nxtact) != NULL && (int) ip->insno != insno);
    if (UNLIKELY(ip == NULL)) {
      csoundLockMutex(csound->API_lock);
      return CSOUND_ERROR;
    }
    do {                        /* This loop does not terminate in mode=0 */
      nip = ip->nxtact;
      if (((mode & 8) && ip->offtim >= 0.0) ||
          ((mode & 4) && ip->p1 != instr) ||
          (allow_release && ip->relesing)) {
        ip = nip;
        continue;
      }
      if (!(mode & 3)) {
        if (allow_release) {
          xturnoff(csound, ip);
        }
        else {
          nip = ip->nxtact;
          xturnoff_now(csound, ip);
        }
      }
      else {
        ip2 = ip;
        if ((mode & 3) == 1)
          break;
      }
      ip = nip;
    } while (ip != NULL && (int) ip->insno == insno);
    if (ip2 != NULL) {
      if (allow_release) {
        xturnoff(csound, ip2);
      }
      else {
        xturnoff_now(csound, ip2);
      }
    }
    csoundUnlockMutex(csound->API_lock);
    return CSOUND_SUCCESS;
}



/**
   In realtime mode, this function takes care of the init pass in a
   separate thread.
   Any new instances will have their init-pass code executed here.
   This thread is started by musmon() and killed by csoundCleanup()
*/
void *init_pass_thread(void *p){
    CSOUND *csound = (CSOUND *) p;
    INSDS *ip;
    int done;
    float wakeup = (1000*csound->ksmps/csound->esr);
    while (csound->init_pass_loop) {

#if defined(MACOSX) || defined(LINUX) || defined(HAIKU)
      usleep(1000*wakeup);
#else
      csoundSleep(((int)wakeup > 0) ? wakeup : 1);
#endif
      ip = csound->actanchor.nxtact;
      /* do init pass for this instr */
      while (ip != NULL){
        INSDS *nxt = ip->nxtact;
#ifdef HAVE_ATOMIC_BUILTIN
        done = __sync_fetch_and_add((int *) &ip->init_done, 0);
#else
        done = ip->init_done;
#endif
        if (done == 0) {
          csoundLockMutex(csound->init_pass_threadlock);
          csound->ids = (OPDS *) (ip->nxti);
          csound->curip = ip;
          while (csound->ids != NULL) {
            if (UNLIKELY(csound->oparms->odebug))
              csound->Message(csound, "init %s:\n",
                              csound->ids->optext->t.oentry->opname);
            (*csound->ids->iopadr)(csound, csound->ids);
            csound->ids = csound->ids->nxti;
          }
          ip->tieflag = 0;
#ifdef HAVE_ATOMIC_BUILTIN
          __sync_lock_test_and_set((int*)&ip->init_done,1);
#else
          ip->init_done = 1;
#endif
          if (ip->reinitflag==1) {
            ip->reinitflag = 0;
          }
          csoundUnlockMutex(csound->init_pass_threadlock);
        }
        ip = nxt;

      }

    }
    return NULL;
}
