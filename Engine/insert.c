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
/*extern MYFLT cpsocfrc[]; */       /* Needed by CPSOCTL */

static  void    showallocs(CSOUND *);
static  void    deact(CSOUND *, INSDS *);
static  void    schedofftim(CSOUND *, INSDS *);
        void    beatexpire(CSOUND *, double);
        void    timexpire(CSOUND *, double);
static  void    instance(CSOUND *, int);

int init0(CSOUND *csound)
{
    INSTRTXT  *tp = csound->instrtxtp[0];
    INSDS     *ip;

    instance(csound, 0);                            /* allocate instr 0     */
    csound->curip = ip = tp->act_instance;
    tp->act_instance = ip->nxtact;
    csound->ids = (OPDS*) ip;
    tp->active++;
    ip->actflg++;
    csound->inerrcnt = 0;
    while ((csound->ids = csound->ids->nxti) != NULL) {
      (*csound->ids->iopadr)(csound, csound->ids);  /*   run all i-code     */
    }
    return csound->inerrcnt;                        /*   return errcnt      */
}

static void set_xtratim(CSOUND *csound, INSDS *ip)
{
    if (ip->relesing)
      return;
    ip->offtim = (csound->icurTime +
                  csound->ksmps * (double) ip->xtratim)/csound->esr;
    ip->offbet = csound->curBeat + (csound->curBeat_inc * (double) ip->xtratim);
    ip->relesing = 1;
}

/* insert an instr copy into active list */
/*      then run an init pass            */

int insert(CSOUND *csound, int insno, EVTBLK *newevtp)
{
    INSTRTXT  *tp;
    INSDS     *ip, *prvp, *nxtp;
    OPARMS    *O = csound->oparms;

    if (csound->advanceCnt)
      return 0;
    if (UNLIKELY(O->odebug)) {
      char *name = csound->instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("activating instr %s at %d\n"),
                        name, csound->icurTime);
      else
        csound->Message(csound, Str("activating instr %d at %d\n"),
                        insno, csound->icurTime);
    }
    csound->inerrcnt = 0;
    tp = csound->instrtxtp[insno];
    if (UNLIKELY(tp->muted == 0)) {
      char *name = csound->instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Warning(csound, Str("Instrument %s muted\n"), name);
      else
        csound->Warning(csound, Str("Instrument %d muted\n"), insno);
      return 0;
    }
    if (UNLIKELY(tp->mdepends & 04)) {
      char *name = csound->instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("instr %s expects midi event data, "
                                    "cannot run from score\n"), name);
      else
      csound->Message(csound, Str("instr %d expects midi event data, "
                                  "cannot run from score\n"), insno);
      return(1);
    }
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
        goto init;                      /*     continue that event */
      }
    }
    /* alloc new dspace if needed */
    if (tp->act_instance == NULL) {
      if (O->msglevel & RNGEMSG) {
        char *name = csound->instrtxtp[insno]->insname;
        if (UNLIKELY(name))
          csound->Message(csound, Str("new alloc for instr %s:\n"), name);
        else
          csound->Message(csound, Str("new alloc for instr %d:\n"), insno);
      }
      instance(csound, insno);
    }
    /* pop from free instance chain */
    ip = tp->act_instance;
    tp->act_instance = ip->nxtact;
    ip->insno = (int16) insno;

    /* Add an active instrument */
    tp->active++;
    tp->instcnt++;
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
        char *name = csound->instrtxtp[insno]->insname;
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
    if (O->Beatmode)
      ip->p2 = (MYFLT) (csound->icurTime/csound->esr - csound->timeOffs);
    ip->offtim       = (double) ip->p3;         /* & duplicate p3 for now */
    ip->m_chnbp      = (MCHNBLK*) NULL;
    ip->xtratim      = 0;
    ip->relesing     = 0;
    ip->m_sust       = 0;
    ip->nxtolap      = NULL;
    ip->opcod_iobufs = NULL;
    csound->curip    = ip;
    csound->ids      = (OPDS *)ip;
    /* do init pass for this instr */
    while ((csound->ids = csound->ids->nxti) != NULL) {
      if (UNLIKELY(O->odebug))
        csound->Message(csound, "init %s:\n",
                        csound->opcodlst[csound->ids->optext->t.opnum].opname);
      (*csound->ids->iopadr)(csound, csound->ids);
    }
    csound->tieflag = csound->reinitflag = 0;
    if (UNLIKELY(csound->inerrcnt || ip->p3 == FL(0.0))) {
      xturnoff_now(csound, ip);
      return csound->inerrcnt;
    }
#ifdef BETA
    if (UNLIKELY(O->odebug))
      csound->Message(csound, "In insert:  %d %lf %lf\n",
                      __LINE__, ip->p3, ip->offtim); /* *********** */
#endif
    if (ip->p3 > FL(0.0) && ip->offtim > 0.0) { /* if still finite time, */
      double p2 = (double) ip->p2 + csound->timeOffs;
      ip->offtim = p2 + (double) ip->p3;
      /* csound->Message(csound, "ip->offtim = %lf -> ", ip->offtim); */
      ip->offtim = FLOOR(ip->offtim * csound->ekr +0.5)/csound->ekr;
      /* csound->Message(csound, "%lf\n", ip->offtim); */
      if (O->Beatmode) {
        p2 = ((p2*csound->esr - csound->icurTime) / csound->ibeatTime)
          + csound->curBeat;
        ip->offbet = p2 + ((double) ip->p3*csound->esr / csound->ibeatTime);
      }
#ifdef BETA
      if (UNLIKELY(O->odebug))
        csound->Message(csound, "Calling schedofftim line %d; offtime= %lf (%lf)\n",
                        __LINE__, ip->offtim, ip->offtim*csound->ekr);
#endif
      schedofftim(csound, ip);                  /*   put in turnoff list */
    }
    else {
      ip->offbet = -1.0;
      ip->offtim = -1.0;                        /*   else mark indef     */
    }
    if (UNLIKELY(O->odebug)) {
      char *name = csound->instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("instr %s now active:\n"), name);
      else
        csound->Message(csound, Str("instr %d now active:\n"), insno);
      showallocs(csound);
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

    if (csound->advanceCnt)
      return 0;
    if (insno <= 0 || csound->instrtxtp[insno]->muted == 0)
      return 0;     /* muted */

    tp = csound->instrtxtp[insno];
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
      char *name = csound->instrtxtp[insno]->insname;
      if (UNLIKELY(name))
        csound->Message(csound, Str("activating instr %s\n"), name);
      else
        csound->Message(csound, Str("activating instr %d\n"), insno);
    }
    csound->inerrcnt = 0;
    ipp = &chn->kinsptr[mep->dat1];       /* key insptr ptr           */
    /* alloc new dspace if needed */
    if (tp->act_instance == NULL) {
      if (O->msglevel & RNGEMSG) {
        char *name = csound->instrtxtp[insno]->insname;
        if (UNLIKELY(name))
          csound->Message(csound, Str("new alloc for instr %s:\n"), name);
        else
          csound->Message(csound, Str("new alloc for instr %d:\n"), insno);
      }
      instance(csound, insno);
    }
    /* pop from free instance chain */
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
    ip->nxtact = nxtp;
    ip->prvact = prvp;
    prvp->nxtact = ip;
    ip->actflg++;                         /* and mark the instr active */
    ip->m_chnbp = chn;                    /* rec address of chnl ctrl blk */
    ip->m_pitch = (unsigned char) mep->dat1;    /* rec MIDI data   */
    ip->m_veloc = (unsigned char) mep->dat2;
    ip->xtratim = 0;
    ip->m_sust = 0;
    ip->relesing = 0;
    ip->offbet = -1.0;
    ip->offtim = -1.0;              /* set indef duration */
    ip->opcod_iobufs = NULL;        /* IV - Sep 8 2002:            */
    ip->p1 = (MYFLT) insno;         /* set these required p-fields */
    ip->p2 = (MYFLT) (csound->icurTime/csound->esr - csound->timeOffs);
    ip->p3 = FL(-1.0);
    if (tp->psetdata != NULL) {
      MYFLT *pfld = &ip->p3;              /* if pset data present */
      MYFLT *pdat = tp->psetdata + 2;
      int32 nn = tp->pmax - 2;             /*   put cur vals in pflds */
      memcpy(pfld, pdat, nn*sizeof(MYFLT));
/*       do { */
/*         *pfld++ = *pdat++; */
/*       } while (--nn); */
    }

    /* MIDI channel message note on routing overrides pset: */

    if (O->midiKey) {
      int pfield = O->midiKey;
      int index = pfield - 1;
      MYFLT *pfields = &ip->p1;
      MYFLT value = (MYFLT) ip->m_pitch;
      pfields[index] = value;
      if (O->msglevel & WARNMSG) {
        csound->Message(csound, "  midiKey:         pfield: %3d  value: %.3f\n",
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
      if (O->msglevel & WARNMSG) {
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
      if (O->msglevel & WARNMSG) {
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
                        pfield, pfields[index]);
      }
    }
    /* 
       the code above assumes &p1 is a pointer to an array of N pfields, but 
       this is wrong. It overwrites memory and uses it for passing p-field
       values. When the overwritten memory is taken to be a pointer in the
       loop below, the loop does not stop at the end of the opcode list
       and causes iopadr to be garbage, leading to a segfault. 
       This happens where there is exactly one opcode in an instrument. 
       It is a nasty bug that needs to be fixed. 
        
       A possible solution is to  allocate always a minimum of 5 p-fields (see line
       approx 1809 below). The extra p-fields appear to be hanging at the end of 
       an INSDS structure, and &p1 appears to be a legal array start address.
       This allows p4 and p5 to be mapped, but no further p-fields (possibly).

       This fix is a bit of hack IMHO. But I have implemented it here, as it
       seemingly prevents the crashes.

       (JPff) a safer fix to readthe extra arg numbers
     */

    csound->curip = ip;
    csound->ids = (OPDS *)ip;
    /* do init pass for this instr  */
    while ((csound->ids = csound->ids->nxti) != NULL) {
      if (O->odebug)
        csound->Message(csound, "init %s:\n",
                        csound->opcodlst[csound->ids->optext->t.opnum].opname);
      (*csound->ids->iopadr)(csound, csound->ids);
    }
    csound->tieflag = csound->reinitflag = 0;
    if (csound->inerrcnt) {
      xturnoff_now(csound, ip);
      return csound->inerrcnt;
    }
    if (UNLIKELY(O->odebug)) {
      char *name = csound->instrtxtp[insno]->insname;
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
    for (txtp = &(csound->instxtanchor);  txtp != NULL;  txtp = txtp->nxtinstxt)
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
    csound->instrtxtp[ip->insno]->active--; /* remove an active instrument */
    csound->cpu_power_busy -= csound->instrtxtp[ip->insno]->cpuload;
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
      char *name = csound->instrtxtp[ip->insno]->insname;
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
    ip->nxtact = csound->instrtxtp[ip->insno]->act_instance;
    csound->instrtxtp[ip->insno]->act_instance = ip;
    if (ip->fdchp != NULL)
      fdchclose(csound, ip);
}

/* Turn off a particular insalloc, also remove from list of active */
/* MIDI notes. Allows for releasing if ip->xtratim > 0. */

void xturnoff(CSOUND *csound, INSDS *ip)  /* turnoff a particular insalloc  */
{                                         /* called by inexclus on ctrl 111 */
    MCHNBLK *chn;

    if (ip->relesing)
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

void orcompact(CSOUND *csound)          /* free all inactive instr spaces */
{
    INSTRTXT  *txtp;
    INSDS     *ip, *nxtip, *prvip, **prvnxtloc;
    int       cnt = 0;

    for (txtp = &(csound->instxtanchor);
         txtp != NULL;  txtp = txtp->nxtinstxt) {
      if ((ip = txtp->instance) != NULL) {        /* if instance exists */
        prvip = NULL;
        prvnxtloc = &txtp->instance;
        do {
          if (!ip->actflg) {
            cnt++;
            if (ip->opcod_iobufs && ip->insno > csound->maxinsno)
              mfree(csound, ip->opcod_iobufs);          /* IV - Nov 10 2002 */
            if (ip->fdchp != NULL)
              fdchclose(csound, ip);
            if (ip->auxchp != NULL)
              auxchfree(csound, ip);
            if ((nxtip = ip->nxtinstance) != NULL)
              nxtip->prvinstance = prvip;
            *prvnxtloc = nxtip;
            mfree(csound, (char *)ip);
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
    if (UNLIKELY(cnt))
      csound->Message(csound, Str("inactive allocs returned to freespace\n"));
}

void infoff(CSOUND *csound, MYFLT p1)   /* turn off an indef copy of instr p1 */
{                                       /*      called by musmon              */
    INSDS *ip;
    int   insno;

    insno = (int) p1;
    if (LIKELY((ip = (csound->instrtxtp[insno])->instance) != NULL)) {
      do {
        if (ip->insno == insno          /* if find the insno */
            && ip->actflg               /*      active       */
            && ip->offtim < 0.0          /*  but indef, VL: currently this condition
            cannot be removed, as it breaks turning off extratime instances */
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
        sprintf(buf, Str("INIT ERROR in instr %d (opcode %s): "),
                ip->insno, op->name);
      else
        sprintf(buf, Str("INIT ERROR in instr %d (subinstr %d): "),
                ip->insno, csound->ids->insdshead->insno);
    }
    else
      sprintf(buf, Str("INIT ERROR in instr %d: "), ip->insno);
    va_start(args, s);
    csoundErrMsgV(csound, buf, s, args);
    va_end(args);
    putop(csound, &(csound->ids->optext->t));

    return ++(csound->inerrcnt);
}

int csoundPerfError(CSOUND *csound, const char *s, ...)
{
    va_list args;
    INSDS   *ip;
    char    buf[512];

    /* RWD and probably this too... */
    if (UNLIKELY(csound->pds == NULL)) {
      va_start(args, s);
      csoundErrMsgV(csound, Str("\nPERF ERROR: "), s, args);
      va_end(args);
      csound->LongJmp(csound, 1);
    }
    /* IV - Oct 16 2002: check for subinstr and user opcode */
    ip = csound->pds->insdshead;
    if (ip->opcod_iobufs) {
      OPCODINFO *op = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->opcode_info;
      /* find top level instrument instance */
      do {
        ip = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->parent_ip;
      } while (ip->opcod_iobufs);
      if (op)
        sprintf(buf, Str("PERF ERROR in instr %d (opcode %s): "),
                     ip->insno, op->name);
      else
        sprintf(buf, Str("PERF ERROR in instr %d (subinstr %d): "),
                     ip->insno, csound->pds->insdshead->insno);
    }
    else
      sprintf(buf, Str("PERF ERROR in instr %d: "), ip->insno);
    va_start(args, s);
    csoundErrMsgV(csound, buf, s, args);
    va_end(args);
    putop(csound, &(csound->pds->optext->t));
    csoundMessage(csound, Str("   note aborted\n"));
    csound->perferrcnt++;
    xturnoff_now((CSOUND*) csound, ip);       /* rm ins fr actlist */
    while (csound->pds->nxtp != NULL)
      csound->pds = csound->pds->nxtp;        /* loop to last opds */

    return csound->perferrcnt;                /* contin from there */
}

/* IV - Oct 12 2002: new simplified subinstr functions */

int subinstrset(CSOUND *csound, SUBINST *p)
{
    OPDS    *saved_ids = csound->ids;
    INSDS   *saved_curip = csound->curip;
    MYFLT   *flp;
    int     instno, n, init_op, inarg_ofs;

    /* check if we are using subinstrinit or subinstr */
    init_op = (p->h.opadr == NULL ? 1 : 0);
    inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
    /* IV - Oct 31 2002 */
    if (UNLIKELY((instno = strarg2insno(csound, p->ar[inarg_ofs],
                                        (p->XSTRCODE & 1)))
                 < 0))
      return NOTOK;
    /* IV - Oct 9 2002: need this check */
    if (UNLIKELY(!init_op && p->OUTOCOUNT > csound->nchnls)) {
      return csoundInitError(csound, Str("subinstr: number of output "
                                         "args greater than nchnls"));
    }
    /* IV - Oct 9 2002: copied this code from useropcdset() to fix some bugs */
    if (!(csound->reinitflag | csound->tieflag)) {
      /* get instance */
      if (csound->instrtxtp[instno]->act_instance == NULL)
        instance(csound, instno);
      p->ip = csound->instrtxtp[instno]->act_instance;
      csound->instrtxtp[instno]->act_instance = p->ip->nxtact;
      p->ip->insno = (int16) instno;
      p->ip->actflg++;                  /*    and mark the instr active */
      csound->instrtxtp[instno]->active++;
      csound->instrtxtp[instno]->instcnt++;
      p->ip->p1 = (MYFLT) instno;
      p->ip->opcod_iobufs = (void*) &p->buf;
      /* link into deact chain */
      p->ip->subins_deact = saved_curip->subins_deact;
      p->ip->opcod_deact = NULL;
      saved_curip->subins_deact = (void*) p;
      p->parent_ip = p->buf.parent_ip = saved_curip;
    }
    /* copy parameters from this instrument into our subinstrument */
    p->ip->xtratim  = saved_curip->xtratim;
    p->ip->m_sust   = 0;
    p->ip->relesing = saved_curip->relesing;
    p->ip->offbet   = saved_curip->offbet;
    p->ip->offtim   = saved_curip->offtim;
    p->ip->nxtolap  = NULL;
    p->ip->p2       = saved_curip->p2;
    p->ip->p3       = saved_curip->p3;

    /* IV - Oct 31 2002 */
    p->ip->m_chnbp  = saved_curip->m_chnbp;
    p->ip->m_pitch  = saved_curip->m_pitch;
    p->ip->m_veloc  = saved_curip->m_veloc;

    /* copy remainder of pfields */
    flp = &p->ip->p3 + 1;
    /* by default all inputs are i-rate mapped to p-fields */
    if (UNLIKELY(p->INOCOUNT > (csound->instrtxtp[instno]->pmax + 1))) {
      return csoundInitError(csound, Str("subinstr: too many p-fields"));
    }
    for (n = 1; n < p->INOCOUNT; n++)
      *flp++ = *p->ar[inarg_ofs + n];

    /* allocate memory for a temporary store of spout buffers */
    if (!init_op && !(csound->reinitflag | csound->tieflag))
      csoundAuxAlloc(csound,
                     (int32) csound->nspout * sizeof(MYFLT), &p->saved_spout);

    /* do init pass for this instr */
    csound->curip = p->ip;
    csound->ids = (OPDS *)p->ip;

    while ((csound->ids = csound->ids->nxti) != NULL) {
      (*csound->ids->iopadr)(csound, csound->ids);
    }
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

/* IV - Sep 8 2002: new functions for user defined opcodes (based */
/* on Matt J. Ingalls' subinstruments, but mostly rewritten) */

int useropcd1(CSOUND *, UOPCODE*), useropcd2(CSOUND *, UOPCODE*);

int useropcdset(CSOUND *csound, UOPCODE *p)
{
    OPDS    *saved_ids = csound->ids;
    INSDS   *saved_curip = csound->curip, *parent_ip = csound->curip, *lcurip;
    INSTRTXT  *tp;
    int     instno, i, n, pcnt;
    OPCODINFO *inm;
    OPCOD_IOBUFS  *buf;
    int     g_ksmps;
    int32    g_kcounter;
    MYFLT   g_ekr, g_onedkr, g_onedksmps, g_kicvt;

    g_ksmps = p->l_ksmps = csound->ksmps;       /* default ksmps */
    p->ksmps_scale = 1;
    /* look up the 'fake' instr number, and opcode name */
    inm = (OPCODINFO*) csound->opcodlst[p->h.optext->t.opnum].useropinfo;
    instno = inm->instno;
    tp = csound->instrtxtp[instno];
    /* set local ksmps if defined by user */
    n = p->OUTOCOUNT + p->INOCOUNT - 1;
    if (*(p->ar[n]) != FL(0.0)) {
      i = (int) *(p->ar[n]);
      if (UNLIKELY(i < 1 || i > csound->ksmps ||
                   ((csound->ksmps / i) * i) != csound->ksmps)) {
        return csoundInitError(csound, Str("%s: invalid local ksmps value: %d"),
                                       inm->name, i);
      }
      p->l_ksmps = i;
    }
    /* save old globals */
    g_kcounter = csound->kcounter;
    g_ekr = csound->ekr;
    g_onedkr = csound->onedkr;
    g_onedksmps = csound->onedksmps;
    g_kicvt = csound->kicvt;
    /* set up local variables depending on ksmps, also change globals */
    if (p->l_ksmps != g_ksmps) {
      csound->ksmps = p->l_ksmps; /* Oh dear!  breaks many assumptions -- JPff */
      p->ksmps_scale = g_ksmps / (int) csound->ksmps;
      csound->pool[csound->poolcount + 2] = (MYFLT) p->l_ksmps;
      p->l_onedksmps = csound->onedksmps = FL(1.0) / (MYFLT) p->l_ksmps;
      p->l_ekr = csound->ekr = csound->pool[csound->poolcount + 1] =
          csound->esr / (MYFLT) p->l_ksmps;
      p->l_onedkr = csound->onedkr = FL(1.0) / p->l_ekr;
      p->l_kicvt = csound->kicvt = (MYFLT) FMAXLEN / p->l_ekr;
      csound->kcounter *= p->ksmps_scale;
    }

    if (!p->ip) {
      /* search for already allocated, but not active instance */
      /* if none was found, allocate a new instance */
      if (!tp->act_instance)
        instance(csound, instno);
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

      /* store parameters of input and output channels, and parent ip */
      buf->uopcode_struct = (void*) p;
      buf->parent_ip = p->parent_ip = parent_ip;
    }

    /* copy parameters from the caller instrument into our subinstrument */
    lcurip = p->ip;
    lcurip->m_chnbp = parent_ip->m_chnbp;       /* MIDI parameters */
    lcurip->m_pitch = parent_ip->m_pitch;
    lcurip->m_veloc = parent_ip->m_veloc;
    lcurip->xtratim = parent_ip->xtratim * p->ksmps_scale;
    lcurip->m_sust = 0;
    lcurip->relesing = parent_ip->relesing;
    lcurip->offbet = parent_ip->offbet;
    lcurip->offtim = parent_ip->offtim;
    lcurip->nxtolap = NULL;
    /* copy all p-fields, including p1 (will this work ?) */
    if (tp->pmax > 3) {         /* requested number of p-fields */
      n = tp->pmax; pcnt = 0;
      while (pcnt < n) {
        if ((i = csound->instrtxtp[parent_ip->insno]->pmax) > pcnt) {
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
    csound->curip = lcurip;
    csound->ids = (OPDS *) (lcurip->nxti);
    while (csound->ids != NULL) {
      (*csound->ids->iopadr)(csound, csound->ids);
      csound->ids = csound->ids->nxti;
    }
    /* copy length related parameters back to caller instr */
    saved_curip->relesing = lcurip->relesing;
    saved_curip->offbet = lcurip->offbet;
    saved_curip->offtim = lcurip->offtim;
    saved_curip->p3 = lcurip->p3;

    /* restore globals */
    csound->ids = saved_ids;
    csound->curip = saved_curip;
    if (csound->ksmps != g_ksmps) {
      csound->ksmps = g_ksmps;
      saved_curip->xtratim = lcurip->xtratim / p->ksmps_scale;
      csound->pool[csound->poolcount + 2] = (MYFLT) g_ksmps;
      csound->kcounter = g_kcounter;
      csound->ekr = csound->pool[csound->poolcount + 1] = g_ekr;
      csound->onedkr = g_onedkr;
      csound->onedksmps = g_onedksmps;
      csound->kicvt = g_kicvt;
      /* IV - Sep 17 2002: also select perf routine */
      p->h.opadr = (SUBR) useropcd1;
    }
    else {
      saved_curip->xtratim = lcurip->xtratim;
      p->h.opadr = (SUBR) useropcd2;
    }

    return OK;
}

/* IV - Sep 17 2002: dummy user opcode function for not initialised case */

int useropcd(CSOUND *csound, UOPCODE *p)
{
    return csoundPerfError(csound, Str("%s: not initialised"),
                                   p->h.optext->t.opcod);
}

/* IV - Sep 1 2002: new opcodes: xin, xout */

int xinset(CSOUND *csound, XIN *p)
{
    OPCOD_IOBUFS  *buf;
    OPCODINFO   *inm;
    int16       *ndx_list;
    MYFLT       **tmp, **bufs;

    (void) csound;
    buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
    inm = buf->opcode_info;
    bufs = ((UOPCODE*) buf->uopcode_struct)->ar + inm->outchns;
    /* copy i-time variables */
    ndx_list = inm->in_ndx_list - 1;
    while (*++ndx_list >= 0)
      *(*(p->args + *ndx_list)) = *(*(bufs + *ndx_list));
    /* IV - Jul 29 2006: and string variables */
    while (*++ndx_list >= 0) {
      const char  *src = (char *)bufs[*ndx_list];
      char  *dst = (char *)(p->args[*ndx_list]);
      int n;
      /* FIXME: should throw error instead of truncating string ? */
      for (n = csound->strVarMaxLen - 1; *src != '\0' && n != 0; n--)
        *(dst++) = *(src++);
      *dst = '\0';
    }

    /* find a-rate variables and add to list of perf-time buf ptrs ... */
    tmp = buf->iobufp_ptrs;
    if (*tmp || *(tmp + 1))
      return OK;

    while (*++ndx_list >= 0) {
      *(tmp++) = *(bufs + *ndx_list);   /* "from" address */
      *(tmp++) = *(p->args + *ndx_list);/* "to" address */
    }
    *(tmp++) = NULL;            /* put delimiter */
    /* ... same for k-rate */
    while (*++ndx_list >= 0) {
      *(tmp++) = *(bufs + *ndx_list);   /* "from" address */   
      *(tmp++) = *(p->args + *ndx_list);/* "to" address */
    }
    *(tmp++) = NULL;            /* put delimiter */
    /* fsigs: we'll need to do extra work */
     while (*++ndx_list >= 0) {
       void *in, *out;
       in = (void *)*(bufs + *ndx_list);
       *(tmp++) = (MYFLT *) in;   
       out = (void *) *(p->args + *ndx_list);
       *(tmp++) = (MYFLT *) out;
       memcpy(out, in, sizeof(PVSDAT));    
    }
     *(tmp++) = NULL;
    /* tsigs: similar to avove */
     while (*++ndx_list >= 0) {
       void *in, *out;
       in = (void *)*(bufs + *ndx_list);
       *(tmp++) = (MYFLT *) in;   
       out = (void *) *(p->args + *ndx_list);
       *(tmp++) = (MYFLT *) out;
       memcpy(out, in, sizeof(TABDAT));   
    }
     *(tmp++) = NULL;
   
    /* fix for case when xout is omitted */
    *(tmp++) = NULL;  *(tmp++) = NULL;  *(tmp++) = NULL; *tmp = NULL;
    return OK;
}

int xoutset(CSOUND *csound, XOUT *p)
{
    OPCOD_IOBUFS  *buf;
    OPCODINFO   *inm;
    int16       *ndx_list;
    MYFLT       **tmp, **bufs;
    
    (void) csound;
    buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
    inm = buf->opcode_info;
    bufs = ((UOPCODE*) buf->uopcode_struct)->ar;
    /* copy i-time variables */
    ndx_list = inm->out_ndx_list - 1;
    while (*++ndx_list >= 0) {
      *(*(bufs + *ndx_list)) = *(*(p->args + *ndx_list));
    }
    /* IV - Jul 29 2006: and string variables */
    while (*++ndx_list >= 0) {
      const char  *src = (char *)(p->args[*ndx_list]);
      char  *dst = (char *)(bufs[*ndx_list]);
      int n;
      /* FIXME: should throw error instead of truncating string ? */
      for (n = csound->strVarMaxLen - 1; *src != '\0' && n != 0; n--)
        *(dst++) = *(src++);
      *dst = '\0';
    }
    /* skip input pointers, including the three delimiter NULLs */
    tmp = buf->iobufp_ptrs;
    /* VL: needs to check if there are not 4 nulls in a sequence, which
       would indicate no a, k, f or t sigs */
    if (*tmp || *(tmp + 1) || *(tmp + 2) || *(tmp + 3)) tmp += (inm->perf_incnt << 1);
    tmp += 4;  /* VL: this was 2, now 4 with fsigs and tsigs added */
    if (*tmp || *(tmp + 1))
    return OK;
   
    /* find a-rate variables and add to list of perf-time buf ptrs ... */
    while (*++ndx_list >= 0) {
      *(tmp++) = *(p->args + *ndx_list);/* "from" address */
      *(tmp++) = *(bufs + *ndx_list);   /* "to" address */
    }
    *(tmp++) = NULL;            /* put delimiter */
    /* ... same for k-rate */
    while (*++ndx_list >= 0) {
      *(tmp++) = *(p->args + *ndx_list);/* "from" address */
      *(tmp++) = *(bufs + *ndx_list);   /* "to" address */
    }
    *(tmp++) = NULL;                /* put delimiter */
 
    /* fsigs: we'll need to do extra work */
    while (*++ndx_list >= 0) {
      void *in, *out;
      in =  (void *) *(p->args + *ndx_list);
      *(tmp++) = (MYFLT *) in;
      out = (void *) *(bufs + *ndx_list); 
      *(tmp++) = (MYFLT *) out;
      memcpy(out, in, sizeof(PVSDAT));
    }
    *(tmp++) = NULL; 
   /* tsigs: as above */
    while (*++ndx_list >= 0) {
      void *in, *out;
      in =  (void *) *(p->args + *ndx_list);
      *(tmp++) = (MYFLT *) in;
      out = (void *) *(bufs + *ndx_list); 
      *(tmp++) = (MYFLT *) out;
      memcpy(out, in, sizeof(TABDAT));
    }
    *tmp = NULL;

    return OK;
}

/* IV - Sep 8 2002: new opcode: setksmps */

int setksmpsset(CSOUND *csound, SETKSMPS *p)
{
    OPCOD_IOBUFS  *buf;
    UOPCODE       *pp;
    int           l_ksmps, n;

    buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
    l_ksmps = (int) *(p->i_ksmps);
    if (!l_ksmps) return OK;       /* zero: do not change */
    if (UNLIKELY(l_ksmps < 1 || l_ksmps > csound->ksmps
                 || ((csound->ksmps / l_ksmps) * l_ksmps != csound->ksmps))) {
      return csoundInitError(csound, Str("setksmps: invalid ksmps value: %d"),
                                     l_ksmps);
    }
    /* set up global variables according to the new ksmps value */
    pp = (UOPCODE*) buf->uopcode_struct;
    n = csound->ksmps / l_ksmps;
    pp->ksmps_scale *= n;
    p->h.insdshead->xtratim *= n;
    pp->l_ksmps = csound->ksmps = l_ksmps;
    csound->pool[csound->poolcount + 2] = (MYFLT) csound->ksmps;
    pp->l_onedksmps = csound->onedksmps = FL(1.0) / (MYFLT) csound->ksmps;
    pp->l_ekr = csound->ekr = csound->pool[csound->poolcount + 1] =
        csound->esr / (MYFLT) csound->ksmps;
    pp->l_onedkr = csound->onedkr = FL(1.0) / csound->ekr;
    pp->l_kicvt = csound->kicvt = (MYFLT) FMAXLEN / csound->ekr;
    csound->kcounter *= pp->ksmps_scale;
    return OK;
}

/* IV - Oct 16 2002: nstrnum opcode (returns the instrument number of a */
/* named instrument) */

int nstrnumset(CSOUND *csound, NSTRNUM *p)
{
    /* IV - Oct 31 2002 */
    *(p->i_insno) = (MYFLT) strarg2insno(csound, p->iname, (p->XSTRCODE & 1));
    return (*(p->i_insno) > FL(0.0) ? OK : NOTOK);
}

/* IV - Nov 16 2002: moved insert_event() here to have access to some static */
/* functions defined in this file */

INSDS *insert_event(CSOUND *csound,
                    MYFLT instr,
                    MYFLT when,
                    MYFLT dur,
                    int narg,
                    MYFLT **args,
                    int midi)
{
    int pcnt = narg + 3;
    int insno = (int) instr, saved_inerrcnt = csound->inerrcnt;
    int saved_reinitflag = csound->reinitflag, saved_tieflag = csound->tieflag;
    INSDS     *saved_curip = csound->curip, *ip = NULL;
    INSDS     *prvp, *nxtp;                             /* IV - Nov 16 2002 */
    OPDS      *saved_ids = csound->ids;
    OPARMS    *O = csound->oparms;
    INSTRTXT  *tp;

    if (csound->advanceCnt)
      return NULL;

    csound->inerrcnt = csound->tieflag = csound->reinitflag = 0;
    tp = csound->instrtxtp[insno];
    if (UNLIKELY(tp == NULL)) {
      csound->Message(csound,
                      Str("schedule event ignored. instr %d undefined\n"),
                      insno);
      csound->perferrcnt++;
      goto endsched;            /* IV - Nov 16 2002 */
    }
    csound->cpu_power_busy += tp->cpuload;
    /* if there is no more cpu processing time: */
    if (UNLIKELY(csound->cpu_power_busy > 100.0)) {
      csound->cpu_power_busy -= tp->cpuload;
      csoundWarning(csound, Str("cannot allocate last note because it exceeds "
                                "100%% of cpu time"));
      goto endsched;
    }
    if (UNLIKELY(tp->maxalloc > 0 && tp->active >= tp->maxalloc)) {
      csoundWarning(csound, Str("cannot allocate last note because it exceeds "
                                "instr maxalloc"));
      goto endsched;
    }
    /* Insert this event into event queue */
    if (UNLIKELY(O->odebug))
      csound->Message(csound, "activating instr %d\n", insno);
    if (UNLIKELY((tp->mdepends & 4) && !midi)) {
      char *name = csound->instrtxtp[ip->insno]->insname;
      if (name)
        csound->Message(csound, Str("instr %s expects midi event data, "
                                    "cannot run from score\n"), name);
      else
        csound->Message(csound, Str("instr %d expects midi event data, "
                                    "cannot run from score\n"), insno);
      csound->perferrcnt++;
      goto endsched;
    }
    /* if find this insno, active, with indef (tie) & matching p1 */
    for (ip = tp->instance; ip != NULL; ip = ip->nxtinstance) {
      /* if find this insno, active, with indef (tie) & matching p1 */
      if (ip->actflg && ip->offtim < 0.0 && ip->p1 == instr) {
        csound->tieflag++;
        goto init;                      /*     continue that event */
      }
    }
    /* alloc new dspace if needed */
    if (tp->act_instance == NULL) {
      if (O->msglevel & RNGEMSG) {
      char *name = csound->instrtxtp[insno]->insname;
      if (name)
        csound->Message(csound, Str("new alloc for instr %s:\n"), name);
      else
        csound->Message(csound, Str("new alloc for instr %d:\n"), insno);
      }
      instance(csound, insno);
    }
    /* pop from free instance chain */
    ip = tp->act_instance;
    tp->act_instance = ip->nxtact;
    ip->insno = (int16) insno;

    /* Add an active instrument */
    tp->active++;
    tp->instcnt++;
    nxtp = &(csound->actanchor);    /* now splice into active list */
    while ((prvp = nxtp) && (nxtp = prvp->nxtact) != NULL)
      if (nxtp->insno > insno    ||
          (nxtp->insno == insno && nxtp->p1 > instr)) {
        nxtp->prvact = ip;
        break;
      }
    ip->nxtact = nxtp;
    ip->prvact = prvp;
    prvp->nxtact = ip;
    ip->actflg++;             /*        and mark the instr active */
 init:
    {
      int i;
      int imax = tp->pmax - 3;
      MYFLT  *flp;
      if (UNLIKELY((int) tp->pmax != pcnt)) {
        char *name = csound->instrtxtp[insno]->insname;
        if (name)
          csoundWarning(csound, Str("instr %s pmax = %d, note pcnt = %d"),
                      name, (int) tp->pmax, pcnt);
        else
          csoundWarning(csound, Str("instr %d pmax = %d, note pcnt = %d"),
                        insno, (int) tp->pmax, pcnt);
      }
      ip->p1 = instr;
      ip->p2 = when;
      ip->p3 = dur;
      flp = &(ip->p1) + 3;
      if (UNLIKELY(O->odebug))
        csound->Message(csound, Str("psave beg at %p\n"), flp);
      for (i = 0; i < imax; i++) {
        if (i < narg)
          *flp++ = *(args[i]);
        else
          *flp++ = FL(0.0);
      }
      if (UNLIKELY(O->odebug))
        csound->Message(csound, Str("   ending at %p\n"), flp);
    }
    if (O->Beatmode)
      ip->p2 = (MYFLT) (csound->icurTime/csound->esr - csound->timeOffs);
    ip->offbet = (double) ip->p3;
    ip->offtim = (double) ip->p3;       /* & duplicate p3 for now */
    ip->xtratim = 0;
    ip->relesing = 0;
    ip->m_sust = 0;
    ip->nxtolap = NULL;
    /* IV - Nov 16 2002 */
    ip->opcod_iobufs = NULL;
    if (midi) {
      /* should we copy MIDI parameters from the note from which the */
      /* event was scheduled ? */
      ip->m_chnbp = saved_curip->m_chnbp;
      ip->m_pitch = saved_curip->m_pitch;
      ip->m_veloc = saved_curip->m_veloc;
    }
    else
      ip->m_chnbp = NULL;     /* score event */
    csound->curip = ip;
    csound->ids = (OPDS *)ip;
    /* do init pass for this instr */
    while ((csound->ids = csound->ids->nxti) != NULL) {
      /*    if (O->odebug) csound->Message(csound, "init %s:\n",
            csound->opcodlst[csound->ids->optext->t.opnum].opname);      */
      (*csound->ids->iopadr)(csound, csound->ids);
    }
    if (csound->inerrcnt || ip->p3 == FL(0.0)) {
      xturnoff_now(csound, ip);
      ip = NULL; goto endsched;
    }
    if (!midi &&                                /* if not MIDI activated, */
        ip->p3 > FL(0.0) && ip->offtim > 0.0) { /* and still finite time, */
      double p2;
      p2 = (double) ip->p2 + csound->timeOffs;
      ip->offtim = p2 + (double) ip->p3;
      p2 = ((p2 - csound->icurTime) / csound->ibeatTime) + csound->curBeat;
      ip->offbet = p2 + ((double) ip->p3*csound->esr / csound->ibeatTime);
      schedofftim(csound, ip);  /*      put in turnoff list */
      if (!ip->actflg) {
        ip = NULL; goto endsched;
      }
    }
    else {
      ip->offbet = -1.0;
      ip->offtim = -1.0;        /* else mark indef */
    }
    if (UNLIKELY(O->odebug)) {
      csound->Message(csound, "instr %d now active:\n", insno);
      showallocs(csound);
    }
 endsched:
    /* IV - Nov 16 2002: restore globals */
    csound->inerrcnt = saved_inerrcnt;
    csound->reinitflag = saved_reinitflag;
    csound->tieflag = saved_tieflag;
    csound->curip = saved_curip;
    csound->ids = saved_ids;
    return ip;
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

int subinstr(CSOUND *csound, SUBINST *p)
{
    OPDS    *saved_pds = csound->pds;
    int     saved_sa = csound->spoutactive;
    MYFLT   *pbuf, *saved_spout = csound->spout;
    int32    frame, chan;

    if (UNLIKELY(p->ip == NULL)) {                /* IV - Oct 26 2002 */
      return csoundPerfError(csound, Str("subinstr: not initialised"));
    }
    /* copy current spout buffer and clear it */
    csound->spout = (MYFLT*) p->saved_spout.auxp;
    csound->spoutactive = 0;
    /* update release flag */
    p->ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */

    /*  run each opcode  */
    csound->pds = (OPDS *)p->ip;
    while ((csound->pds = csound->pds->nxtp) != NULL) {
      (*csound->pds->opadr)(csound, csound->pds);
    }

    /* copy outputs */
    if (csound->spoutactive) {
      for (chan = 0; chan < p->OUTOCOUNT; chan++) {
        for (pbuf = csound->spout + chan, frame = 0;
             frame < csound->ksmps; frame++) {
          p->ar[chan][frame] = *pbuf;
          pbuf += csound->nchnls;
        }
      }
    }
    else {
      for (chan = 0; chan < p->OUTOCOUNT; chan++)
        for (frame = 0; frame < csound->ksmps; frame++)
          p->ar[chan][frame] = FL(0.0);
    }

    /* restore spouts */
    csound->spout = saved_spout;
    csound->spoutactive = saved_sa;
    csound->pds = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip)                                         /* loop to last opds */
      while (csound->pds->nxtp) csound->pds = csound->pds->nxtp;
    return OK;
}

/* IV - Sep 17 2002 -- case 1: local ksmps is used */

int useropcd1(CSOUND *csound, UOPCODE *p)
{
    OPDS    *saved_pds = csound->pds;
    int     g_ksmps, ofs = 0, n;
    MYFLT   g_ekr, g_onedkr, g_onedksmps, g_kicvt, **tmp, *ptr1, *ptr2;
    int32    g_kcounter;

    /* update release flag */
    p->ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */
    /* save old globals */
    g_ksmps = csound->ksmps;
    g_ekr = csound->ekr;
    g_onedkr = csound->onedkr;
    g_onedksmps = csound->onedksmps;
    g_kicvt = csound->kicvt;
    g_kcounter = csound->kcounter;
    /* set local ksmps and related values */
    csound->ksmps = p->l_ksmps;
    csound->pool[csound->poolcount + 2] = (MYFLT) p->l_ksmps;
    csound->ekr = csound->pool[csound->poolcount + 1] = p->l_ekr;
    csound->onedkr = p->l_onedkr;
    csound->onedksmps = p->l_onedksmps;
    csound->kicvt = p->l_kicvt;
    csound->kcounter = csound->kcounter * p->ksmps_scale;

    if (csound->ksmps == 1) {           /* special case for local kr == sr */
      do {
        /* copy inputs */
        tmp = p->buf->iobufp_ptrs;
        while (*tmp) {                  /* a-rate */
          ptr1 = *(tmp++) + ofs; *(*(tmp++)) = *ptr1;
        }
        while (*(++tmp)) {              /* k-rate */
          ptr1 = *tmp; *(*(++tmp)) = *ptr1;
        }
        /* VL: fsigs in need to be dealt with here */
        while (*(++tmp)) {                
         ptr1 = *tmp; 
         memcpy((void *)(*(++tmp)), (void *) ptr1, sizeof(PVSDAT)); 
         }
        /* and tsigs */
        while (*(++tmp)) {                
         ptr1 = *tmp; 
         memcpy((void *)(*(++tmp)), (void *) ptr1, sizeof(TABDAT)); 
         }
        
        /*  run each opcode  */
        csound->pds = (OPDS *) (p->ip);
        while ((csound->pds = csound->pds->nxtp)) {
          (*csound->pds->opadr)(csound, csound->pds);
        }
        /* copy outputs */
        while (*(++tmp)) {              /* a-rate */
          ptr1 = *tmp; (*(++tmp))[ofs] = *ptr1;
        }
        ++(csound->kcounter);
      } while (++ofs < g_ksmps);
    }
    else {                              /* generic case for local kr != sr */
      do {
        /* copy inputs */
        tmp = p->buf->iobufp_ptrs;
        while (*tmp) {                  /* a-rate */
          ptr1 = *(tmp++) + ofs; ptr2 = *(tmp++);
          n = csound->ksmps;
          do {
            *(ptr2++) = *(ptr1++);
          } while (--n);
        }
        while (*(++tmp)) {              /* k-rate */
          ptr1 = *tmp; *(*(++tmp)) = *ptr1;
        }
        /* VL: fsigs in need to be dealt with here */
        while (*(++tmp)) {                
         ptr1 = *tmp; 
         memcpy((void *)(*(++tmp)), (void *) ptr1, sizeof(PVSDAT)); 
         }
        /* and tsigs */
        while (*(++tmp)) {                
         ptr1 = *tmp; 
         memcpy((void *)(*(++tmp)), (void *) ptr1, sizeof(TABDAT)); 
         } 
        /*  run each opcode  */
        csound->pds = (OPDS *) (p->ip);
        while ((csound->pds = csound->pds->nxtp)) {
          (*csound->pds->opadr)(csound, csound->pds);
        }
        /* copy outputs */
        while (*(++tmp)) {              /* a-rate */
          ptr1 = *tmp; ptr2 = *(++tmp) + ofs;
          n = csound->ksmps;
          do {
            *(ptr2++) = *(ptr1++);
          } while (--n);
        }
        ++(csound->kcounter);
      } while ((ofs += csound->ksmps) < g_ksmps);
    }
    /* k-rate outputs are copied only in the last sub-kperiod, */
    /* so we do it now */
    while (*(++tmp)) {                  /* k-rate */
      ptr1 = *tmp; 
      *(*(++tmp)) = *ptr1;
    }
    /* VL: fsigs out need to be dealt with here */
     while (*(++tmp)) {                
       ptr1 = *tmp;
       memcpy((void *)(*(++tmp)), (void *)ptr1, sizeof(PVSDAT));
    }
     /* tsigs  */
    while (*(++tmp)) {                
       ptr1 = *tmp;
       memcpy((void *)(*(++tmp)), (void *)ptr1, sizeof(TABDAT));
    }
    
    /* restore globals */
    csound->ksmps = g_ksmps;
    csound->pool[csound->poolcount + 2] = (MYFLT) g_ksmps;
    csound->ekr = csound->pool[csound->poolcount + 1] = g_ekr;
    csound->onedkr = g_onedkr;
    csound->onedksmps = g_onedksmps;
    csound->kicvt = g_kicvt;
    csound->kcounter = g_kcounter;
    csound->pds = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip)                                         /* loop to last opds */
      while (csound->pds->nxtp) csound->pds = csound->pds->nxtp;
    return OK;
}

/* IV - Sep 17 2002 -- case 2: simplified routine for no local ksmps */

int useropcd2(CSOUND *csound, UOPCODE *p)
{
    OPDS    *saved_pds = csound->pds;
    int     n;
    MYFLT   **tmp, *ptr1, *ptr2;
    
     if (!(csound->pds = (OPDS*) (p->ip->nxtp))) goto endop; /* no perf code */

    //csound->Message(csound, "end input\n"); /* FOR SOME REASON the opcode has no perf code */ 
    /* IV - Nov 16 2002: update release flag */
    p->ip->relesing = p->parent_ip->relesing;
   
    tmp = p->buf->iobufp_ptrs;
    if (csound->ksmps != 1) {           /* generic case for kr != sr */

      /* copy inputs */
      while (*tmp) {                    /* a-rate */
        ptr1 = *(tmp++); ptr2 = *(tmp++);
        n = csound->ksmps;
        do {
          *(ptr2++) = *(ptr1++);
        } while (--n);
      }
   
      while (*(++tmp)) {                /* k-rate */
        ptr1 = *tmp; *(*(++tmp)) = *ptr1;
      }
  
      /* VL: fsigs in need to be dealt with here */
       while (*(++tmp)) {                
         ptr1 = *tmp; 
         memcpy((void *)(*(++tmp)), (void *) ptr1, sizeof(PVSDAT)); 
         } 
       /* VL: tsigs */
        while (*(++tmp)) {                
         ptr1 = *tmp; 
         memcpy((void *)(*(++tmp)), (void *) ptr1, sizeof(TABDAT)); 
         } 
       
        
      /*  run each opcode  */
      do {
        (*csound->pds->opadr)(csound, csound->pds);
      } while ((csound->pds = csound->pds->nxtp));
      /* copy outputs */
      while (*(++tmp)) {                /* a-rate */
        ptr1 = *tmp; ptr2 = *(++tmp);
        n = csound->ksmps;
        do {
          *(ptr2++) = *(ptr1++);
        } while (--n);
      }
    }
    else {                      /* special case for kr == sr */
      /* copy inputs */
      while (*tmp) {                    /* a-rate */
        ptr1 = *(tmp++); 
        *(*(tmp++)) = *ptr1;
      }
      while (*(++tmp)) {                /* k-rate */
        ptr1 = *tmp; *(*(++tmp)) = *ptr1;
      }
      /* VL: fsigs in need to be dealt with here */
       while (*(++tmp)) {                
         ptr1 = *tmp; 
         memcpy((void *)(*(++tmp)), (void *) ptr1, sizeof(PVSDAT)); 
         }
       /* VL: tsigs */
       while (*(++tmp)) {                
         ptr1 = *tmp; 
         memcpy((void *)(*(++tmp)), (void *) ptr1, sizeof(TABDAT)); 
         } 
      /*  run each opcode  */
      do {
        (*csound->pds->opadr)(csound, csound->pds);
      } while ((csound->pds = csound->pds->nxtp));
      /* copy outputs */
      while (*(++tmp)) {                /* a-rate */
        ptr1 = *tmp; *(*(++tmp)) = *ptr1;
      }
    }
    while (*(++tmp)) {                  /* k-rate */
      ptr1 = *tmp; *(*(++tmp)) = *ptr1;
    }
    /* VL: fsigs out need to be dealt with here */
     while (*(++tmp)) {                
       ptr1 = *tmp;
       memcpy((void *)(*(++tmp)), (void *)ptr1, sizeof(PVSDAT));
       }
     /* tsigs */
    while (*(++tmp)) {                
       ptr1 = *tmp;
       memcpy((void *)(*(++tmp)), (void *)ptr1, sizeof(TABDAT));
       } 
 endop:
    /* restore globals */
    csound->pds = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip)                                         /* loop to last opds */
      while (csound->pds->nxtp) csound->pds = csound->pds->nxtp;
    return OK;
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
    LBLBLK    **lopdsp;
    LARGNO    *largp;
    int       n, cnt, pextent, opnum;
    char      *nxtopds, *opdslim;
    MYFLT     **argpp, *lclbas, *gbloffbas, *lcloffbas;
    int       *ndxp;
    OPARMS    *O = csound->oparms;
    int       odebug = O->odebug;

    lopdsp = csound->lopds;
    largp = (LARGNO*) csound->larg;
    tp = csound->instrtxtp[insno];
    /* VL: added 2 extra MYFLT pointers to the memory to account for possible
       use by midi mapping flags */
    n = 3;
    if (O->midiKey>n) n = O->midiKey;
    if (O->midiKeyCps>n) n = O->midiKeyCps;
    if (O->midiKeyOct>n) n = O->midiKeyOct;
    if (O->midiKeyPch>n) n = O->midiKeyPch;
    if (O->midiVelocity>n) n = O->midiVelocity;
    if (O->midiVelocityAmp>n) n = O->midiVelocityAmp;
    pextent = sizeof(INSDS) + tp->pextrab + (n-3)*sizeof(MYFLT *);      /* alloc new space,  */
    ip = (INSDS*) mcalloc(csound, (size_t) pextent + tp->localen + tp->opdstot);
    ip->csound = csound;
    ip->m_chnbp = (MCHNBLK*) NULL;
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
    /* IV - Nov 10 2002 */
    if (insno > csound->maxinsno) {
      size_t pcnt = (size_t) tp->opcode_info->perf_incnt;
      pcnt += (size_t) tp->opcode_info->perf_outcnt;
      pcnt = sizeof(OPCOD_IOBUFS) + sizeof(MYFLT*) * (pcnt << 1);
      ip->opcod_iobufs = (void*) mmalloc(csound, pcnt);
    }
    gbloffbas = csound->gbloffbas;
    lcloffbas = &ip->p0;
    lclbas = (MYFLT*) ((char*) ip + pextent);   /* split local space */
    nxtopds = (char*) lclbas + tp->localen;
    opdslim = nxtopds + tp->opdstot;
    if (UNLIKELY(odebug))
      csound->Message(csound,
                      Str("instr %d allocated at %p\n\tlclbas %p, opds %p\n"),
                      insno, ip, lclbas, nxtopds);
    optxt = (OPTXT*) tp;
    prvids = prvpds = (OPDS*) ip;
    while ((optxt = optxt->nxtop) != NULL) {    /* for each op in instr */
      TEXT *ttp = &optxt->t;
      if ((opnum = ttp->opnum) == ENDIN         /*  (until ENDIN)  */
          || opnum == ENDOP)                    /*  (or ENDOP)     */
        break;
      if (opnum == PSET) {
        ip->p1 = (MYFLT) insno;
        continue;
      }
      ep = &(csound->opcodlst[opnum]);          /* for all ops:     */
      opds = (OPDS*) nxtopds;                   /*   take reqd opds */
      nxtopds += ep->dsblksiz;
      if (UNLIKELY(odebug))
        csound->Message(csound, Str("op %d (%s) allocated at %p\n"),
                                opnum, ep->opname, opds);
      opds->optext = optxt;                     /* set common headata */
      opds->insdshead = ip;
      if (opnum == LABEL) {                     /* LABEL:       */
        LBLBLK  *lblbp = (LBLBLK *) opds;
        lblbp->prvi = prvids;                   /*    save i/p links */
        lblbp->prvp = prvpds;
        *lopdsp++ = lblbp;                      /*    log the lbl bp */
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
            (ttp->pftype == 'k' && ep->kopadr != NULL))
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
      ndxp = ttp->outoffs->indx;                /* for outarg codes: */
      cnt = ttp->outoffs->count;
      for (n = 0; n < cnt; n++) {
        MYFLT *fltp;
        int   indx = ndxp[n];
        if (indx > 0)                           /* cvt index to lcl/gbl adr */
          fltp = gbloffbas + indx;
        else
          fltp = lcloffbas + (-indx);
        argpp[n] = fltp;
      }
      for ( ; ep->outypes[n] != (char) 0; n++)  /* if more outypes, pad */
        argpp[n] = NULL;
      ndxp = ttp->inoffs->indx;                 /* for inarg codes: */
      cnt = n + ttp->inoffs->count;
      for ( ; n < cnt; n++) {
        int   indx = *(ndxp++);
        if (indx > 0)                           /* cvt ndx to lcl/gbl */
          argpp[n] = gbloffbas + indx;
        else if (indx >= LABELIM)
          argpp[n] = lcloffbas + (-indx);
        else {                                  /* if label ref, defer */
          largp->lblno = indx - LABELOFS;
          largp->argpp = &(argpp[n]);
          largp++;
        }
      }
      if (UNLIKELY(odebug)) {
        csound->Message(csound, "argptrs:");
        cnt = ttp->outoffs->count;
        for (n = 0; n < cnt; n++)
          csound->Message(csound, "\t%p", (void*) argpp[n]);
        for ( ; ep->outypes[n] != (char) 0; n++)
          csound->Message(csound, "\tPADOUT");
        ndxp = ttp->inoffs->indx;
        cnt = n + ttp->inoffs->count;
        for ( ; n < cnt; n++) {
          int   indx = *(ndxp++);
          if (indx >= LABELIM)
            csound->Message(csound, "\t%p", (void*) argpp[n]);
          else
            csound->Message(csound, "\t***lbl");
        }
        csound->Message(csound, "\n");
      }
    }
    /* if (nxtopds != opdslim) { */
      /*      csound->Message(csound, Str("nxtopds = %p opdslim = %p\n"),
              nxtopds, opdslim); */
    if (UNLIKELY(nxtopds > opdslim))
      csoundDie(csound, Str("inconsistent opds total"));
    /* } */
    while (largp > (LARGNO*) csound->larg) {    /* now label refs */
      largp--;
      *largp->argpp = (MYFLT*) csound->lopds[largp->lblno];
    }
}

int prealloc(CSOUND *csound, AOP *p)
{
    int     n, a;

    n = (int) strarg2opcno(csound, p->r, (p->XSTRCODE & 1),
                                   (*p->b == FL(0.0) ? 0 : 1));
    if (UNLIKELY(n < 1))
      return NOTOK;
    a = (int) *p->a - csound->instrtxtp[n]->active;
    for ( ; a > 0; a--)
      instance(csound, n);
    return OK;
}

int delete_instr(CSOUND *csound, DELETEIN *p)
{
    int       n;
    INSTRTXT  *ip;
    INSDS     *active;
    INSTRTXT  *txtp;
    int isNamedInstr = (int) csound->GetInputArgSMask(p);

    if (isNamedInstr)
      n = csound->strarg2insno(csound, p->insno, isNamedInstr);
    else
      n = (int) (*p->insno + FL(0.5));

    if (n < 1 || n > csound->maxinsno || csound->instrtxtp[n] == NULL)
      return OK;                /* Instrument does not exist so noop */
    ip = csound->instrtxtp[n];
    active = ip->instance;
    while (active != NULL) {    /* Check there are no active instances */
      INSDS   *nxt = active->nxtinstance;
      if (UNLIKELY(active->actflg)) { /* Can only remove non-active instruments */
        char *name = csound->instrtxtp[n]->insname;
        if (name)
          return csound->InitError(csound,
                                   Str("Instrument %s is still active"), name);
        else
          return csound->InitError(csound,
                                   Str("Instrument %d is still active"), n);
      }
#if 0
      if (active->opcod_iobufs && active->insno > csound->maxinsno)
        mfree(csound, active->opcod_iobufs);            /* IV - Nov 10 2002 */
#endif
      if (active->fdchp != NULL)
        fdchclose(csound, active);
      if (active->auxchp != NULL)
        auxchfree(csound, active);
      mfree(csound, active);
      active = nxt;
    }
    csound->instrtxtp[n] = NULL;
    /* Now patch it out */
    for (txtp = &(csound->instxtanchor); txtp != NULL; txtp = txtp->nxtinstxt)
      if (txtp->nxtinstxt == ip) {
        OPTXT *t = ip->nxtop;
        txtp->nxtinstxt = ip->nxtinstxt;
        while (t) {
          OPTXT *s = t->nxtop;
          mfree(csound, t);
          t = s;
        }
        mfree(csound, ip);
        return OK;
      }
    return NOTOK;
}


