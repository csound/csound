/*
insert.c:

Copyright (C) 1991, 1997, 1999 2002
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

#include "cs.h"                 /*                              INSERT.C        */
#include "insert.h"     /* for goto's */
#include "aops.h"       /* for cond's */
#include "midiops.h"
#include "namedins.h"   /* IV - Oct 31 2002 */

#ifdef mills_macintosh
#include "MacTransport.h"
#endif

static MYFLT cpu_power_busy=FL(0.0);  /* accumulates the supposed percent of cpu usage */
/* extern  INSDS *insalloc[]; */
extern  OPARMS  O;

INSDS   *instance(int);

/* glob int     tieflag = 0;               toggled by insert for tigoto */
static  int     reinitflag = 0;         /* toggled by reinit for others */
static  OPDS    *ids, *pds;             /* used by init and perf loops  */
					/*  & modified by igoto, kgoto  */
static  OPDS    opdstmp;
void    showallocs(void);
extern  void    putop(TEXT*);
void    deact(INSDS *), schedofftim(INSDS *);
int     sensOrcEvent(void);             /* For triginstr (re Aug 1999)  */
extern  int      csoundYield(void*);

void insertRESET(void)
{
    memset(&actanchor,0,sizeof(INSDS));
    curip          = NULL;
    frstoff        = NULL;
    kcounter       = 0;
    inerrcnt       = 0;
    perferrcnt     = 0;
    tieflag        = 0;
    reinitflag     = 0;
    ids            = NULL;
    pds            = NULL;
    cpu_power_busy = FL(0.0);
    /* don't forget local externs in this file... */
}

int init0(void)
{
    INSDS  *ip;

    curip = ip = instance(0);               /* allocate instr 0     */
    ids = (OPDS *)ip;
    while ((ids = ids->nxti) != NULL) {
      (*ids->iopadr)(ids);                  /*   run all i-code     */
    }
    return(inerrcnt);                       /*   return errcnt      */
}

int
insert(int insno, EVTBLK *newevtp)  /* insert an instr copy into active list */
{                                   /*      then run an init pass            */
    INSTRTXT *tp;
    INSDS  *ip, *prvp, *nxtp;
    int err = 0;

    if (O.odebug) printf("activating instr %d\n",insno);
    inerrcnt = 0;
    tp = instrtxtp[insno];
    if (tp->muted==0) {
      printf("Instrument %d muted\n", insno);
      /*       if (O.odebug) printf("Instrument %d muted\n", insno); */
      return 0;
    }
    if (tp->mdepends & 04) {
      printf(Str(X_925,
                 "instr %d expects midi event data, cannot run from score\n"),
             insno);
      return(1);
    }
    if ((ip = tp->instance) != NULL) {      /* if allocs of text exist: */
      do {
        if (ip->insno == insno &&      /*   if find this insno,  */
            ip->actflg         &&      /*      active            */
            ip->offtim < 0     &&      /*      with indef (tie)  */
            ip->p1 == newevtp->p[1]) { /*  & matching p1  */
          tieflag++;
          goto init;      /*     continue that event */
        }
      } while ((ip = ip->nxtinstance) != NULL);
      ip = tp->instance;              /*   else get alloc of text */
      do {
        if (!ip->actflg)        /*      that is free        */
          goto actlnk;    /*      and use its space   */
      } while ((ip = ip->nxtinstance) != NULL);
    }
    /* RWD: screen writes badly slow down RT playback */
    if (O.msglevel & 2) printf(Str(X_1013,"new alloc for instr %d:\n"),insno);
    ip = instance(insno);                   /* else alloc new dspace  */

 actlnk:
    cpu_power_busy += instrtxtp[insno]->cpuload;
    if (cpu_power_busy > FL(100.0)) { /* if there is no more cpu processing time*/
      cpu_power_busy -= instrtxtp[insno]->cpuload;
      if (O.msglevel & WARNMSG)
        printf(Str(X_26,
                   "WARNING: cannot allocate last note because it exceeds "
                   "100%% of cpu time"));
      return(0);
    }
    /* Add an active instrument */
    if (instrtxtp[insno]->active++ > instrtxtp[insno]->maxalloc &&
        instrtxtp[insno]->maxalloc>0) {
      instrtxtp[insno]->active--;
      if (O.msglevel & WARNMSG)
        printf(Str(X_27,
                   "WARNING: cannot allocate last note because it exceeds instr maxalloc"));
      return(0);
    }

#ifdef TESTING
    printf("Now %d active instr %d\n", instrtxtp[insno]->active, insno);
#endif
    ip->insno = insno;
    nxtp = &actanchor;                      /* now splice into activ lst */
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
        MYFLT *pfld = &ip->p3;              /* if pset data present */
        MYFLT *pdat = tp->psetdata + 2;
        long nn = tp->pmax - 2;             /*   put cur vals in pflds */
        do {
          *pfld++ = *pdat++;
        } while (nn--);
      }
      if ((n = tp->pmax) != newevtp->pcnt && !tp->psetdata &&
          (O.msglevel & WARNMSG)) {
        sprintf(errmsg,Str(X_928,"instr %d uses %d p-fields but is given %d"),
                insno, n, newevtp->pcnt);
        printf(Str(X_526,"WARNING: %s\n"), errmsg);
      }
      if (newevtp->p3orig >= FL(0.0))
        ip->offbet = newevtp->p2orig + newevtp->p3orig;
      else ip->offbet = FL(-1.0);
      flp = &ip->p1;
      fep = &newevtp->p[1];
      if (O.odebug) printf("psave beg at %p\n",flp);
      if (n > newevtp->pcnt) n = newevtp->pcnt; /* IV - Oct 20 2002 */
      memcpy(flp, fep, n * sizeof(MYFLT)); flp += n;
      if (n < tp->pmax) memset(flp, 0, (tp->pmax - n) * sizeof(MYFLT));
      if (O.odebug) printf("   ending at %p\n",flp);
    }
    ip->offtim       = ip->p3;                    /* & duplicate p3 for now */
    ip->xtratim      = 0;
    ip->relesing     = 0;
    ip->m_chnbp      = (MCHNBLK*) NULL;  /* IV May 2002 */
    ip->opcod_iobufs = NULL;        /* IV - Sep 8 2002 */
    curip            = ip;
    ids              = (OPDS *)ip;
    while ((ids = ids->nxti) != NULL) {   /* do init pass for this instr */
      if (O.odebug) printf("init %s:\n", opcodlst[ids->optext->t.opnum].opname);
      err |= (*ids->iopadr)(ids);      /* $$$$ CHECK RETURN CODE $$$$ */
    }
    tieflag = 0;
    if (err || !ip->p3) {
      deact(ip);
      return(inerrcnt);
    }
    if (ip->p3 > FL(0.0) && ip->offtim > FL(0.0)) {   /* if still finite time, */
      ip->offtim = ip->p2 + ip->p3;
      schedofftim(ip);                    /*   put in turnoff list */
    }
    else ip->offtim = -FL(1.0);                      /* else mark indef */
    if (O.odebug) {
      printf("instr %d now active:\n",insno); showallocs();
    }
    return(0);
}

int MIDIinsert(int insno, MCHNBLK *chn, MEVENT *mep)
/* insert a MIDI instr copy into active list */
/*  then run an init pass           */
{
    INSTRTXT *tp;
    INSDS    *ip, **ipp;
    int err = 0;

    cpu_power_busy += instrtxtp[insno]->cpuload;
    if (cpu_power_busy > FL(100.0)) { /* if there is no more cpu processing time*/
      cpu_power_busy -= instrtxtp[insno]->cpuload;
      if (O.msglevel & WARNMSG)
        printf(Str(X_26,
                   "WARNING: cannot allocate last note because "
                   "it exceeds 100%% of cpu time"));
      return(0);
    }
    if (O.odebug) printf("activating instr %d\n",insno);
    inerrcnt = 0;
    ipp = &chn->kinsptr[mep->dat1];       /* key insptr ptr           */
    tp = instrtxtp[insno];
    if (tp->instance != NULL) {           /* if allocs of text exist  */
      INSDS **spp;
      if ((ip = *ipp) != NULL) {          /*   if key currently activ */
        if (ip->xtratim == 0)             /*     if decay not needed  */
          goto m_dat2;                    /*        usurp curr space  */
        else goto forcdec;                /*     else force a decay   */
      }
      spp = ipp + 128;                    /*   (struct dependent ! )  */
      if ((ip = *spp) != NULL) {          /*   else if pch sustaining */
        *spp = NULL;                      /*     remov from sus array */
        chn->ksuscnt--;
        if (ip->xtratim == 0) {
          *ipp = ip;
          goto m_dat2;
        }
        else {
        forcdec:
          ip->relesing = 1;               /*     else force a decay   */
          ip->offtim = (kcounter + ip->xtratim) * onedkr;
          schedofftim(ip);
          Mforcdecs++;
        }
      }                                   /*         & get new space  */
      ip = tp->instance;                  /*    srch existing allocs  */
      do {
        if (!ip->actflg)                  /*      if one is free      */
          goto actlnk;                    /*      then use its space  */
      } while ((ip = ip->nxtinstance) != NULL);
    }
    printf(Str(X_1013,"new alloc for instr %d:\n"),insno);
    ip = instance(insno);                 /* else alloc new dspace  */

 actlnk:
    ip->insno = insno;
    if (instrtxtp[insno]->active++ > instrtxtp[insno]->maxalloc &&
        instrtxtp[insno]->maxalloc>0) {
      instrtxtp[insno]->active--;
      if (O.msglevel & WARNMSG)
        printf(Str(X_27,
                   "WARNING: cannot allocate last note because "
                   "it exceeds instr maxalloc"));
      return(0);
    }
    if (O.odebug)
      printf("Now %d active instr %d\n", instrtxtp[insno]->active, insno);
    {
      INSDS  *prvp, *nxtp;       /* now splice into activ lst */
      nxtp = &actanchor;
      while ((prvp = nxtp) && (nxtp = prvp->nxtact) != NULL) {
        if (nxtp->insno > insno) {
          nxtp->prvact = ip;
          break;
        }
      }
      ip->nxtact = nxtp;
      ip->prvact = prvp;
      prvp->nxtact = ip;
    }
    ip->actflg++;            /*    and mark the instr active */
    if (tp->pmax > 3 && tp->psetdata==NULL &&
        (O.msglevel & WARNMSG)) {
      sprintf(errmsg,Str(X_927,"instr %d p%d illegal for MIDI"),
              insno, tp->pmax);
      printf(Str(X_526,"WARNING: %s\n"), errmsg);
    }
    ip->m_chnbp = chn;       /* rec address of chnl ctrl blk */
    *ipp = ip;               /* insds ptr for quick midi-off */
    ip->m_pitch = mep->dat1; /* rec MIDI data                */
 m_dat2:
    ip->m_veloc = mep->dat2;
    ip->xtratim = 0;
    ip->relesing = 0;
    ip->offtim = -FL(1.0);    /* set indef duration */
    ip->opcod_iobufs = NULL;        /* IV - Sep 8 2002:            */
    ip->p1 = (MYFLT) ip->insno;     /* set these required p-fields */
    ip->p2 = (MYFLT) kcounter * onedkr;
    ip->p3 = FL(-1.0);
    if (tp->psetdata != NULL) {
      MYFLT *pfld = &ip->p3;              /* if pset data present */
      MYFLT *pdat = tp->psetdata + 2;
      long nn = tp->pmax - 2;             /*   put cur vals in pflds */
      do {
        *pfld++ = *pdat++;
      } while (nn--);
    }
    curip = ip;
    ids = (OPDS *)ip;
    while ((ids = ids->nxti) != NULL) { /* do init pass for this instr  */
      if (O.odebug) printf("init %s:\n", opcodlst[ids->optext->t.opnum].opname);
      err |= (*ids->iopadr)(ids); /* $$$ CHECK RETURN CODE $$$ */
    }
    tieflag = 0;
    if (err) {
      deact(ip);
      return(inerrcnt);
    }
    if (O.odebug) {
      printf("instr %d now active:\n",insno); showallocs();
    }
    return(0);
}

void showallocs(void)    /* debugging aid        */
{
    INSTRTXT *txtp;
    INSDS   *p;

    printf("insno\tinstanc\tnxtinst\tprvinst\tnxtact\t"
           "prvact\tnxtoff\tactflg\tofftim\n");
    for (txtp = &instxtanchor;  txtp != NULL;  txtp = txtp->nxtinstxt)
      if ((p = txtp->instance) != NULL) {
        /*
         * On Alpha, we print pointers as pointers.  heh 981101
         * and now on all platforms (JPff)
         */
        do {
          printf("%d\t%p\t%p\t%p\t%p\t%p\t%p\t%d\t%3.1f\n",
                 (int)p->insno, p,
                 p->nxtinstance,
                 p->prvinstance, p->nxtact,
                 p->prvact, p->nxtoff,
                 p->actflg, p->offtim);
        } while ((p = p->nxtinstance) != NULL);
      }
}

void schedofftim(INSDS *ip)     /* put an active instr into offtime list  */
                                /* called by insert() & midioff + xtratim */
{
    INSDS *prvp, *nxtp;

    if ((nxtp = frstoff) == NULL ||
        nxtp->offtim > ip->offtim)          /*   set into       */
      frstoff = ip;                         /*   firstoff chain */
    else {
      while ((prvp = nxtp)
             && (nxtp = nxtp->nxtoff) != NULL
             && ip->offtim >= nxtp->offtim);
      prvp->nxtoff = ip;
    }
    ip->nxtoff = nxtp;
}


void insxtroff(short insno)     /* deactivate all schedofftim copies    */
{                               /*  (such as xtratims still playing)    */
    INSDS *ip, *prvp = NULL;

    for (ip = frstoff; ip != NULL; ip = ip->nxtoff) {
      if (ip->insno == insno && ip->actflg) {
        deact(ip);
        if (frstoff == ip)
          frstoff = ip->nxtoff;
        else prvp->nxtoff = ip->nxtoff;
      }
      else prvp = ip;
    }
}

int useropcd(UOPCODE*);        /* IV - Oct 26 2002 */

void deact(INSDS *ip)           /* unlink single instr from activ chain */
                                /*      and mark it inactive            */
{                               /*   close any files in fd chain        */
    INSDS  *nxtp;
    /*  OPDS   *p;          IV - Oct 24 2002 */

    /*     printf("active(%d) = %d\n", ip->insno, instrtxtp[ip->insno]->active); */
    instrtxtp[ip->insno]->active--;     /* remove an active instrument */
    cpu_power_busy -= instrtxtp[ip->insno]->cpuload;
    /* IV - Sep 8 2002: free subinstr instances */
    /* that would otherwise result in a memory leak */
    if (ip->opcod_deact) {
      UOPCODE *p = (UOPCODE*) ip->opcod_deact;          /* IV - Oct 26 2002 */
      deact(p->ip);             /* deactivate */
      /* link into free instance chain */
      p->ip->nxtact = instrtxtp[p->ip->insno]->act_instance;
      instrtxtp[p->ip->insno]->act_instance = p->ip;
      p->ip = NULL;
      /* IV - Oct 26 2002: set perf routine to "not initialised" */
      p->h.opadr = (SUBR) useropcd;
      ip->opcod_deact = NULL;
    }
    if (ip->subins_deact) {
      deact(((SUBINST*) ip->subins_deact)->ip);         /* IV - Oct 24 2002 */
      ((SUBINST*) ip->subins_deact)->ip = NULL;
      ip->subins_deact = NULL;
    }
    if (O.odebug) printf("removed instance of instr %d\n", ip->insno);
    /* IV - Oct 24 2002: ip->prvact may be NULL, so need to check */
    if (ip->prvact && (nxtp = ip->prvact->nxtact = ip->nxtact) != NULL)
      nxtp->prvact = ip->prvact;
    ip->actflg = 0;
    if (ip->fdch.nxtchp != NULL)
      fdchclose(ip);
}

int ihold(LINK *p)                      /* make this note indefinit duration */
{                                       /* called by ihold statmnt at Itime */
    if (!reinitflag) {                  /* no-op at reinit                  */
      curip->offbet = -FL(1.0);
      curip->offtim = -FL(1.0);
    }
    return OK;
}

int turnoff(LINK *p)                    /* terminate the current instrument  */
{                                       /* called by turnoff statmt at Ptime */
    INSDS  *lcurip = pds->insdshead;
    /* IV - Oct 16 2002: check for subinstr and user opcode */
    /* find top level instrument instance */
    while (lcurip->opcod_iobufs)
      lcurip = ((OPCOD_IOBUFS*) lcurip->opcod_iobufs)->parent_ip;
    if (lcurip->xtratim) {               /* if extra time needed:  */
      INSDS *nxt;
      MYFLT oldtim = lcurip->offtim;
      MYFLT newtim = (kcounter + lcurip->xtratim) * onedkr;
      /* printf("Turnoff called %d offtime %f->%f\n", lcurip->xtratim,oldtim,newtim); */
      lcurip->relesing = 1;
      lcurip->offtim = newtim;
      if (oldtim < FL(0.0))             /* if indef duratn instr   */
        schedofftim(lcurip);             /*    schedoff at new time */
      else if ((nxt = lcurip->nxtoff) != NULL
               && newtim > nxt->offtim) {
        INSDS *prv, *newip = nxt;       /* else relink if reqd  */
        while ((prv = nxt)
               && (nxt = nxt->nxtoff) != NULL
               && newtim > nxt->offtim);
        prv->nxtoff = lcurip;
        lcurip->nxtoff = nxt;
        if (lcurip == frstoff)
          frstoff = newip;
      }
      lcurip->xtratim--;         /* Decay the delay */
    }
    else {                                /* no extra time needed:  */
      INSDS   *ip, *prvip;
      MCHNBLK *chn;
      short   pch;
      deact(lcurip);                       /* deactivate immediately */
      if ((chn = lcurip->m_chnbp) != NULL
          && (pch = lcurip->m_pitch)) {    /* if this was a MIDI note */
        INSDS **ipp = &chn->kinsptr[pch];
        if (*ipp == lcurip) *ipp = NULL;   /*    remov from activ lst */
        else if (chn->ksuscnt) {
          ipp += 128;                     /* STRUCT DEPEND */
          if (*ipp == lcurip) {
            *ipp = NULL;                  /*    or from sustain list */
            chn->ksuscnt--;
          }
        }
      }
      if (lcurip->offtim >= FL(0.0)      /* skip indefinite durs    */
          && (ip = frstoff) != NULL) {
        if (ip == lcurip)                /* else rm from nxtoff chn */
          frstoff = ip->nxtoff;
        else while ((prvip = ip) && (ip = ip->nxtoff) != NULL)
          if (ip == lcurip) {
            prvip->nxtoff = ip->nxtoff;
            break;
          }
      }
    }
    return OK;
}

void xturnoff(INSDS *ip)                 /* turnoff a particular insalloc */
{                                        /* called by inexclus on ctrl 111 */
    pds = &opdstmp;
    pds->insdshead = ip;
    turnoff(NULL);
}

#ifdef never
void insdealloc(short insno)      /* dealloc all instances of an insno */
{                                 /*   called by midirecv on pgm_chng  */
    INSDS   *ip, *nxtip;

    if ((ip = instrtxtp[insno]->instance) != NULL) {
      do {                              /* for all instances: */
        if (ip->actflg)                 /* if active, deact   */
          deact(ip);                    /*    & close files   */
        if (ip->auxch.nxtchp != NULL)
          auxchfree(ip);                /* free auxil space   */
        nxtip = ip->nxtinstance;
        free((char *)ip);               /* & free the INSDS   */
      } while ((ip = nxtip) != NULL);
      instrtxtp[insno]->instance = NULL;/* now there are none */
    }
}
#endif

void orcompact(void)                    /* free all inactive instr spaces */
{
    INSTRTXT *txtp;
    INSDS   *ip, *nxtip, *prvip, **prvnxtloc;

    for (txtp = &instxtanchor;  txtp != NULL;  txtp = txtp->nxtinstxt) {
      if ((ip = txtp->instance) != NULL) {        /* if instance exists */
        prvip = NULL;
        prvnxtloc = &txtp->instance;
        do {
          if (ip->actflg == 0) {
            OPDS* off = ip->nxtp;
            while (off!=NULL) {
              if (off->dopadr) (*off->dopadr)(off);
              off = off->nxtp;
            }

            /* SYY - 2003.11.30
             * call deinitialization on i-time opcodes
             */
            off = ip->nxti;
            while (off != NULL) {
              if(off->dopadr) (*off->dopadr)(off);
              off = off->nxti;
            }

            if (ip->opcod_iobufs && ip->insno > maxinsno)
              mfree(ip->opcod_iobufs);                  /* IV - Nov 10 2002 */
            if (ip->fdch.nxtchp != NULL)
              fdchclose(ip);
            if (ip->auxch.nxtchp != NULL)
              auxchfree(ip);
            if ((nxtip = ip->nxtinstance) != NULL)
              nxtip->prvinstance = prvip;
            *prvnxtloc = nxtip;
            mfree((char *)ip);
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
    printf(Str(X_897,"inactive allocs returned to freespace\n"));
}

void infoff(MYFLT p1)           /*  turn off an indef copy of instr p1  */
{                               /*      called by musmon                */
    INSDS *ip;
    int   insno;

    insno = (int)p1;
    if ((ip = (instrtxtp[insno])->instance) != NULL) {
      do {
        if (ip->insno == insno          /* if find the insno */
            && ip->actflg                 /*      active       */
            && ip->offtim < 0             /*      but indef,   */
            && ip->p1 == p1) {
          if (ip->xtratim && ip->offbet < 0) {
            /* Gab: delays the off event is xtratim > 0 */
#include "schedule.h"
            MYFLT starttime;
            EVTNODE *evtlist, *newnode;
            EVTBLK  *newevt;

            newnode         = (EVTNODE *) mmalloc((long)sizeof(EVTNODE));
            newevt          = &newnode->evt;
            newevt->opcod   = 'i';
            starttime       = (MYFLT) (kcounter + ip->xtratim) * onedkr;
            newnode->kstart = kcounter + ip->xtratim;
            newevt->p2orig  = starttime;
            newevt->p3orig  = FL(0.0);
            newevt->p[1]    = (MYFLT) -insno; /* negative p1 */
            newevt->p[2]    = starttime;    /* Set actual start time in p2 */
            newevt->p[3]    = FL(0.0);
            newevt->pcnt    = 2;
            newnode->insno  = insno;
            evtlist         = &OrcTrigEvts;
            while (evtlist->nxtevt) {
              if (newnode->kstart < evtlist->nxtevt->kstart) break;
              evtlist = evtlist->nxtevt;
            }
            newnode->nxtevt = evtlist->nxtevt;
            evtlist->nxtevt = newnode;
            O.RTevents      = 1;     /* Make sure kperf() looks for RT events */
            O.ksensing      = 1;
            O.OrcEvts       = 1;
            ip->offbet      = 0; /* to avoid another check */
            ip->relesing    = 1;
            return;
          }
          if (O.odebug) printf("turning off inf copy of instr %d\n",insno);
          deact(ip);
          return;                     /*  turn it off */
        }
      } while ((ip = ip->nxtinstance) != NULL);
    }
    printf(Str(X_669,"could not find indefinitely playing instr %d\n"),insno);
}

#ifdef never
#define numClockPoints 32
static double clocksPerFrame[numClockPoints];
static double clocks     = 0;
static int cpfIndex      = 0;
static int temp          = 0;
static clock_t lastClock = 0;
#endif

long kperf(long kcnt)   /* perform currently active instrs for kcnt kperiods */
/*      & send audio result to output buffer    */
{
    extern  int     sensLine(void);
    extern  int     sensMidi(void), sensFMidi(void);
    extern  void    (*spinrecv)(void), (*spoutran)(void), (*nzerotran)(long);
    INSDS  *ip;
    long    kreq = kcnt;
#ifdef never
    clock_t clockStart = clock();
    clock_t clockEnd;
#endif

    if (O.odebug) printf("perfing %ld kprds\n",kcnt);
    if (!csoundYield(&cenviron)) longjmp(cenviron.exitjmp_,1); /* PC GUI needs attention */
    if (!O.ksensing &&
        actanchor.nxtact == NULL) {  /* if !kreads & !instrs_activ, */
      kcounter += kcnt;
      global_kcounter = kcounter;       /* IV - Sep 8 2002 */
#if defined(mills_macintosh) || defined(SYMANTEC)
      /* silence here, so we can afford to check events more often */
      {
        long pcnt = kcnt;
        while (pcnt!=0) {
          long pp = (pcnt>128 ? 128 : pcnt);
          (*nzerotran)(pp);     /*   send chunk up to kcnt zerospouts  */
          if (!csoundYield(&cenviron)) longjmp(cenviron.exitjmp_,1);
          pcnt -= pp;
        }
      }
#else
      (*nzerotran)(kcnt);     /*   send kcnt zerospouts  */
#endif
    }
    else do {                 /* else for each kcnt:     */
      if (O.RTevents) {
        if ((O.Midiin && (sensType = sensMidi())) /*   if MIDI note message  */
            || (O.FMidiin && kcounter >= FMidiNxtk && (sensType = sensFMidi()))
            || (O.Linein && (sensType = sensLine())) /* or Linein event */
            || (O.OrcEvts && (sensType = sensOrcEvent()))) /* or triginstr event (re Aug 1999) */
          return(kreq - kcnt); /*      do early return    */
      }
      /* #if defined(mills_macintosh) || defined(SYMANTEC) */
      /*       else if (O.Midiin && actanchor.nxtact == NULL) /\* no midi or notes on; check events *\/ */
      /* #endif */
      if (!csoundYield(&cenviron)) longjmp(cenviron.exitjmp_,1);
      kcounter += 1;
      global_kcounter = kcounter;       /* IV - Sep 8 2002 */
      if (O.sfread)           /*   if audio_infile open  */
        (*spinrecv)();        /*      fill the spin buf  */
      spoutactive = 0;        /*   make spout inactive   */
      ip = &actanchor;
      while ((ip = ip->nxtact) != NULL) { /*   for each instr active */
        pds = (OPDS *)ip;
        while ((pds = pds->nxtp) != NULL) {
          (*pds->opadr)(pds); /*      run each opcode    */
        }
      }
      if (spoutactive)        /*   results now in spout? */
        (*spoutran)();        /*      send to audio_out  */
      else (*nzerotran)(1L);  /*   else send zerospout   */
    } while (--kcnt);         /* on Mac/Win, allow system events */
#ifdef never
    clockEnd = clock();
    clocksPerFrame[cpfIndex] = (clockEnd - clockStart)/(double)(kreq*ksmps);
    if (++cpfIndex == numClockPoints)
      cpfIndex = 0;
    /* average this into our tally */
    clocks = clocks*.9 + (clockEnd - clockStart)*.1/(double)(kreq*ksmps);
    /* test printouts */
    /*  temp += kreq;
<<<<<<< insert.c
	if (temp > ekr*.05) {
	int i;
	double total = 0;

	for (i = 0; i < numClockPoints; i++)
	total += clocksPerFrame[i];

	printf("cpu load is: %f\n", esr*total/(double)(numClockPoints*CLOCKS_PER_SEC));
	printf("cpu load[filt]: %f\n", esr*clocks/(double)(CLOCKS_PER_SEC));
	printf("rendering rate is: %f\n", (double)(CLOCKS_PER_SEC*temp)/(ekr*(clockEnd - lastClock)));
	lastClock = clockEnd;
	temp = 0;
	}
	*/
=======
        if (temp > ekr*.05) {
        int i;
        double total = 0;

        for (i = 0; i < numClockPoints; i++)
        total += clocksPerFrame[i];

        printf("cpu load is: %f\n", esr*total/(double)(numClockPoints*CLOCKS_PER_SEC));
        printf("cpu load[filt]: %f\n", esr*clocks/(double)(CLOCKS_PER_SEC));
        printf("rendering rate is: %f\n", (double)(CLOCKS_PER_SEC*temp)/(ekr*(clockEnd - lastClock)));
        lastClock = clockEnd;
        temp = 0;
        }
        */
>>>>>>> 1.11
#endif
    return(kreq);
}

int initerror(char *s)
{
    INSDS *ip;

    /* RWD: need this! */
    if (ids==NULL) {
      dies(Str(X_551,"\nINIT ERROR: %s\n"),s);
    }
    /* IV - Oct 16 2002: check for subinstr and user opcode */
    ip = ids->insdshead;
    if (ip->opcod_iobufs) {
      OPCOD_IOBUFS *buf = (OPCOD_IOBUFS*) ip->opcod_iobufs;
      /* find top level instrument instance */
      while (ip->opcod_iobufs)
        ip = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->parent_ip;
      if (buf->opcode_info)
        printf(Str(X_1798,"INIT ERROR in instr %d (opcode %s): %s\n"),
               ip->insno, buf->opcode_info->name, s);
      else
        printf(Str(X_1799,"INIT ERROR in instr %d (subinstr %d): %s\n"),
               ip->insno, ids->insdshead->insno, s);
    }
    else
      printf(Str(X_299,"INIT ERROR in instr %d: %s\n"), ip->insno, s);
    putop(&ids->optext->t);
    inerrcnt++;
    return inerrcnt;
}

int perferror(char *s)
{
    INSDS *ip;

    /*RWD and probably this too... */
    if (pds==NULL) {
      dies(Str(X_553,"\nPERF ERROR: %s\n"),s);
    }
    /* IV - Oct 16 2002: check for subinstr and user opcode */
    ip = pds->insdshead;
    if (ip->opcod_iobufs) {
      OPCOD_IOBUFS *buf = (OPCOD_IOBUFS*) ip->opcod_iobufs;
      /* find top level instrument instance */
      while (ip->opcod_iobufs)
        ip = ((OPCOD_IOBUFS*) ip->opcod_iobufs)->parent_ip;
      if (buf->opcode_info)
        printf(Str(X_1800,"PERF ERROR in instr %d (opcode %s): %s\n"),
               ip->insno, buf->opcode_info->name, s);
      else
        printf(Str(X_1801,"PERF ERROR in instr %d (subinstr %d): %s\n"),
               ip->insno, pds->insdshead->insno, s);
    }
    else
      printf(Str(X_403,"PERF ERROR in instr %d: %s\n"), ip->insno, s);
    putop(&pds->optext->t);
    printf(Str(X_7,"   note aborted\n"));
    perferrcnt++;
    deact(ip);                                /* rm ins fr actlist */
    while (pds->nxtp != NULL)
      pds = pds->nxtp;                        /* loop to last opds */
    return perferrcnt;
}                                             /* contin from there */

int igoto(GOTO *p)
{
    ids = p->lblblk->prvi;
    return OK;
}

int kgoto(GOTO *p)
{
    pds = p->lblblk->prvp;
    return OK;
}

int icgoto(CGOTO *p)
{
    if (*p->cond)
      ids = p->lblblk->prvi;
    return OK;
}

int kcgoto(CGOTO *p)
{
    if (*p->cond)
      pds = p->lblblk->prvp;
    return OK;
}

/* an 'if-then' variant of 'if-goto' */
int ingoto(CGOTO *p)
{
    /* Make sure we have an i-time conditional */
    if (p->h.optext->t.intype == 'b' && !*p->cond)
      pds = p->lblblk->prvp;
    return OK;
}

int kngoto(CGOTO *p)
{
    if (!*p->cond)
      pds = p->lblblk->prvp;
    return OK;
}

int timset(TIMOUT *p)
{
    if ((p->cnt1 = (long)(*p->idel * ekr + FL(0.5))) < 0L
        || (p->cnt2 = (long)(*p->idur * ekr + FL(0.5))) < 0L)
      return initerror(Str(X_1012,"negative time period"));
    return OK;
}

int timout(TIMOUT *p)
{
    if (p->cnt1)                            /* once delay has expired, */
      p->cnt1--;
    else if (--p->cnt2 >= 0L)               /*  br during idur countdown */
      pds = p->lblblk->prvp;
    return OK;
}

int rireturn(LINK *p)
{
    IGN(p);
    return OK;
}

int reinit(GOTO *p)
{
    reinitflag = 1;
    curip = p->h.insdshead;
    ids = p->lblblk->prvi;        /* now, despite ANSI C warning:  */
    while ((ids = ids->nxti) != NULL && ids->iopadr != (SUBR)rireturn)
      (*ids->iopadr)(ids);
    reinitflag = 0;
    return OK;
}

int rigoto(GOTO *p)
{
    if (reinitflag)
      ids = p->lblblk->prvi;
    return OK;
}

int tigoto(GOTO *p)                    /* I-time only, NOP at reinit */
{
    if (tieflag && !reinitflag)
      ids = p->lblblk->prvi;
    return OK;
}

int tival(EVAL *p)                     /* I-time only, NOP at reinit */
{
    if (!reinitflag)
      *p->r = (tieflag ? FL(1.0) : FL(0.0));
    return OK;
}

/* IV - Oct 12 2002: new simplified subinstr functions */

int subinstrset(SUBINST *p)
{
    OPDS    *saved_ids = ids;
    INSDS   *saved_curip = curip;
    MYFLT   *flp;
    int     instno, n, init_op, inarg_ofs;

    /* check if we are using subinstrinit or subinstr */
    init_op = (p->h.opadr == NULL ? 1 : 0);
    inarg_ofs = (init_op ? 0 : SUBINSTNUMOUTS);
    /* IV - Oct 31 2002 */
    if ((instno = strarg2insno(p->ar[inarg_ofs], p->STRARG)) < 0) return OK;
    /* IV - Oct 9 2002: need this check */
    if (!init_op && p->OUTOCOUNT > nchnls) {
      return initerror(Str(X_1802,"subinstr: number of output args grester than nchnls"));
    }
    /* IV - Oct 9 2002: copied this code from useropcdset() to fix some bugs */
    if (!(reinitflag | tieflag)) {
      /* search for already allocated, but not active instance */
      p->ip = instrtxtp[instno]->instance;
      while (p->ip && p->ip->actflg) p->ip = p->ip->nxtinstance;
      /* if none was found, allocate a new instance */
      if (p->ip == NULL) p->ip = instance(instno);
      p->ip->actflg++;                  /*    and mark the instr active */
      instrtxtp[instno]->active++;
      p->ip->p1 = p->ip->insno = instno;
      p->ip->opcod_iobufs = (void*) &p->buf;
      /* link into deact chain */
      p->ip->subins_deact = saved_curip->subins_deact;
      p->ip->opcod_deact = NULL;
      saved_curip->subins_deact = (void*) p;
      p->parent_ip = p->buf.parent_ip = saved_curip;
    }
    /* copy parameters from this instrument into our subinstrument */
    p->ip->xtratim = saved_curip->xtratim;
    p->ip->relesing = saved_curip->relesing;
    p->ip->offbet = saved_curip->offbet;
    p->ip->offtim = saved_curip->offtim;
    p->ip->p2 = saved_curip->p2;
    p->ip->p3 = saved_curip->p3;

    /* IV - Oct 31 2002 */
    p->ip->m_chnbp = saved_curip->m_chnbp;
    p->ip->m_pitch = saved_curip->m_pitch;
    p->ip->m_veloc = saved_curip->m_veloc;

    /* copy remainder of pfields        */
    flp = &p->ip->p3 + 1;
    /* by default all inputs are i-rate mapped to p-fields */
    if (p->INOCOUNT > (instrtxtp[instno]->pmax + 1)) {  /* IV - Nov 10 2002 */
      return initerror(Str(X_1803,"subinstr: too many p-fields"));
    }
    for (n = 1; n < p->INOCOUNT; n++)
      *flp++ = *p->ar[inarg_ofs + n];

    /* allocate memory for a temporary store of spout buffers */
    if (!init_op && !(reinitflag | tieflag))
      auxalloc((long)nspout*sizeof(MYFLT), &p->saved_spout);

    /* do init pass for this instr */
    curip = p->ip;
    ids = (OPDS *)p->ip;

    while ((ids = ids->nxti) != NULL) {
      (*ids->iopadr)(ids);
    }
    /* copy length related parameters back to caller instr */
    saved_curip->xtratim = curip->xtratim;
    saved_curip->relesing = curip->relesing;
    saved_curip->offbet = curip->offbet;
    saved_curip->offtim = curip->offtim;
    saved_curip->p3 = curip->p3;

    /* restore globals */
    ids = saved_ids;
    curip = saved_curip;
    return OK;
}

/* IV - Sep 8 2002: new functions for user defined opcodes (based */
/* on Matt J. Ingalls' subinstruments, but mostly rewritten) */

int useropcd1(UOPCODE*), useropcd2(UOPCODE*);  /* IV - Sep 17 2002 */

int useropcdset(UOPCODE *p)
{
    OPDS    *saved_ids = ids;
    INSDS   *saved_curip = curip, *parent_ip = curip, *lcurip;
    int     instno, i, n, pcnt;
    OPCODINFO *inm;
    OPCOD_IOBUFS  *buf;
    int     g_ksmps;
    MYFLT   g_ensmps, g_ekr, g_onedkr, g_hfkprd, g_kicvt;

    g_ksmps = p->l_ksmps = ksmps;       /* default ksmps */
    p->ksmps_scale = 1;
    /* look up the 'fake' instr number, and opcode name */
    inm = (OPCODINFO*) opcodlst[p->h.optext->t.opnum].useropinfo;
    instno = inm->instno;
    /* set local ksmps if defined by user */
    n = p->OUTOCOUNT + p->INOCOUNT - 1;
    if (*(p->ar[n]) != FL(0.0)) {
      i = (int) *(p->ar[n]);
      if (i < 1 || i > ksmps || ((ksmps / i) * i) != ksmps) {
        sprintf(errmsg, Str(X_1729, "%s: invalid local ksmps value: %d"),
                inm->name, i);
        return initerror(errmsg);
      }
      p->l_ksmps = i;
    }
    /* save old globals */
    g_ensmps = ensmps;
    g_ekr = ekr; g_onedkr = onedkr; g_hfkprd = hfkprd; g_kicvt = kicvt;
    /* set up local variables depending on ksmps, also change globals */
    if (p->l_ksmps != g_ksmps) {
      ksmps = p->l_ksmps;
      p->ksmps_scale = g_ksmps / (int) ksmps;
      p->l_ensmps = ensmps = pool[O.poolcount + 2] = (MYFLT) p->l_ksmps;
      p->l_ekr = ekr = pool[O.poolcount + 1] = esr / p->l_ensmps;
      p->l_onedkr = onedkr = FL(1.0) / p->l_ekr;
      p->l_hfkprd = hfkprd = FL(0.5) / p->l_ekr;
      p->l_kicvt = kicvt = (MYFLT) FMAXLEN / p->l_ekr;
      kcounter *= p->ksmps_scale;
    }

    if (!(reinitflag | tieflag)) {
      /* search for already allocated, but not active instance */
      /* if none was found, allocate a new instance */
      if (!instrtxtp[instno]->act_instance)             /* IV - Oct 26 2002 */
        instance(instno);
      lcurip = instrtxtp[instno]->act_instance;     /* use free intance, and */
      instrtxtp[instno]->act_instance = lcurip->nxtact; /* remove from chain */
      lcurip->insno = instno;
      lcurip->actflg++;                 /*    and mark the instr active */
      instrtxtp[instno]->active++;
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
      /* store parameters of input and output channels, and parent ip */
      buf->uopcode_struct = (void*) p;
      buf->parent_ip = p->parent_ip = parent_ip;
    }

    /* copy parameters from the caller instrument into our subinstrument */
    lcurip = p->ip;
    lcurip->m_chnbp = parent_ip->m_chnbp;       /* MIDI parameters */
    lcurip->m_pitch = parent_ip->m_pitch;
    lcurip->m_veloc = parent_ip->m_veloc;
    n = (int) parent_ip->xtratim * p->ksmps_scale;
    lcurip->xtratim = (short) (n > 32767 ? 32767 : n);
    lcurip->relesing = parent_ip->relesing;
    lcurip->offbet = parent_ip->offbet;
    lcurip->offtim = parent_ip->offtim;
    /* copy all p-fields, including p1 (will this work ?) */
    if (instrtxtp[instno]->pmax > 3) {      /* requested number of p-fields */
      n = instrtxtp[instno]->pmax; pcnt = 0;
      while (pcnt < n) {
        if ((i = instrtxtp[parent_ip->insno]->pmax) > pcnt) {
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
    else memcpy(&(lcurip->p1), &(parent_ip->p1), 3 * sizeof(MYFLT));

    /* do init pass for this instr */
    curip = lcurip;
    ids = (OPDS *) (lcurip->nxti);
    while (ids != NULL) {
      (*ids->iopadr)(ids);
      ids = ids->nxti;
    }
    /* copy length related parameters back to caller instr */
    if (ksmps == g_ksmps)
      saved_curip->xtratim = lcurip->xtratim;
    else
      saved_curip->xtratim =
        (lcurip->xtratim < 0 ? 32767 : lcurip->xtratim) / p->ksmps_scale;
    saved_curip->relesing = lcurip->relesing;
    saved_curip->offbet = lcurip->offbet;
    saved_curip->offtim = lcurip->offtim;
    saved_curip->p3 = lcurip->p3;

    /* restore globals */
    ids = saved_ids;
    curip = saved_curip;
    if (ksmps != g_ksmps) {
      ksmps = g_ksmps; ensmps = pool[O.poolcount + 2] = g_ensmps;
      ekr = pool[O.poolcount + 1] = g_ekr;
      onedkr = g_onedkr; hfkprd = g_hfkprd; kicvt = g_kicvt;
      kcounter = kcounter / p->ksmps_scale;
      /* IV - Sep 17 2002: also select perf routine */
      p->h.opadr = (SUBR) useropcd1;
    }
    else
      p->h.opadr = (SUBR) useropcd2;
    return OK;
}

/* IV - Sep 17 2002: dummy user opcode function for not initialised case */

int useropcd(UOPCODE *p)
{
    sprintf(errmsg, Str(X_1730, "%s: not initialised"), p->h.optext->t.opcod);
    return perferror(errmsg);
}

/* IV - Sep 1 2002: new opcodes: xin, xout */

int xinset(XIN *p)
{
    OPCOD_IOBUFS  *buf;
    OPCODINFO   *inm;
    short       *ndx_list;
    MYFLT       **tmp, **bufs;

    buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
    inm = buf->opcode_info;
    bufs = ((UOPCODE*) buf->uopcode_struct)->ar + inm->outchns;
    /* copy i-time variables */
    ndx_list = inm->in_ndx_list - 1;
    while (*++ndx_list >= 0)
      *(*(p->args + *ndx_list)) = *(*(bufs + *ndx_list));

    if (reinitflag | tieflag) return OK;
    /* find a-rate variables and add to list of perf-time buf ptrs ... */
    tmp = buf->iobufp_ptrs;
    if (*tmp || *(tmp + 1)) {
      return initerror(Str(X_1734,"xin was already used in this opcode definition"));
    }
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
    /* fix for case when xout is omitted */
    *(tmp++) = NULL; *tmp = NULL;
    return OK;
}

int xoutset(XOUT *p)
{
    OPCOD_IOBUFS  *buf;
    OPCODINFO   *inm;
    short       *ndx_list;
    MYFLT       **tmp, **bufs;

    buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
    inm = buf->opcode_info;
    bufs = ((UOPCODE*) buf->uopcode_struct)->ar;
    /* copy i-time variables */
    ndx_list = inm->out_ndx_list - 1;
    while (*++ndx_list >= 0)
      *(*(bufs + *ndx_list)) = *(*(p->args + *ndx_list));

    if (reinitflag | tieflag) return OK;
    /* skip input pointers, including the two delimiter NULLs */
    tmp = buf->iobufp_ptrs;
    if (*tmp || *(tmp + 1)) tmp += (inm->perf_incnt << 1);
    tmp += 2;
    if (*tmp || *(tmp + 1)) {
      return initerror(Str(X_1738,"xout was already used in this opcode definition"));
    }
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
    *tmp = NULL;                /* put delimiter */
    return OK;
}

/* IV - Sep 8 2002: new opcode: setksmps */

int setksmpsset(SETKSMPS *p)
{
    OPCOD_IOBUFS    *buf;
    UOPCODE *pp;
    int     l_ksmps, n;

    buf = (OPCOD_IOBUFS*) p->h.insdshead->opcod_iobufs;
    l_ksmps = (int) *(p->i_ksmps);
    if (!l_ksmps) return OK;       /* zero: do not change */
    if (l_ksmps < 1 || l_ksmps > ksmps
        || ((ksmps / l_ksmps) * l_ksmps != ksmps)) {
      sprintf(errmsg, Str(X_1750,"setksmps: invalid ksmps value: %d"), l_ksmps);
      return initerror(errmsg);
    }
    /* set up global variables according to the new ksmps value */
    pp = (UOPCODE*) buf->uopcode_struct;
    n = ksmps / l_ksmps;
    pp->ksmps_scale *= n;
    n *= p->h.insdshead->xtratim;
    p->h.insdshead->xtratim = (n > 32767 ? 32767 : n);
    pp->l_ksmps = ksmps = l_ksmps;
    pp->l_ensmps = ensmps = pool[O.poolcount + 2] = (MYFLT) ksmps;
    pp->l_ekr = ekr = pool[O.poolcount + 1] = esr / ensmps;
    pp->l_onedkr = onedkr = FL(1.0) / ekr;
    pp->l_hfkprd = hfkprd = FL(0.5) / ekr;
    pp->l_kicvt = kicvt = (MYFLT) FMAXLEN / ekr;
    kcounter *= pp->ksmps_scale;
    return OK;
}

/* IV - Oct 16 2002: nstrnum opcode (returns the instrument number of a */
/* named instrument) */

int nstrnumset(NSTRNUM *p)
{
    /* IV - Oct 31 2002 */
    *(p->i_insno) = (MYFLT) strarg2insno(p->iname, p->STRARG);
    return OK;
}

/* IV - Nov 16 2002: moved insert_event() here to have access to some static */
/* variables defined in this file */

INSDS *insert_event(MYFLT instr,
                    MYFLT when,
                    MYFLT dur,
                    int narg,
                    MYFLT **args,
                    int midi)
{
    int insno = (int)instr, saved_inerrcnt = inerrcnt;
    int saved_reinitflag = reinitflag, saved_tieflag = tieflag;
    INSDS *saved_curip = curip, *ip = NULL;
    INSDS *prvp, *nxtp;                                 /* IV - Nov 16 2002 */
    OPDS  *saved_ids = ids;
    INSTRTXT  *tp;

    printf("insert_event: %.0f %f %f ...\n", instr, when, dur);

    inerrcnt = tieflag = reinitflag = 0;        /* IV - Nov 16 2002 */
    if (instrtxtp[insno] == NULL) {
      printf(Str(X_1177,"schedule event ignored. instr %d undefined\n"), insno);
      perferrcnt++;
      goto endsched;            /* IV - Nov 16 2002 */
    }
    /* Insert this event into event queue */
    if (O.odebug) printf("activating instr %d\n",insno);
    tp = instrtxtp[insno];
    if (tp->mdepends & 04) {
      printf(Str(X_925,
                 "instr %d expects midi event data, cannot run from score\n"),
             insno);
      perferrcnt++;
      goto endsched;
    }
    if ((ip = tp->instance) != NULL) { /* if allocs of text exist: */
      do {
        if (ip->insno == insno        /*       if find this insno,  */
            && ip->actflg             /*        active              */
            && ip->offtim < 0         /*        with indef (tie)    */
            && ip->p1 == instr) {     /*  & matching p1             */
          tieflag++;
          goto init;                  /*        continue that event */
        }
      } while ((ip = ip->nxtinstance) != NULL);
      ip = tp->instance;              /*       else get alloc of text */
      do {
        if (!ip->actflg)              /*              that is free    */
          goto actlnk;                /*            and use its space */
      } while ((ip = ip->nxtinstance) != NULL);
    }

    if (O.msglevel & 2) printf(Str(X_1013,"new alloc for instr %d:\n"),insno);
    ip = instance(insno);     /* else alloc new dspace  */

 actlnk:
    cpu_power_busy += tp->cpuload;
    if (cpu_power_busy > 100.0) { /* if there is no more cpu processing time*/
      cpu_power_busy -= tp->cpuload;
      if (O.msglevel & WARNMSG)
        printf(Str(X_26,
                   "WARNING: cannot allocate last note because it exceeds"
                   " 100% of cpu time"));
      ip = NULL; goto endsched;
    }
    /* Add an active instrument */
    if (tp->active++ > tp->maxalloc && tp->maxalloc > 0) {
      tp->active--;
      if (O.msglevel & WARNMSG)
        printf(Str(X_27,
                   "WARNING: cannot allocate last note because it exceeds"
                   " instr maxalloc"));
      ip = NULL; goto endsched;
    }
    ip->insno = insno;
    nxtp = &actanchor;        /* now splice into active list */
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
      int   n = -3, m = 0;
      MYFLT  *flp;

      if ((int) tp->pmax != narg &&
          (O.msglevel & WARNMSG)) {
        sprintf(errmsg,Str(X_928,"instr %d pmax = %d, note pcnt = %d"),
                insno, (int) tp->pmax, narg);
        printf(Str(X_526,"WARNING: %s\n"), errmsg);
      }
      ip->offbet = (dur >= FL(0.0) ? when + dur : FL(-1.0));
      ip->p1 = instr;
      ip->p2 = when;
      ip->p3 = dur;
      flp = &(ip->p1) + 3;
      if (O.odebug) printf(Str(X_1137,"psave beg at %p\n"),flp);
      if (narg >= instrtxtp[insno]->pmax) {
        /* copy p-fields */
        n += instrtxtp[insno]->pmax;
        while (n--) *flp++ = *(args[m++]);
      }
      else {
        n += narg;
        while (n-- > 0) *flp++ = *(args[m++]);
        /* insufficient p-fields given, pad with zeroes */
        while (m++ < instrtxtp[insno]->pmax) *flp++ = FL(0.0);
      }
      if (O.odebug) printf(Str(X_6,"   ending at %p\n"),flp);
    }
    ip->offtim = ip->p3;      /* & duplicate p3 for now */
    ip->xtratim = 0;
    ip->relesing = 0;
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
    curip = ip;
    ids = (OPDS *)ip;
    while ((ids = ids->nxti) != NULL) {  /* do init pass for this instr */
      /*    if (O.odebug) printf("init %s:\n",
            opcodlst[ids->optext->t.opnum].opname);      */
      (*ids->iopadr)(ids);
    }
    if (inerrcnt || !ip->p3) {
      deact(ip);
      ip = NULL; goto endsched;
    }
    if (!midi  &&               /* if not MIDI activated, */
        ip->p3 > FL(0.0)) {     /* and still finite time, */
      ip->offtim = ip->p2 + ip->p3;
      schedofftim(ip);          /*       put in turnoff list */
    }
    else ip->offtim = -FL(1.0); /* else mark indef */
    if (O.odebug) {
      printf("instr %d now active:\n",insno); showallocs();
    }
 endsched:
    /* IV - Nov 16 2002: restore globals */
    inerrcnt = saved_inerrcnt;
    reinitflag = saved_reinitflag;
    tieflag = saved_tieflag;
    curip = saved_curip;
    ids = saved_ids;
    return ip;
}

void beatexpire(MYFLT beat)     /* unlink expired notes from activ chain */
{                               /*      and mark them inactive          */
    INSDS  *ip;                 /*    close any files in each fdchain   */
 strt:
    if ((ip = frstoff) != NULL && ip->offbet <= beat) {
      do {
        if (!ip->relesing && ip->xtratim) {
          /* IV - Nov 30 2002: allow extra time for finite length (p3 > 0) */
          /* score notes */
          ip->offtim = global_onedkr
            * (MYFLT) ((long) global_kcounter
                       + (long) ((unsigned short) ip->xtratim));
          ip->relesing++;               /* enter release stage */
          /* possibly wrong */
          ip->offbet += global_onedkr * (MYFLT) ((unsigned short) ip->xtratim);
          frstoff = ip->nxtoff;         /* update turnoff list */
          schedofftim(ip);
          goto strt;                    /* and start again */
        }
        else
          deact(ip);    /* IV - Sep 5 2002: use deact() as it also */
      }                 /* deactivates subinstrument instances */
      while ((ip = ip->nxtoff) != NULL && ip->offbet <= beat);
      frstoff = ip;
      if (O.odebug) {
        printf("deactivated all notes to beat %7.3f\n",beat);
        printf("frstoff = %p\n",frstoff);
      }
    }
}

void timexpire(MYFLT time)      /* unlink expired notes from activ chain */
{                               /*      and mark them inactive           */
    INSDS  *ip;                 /*    close any files in each fdchain    */
 strt:
    if ((ip = frstoff) != NULL && ip->offtim <= time) {
      do {
        if (!ip->relesing && ip->xtratim) {
          /* IV - Nov 30 2002: allow extra time for finite length (p3 > 0) */
          /* score notes */
          ip->offtim = global_onedkr
            * (MYFLT) ((long) global_kcounter
                       + (long) ((unsigned short) ip->xtratim));
          ip->relesing++;               /* enter release stage */
          frstoff = ip->nxtoff;         /* update turnoff list */
          schedofftim(ip);
          goto strt;                    /* and start again */
        } else
          deact(ip);    /* IV - Sep 5 2002: use deact() as it also */
      }                 /* deactivates subinstrument instances */
      while ((ip = ip->nxtoff) != NULL && ip->offtim <= time);
      frstoff = ip;
      if (O.odebug) {
        printf("deactivated all notes to time %7.3f\n",time);
        printf("frstoff = %p\n",frstoff);
      }
    }
}

int subinstr(SUBINST *p)
{
    OPDS    *saved_pds = pds;
    int     saved_sa = spoutactive;
    MYFLT   *pbuf, *saved_spout = spout;
    long    frame, chan;

    if (p->ip == NULL) {                /* IV - Oct 26 2002 */
      return perferror(Str(X_1804,"subinstr: not initialised"));
    }
    /* copy current spout buffer and clear it */
    spout = (MYFLT*) p->saved_spout.auxp;
    spoutactive = 0;
    /* update release flag */
    p->ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */

    /*  run each opcode  */
    pds = (OPDS *)p->ip;
    while ((pds = pds->nxtp) != NULL) {
      (*pds->opadr)(pds);
    }

    /* copy outputs */
    for (chan = 0; chan < p->OUTOCOUNT; chan++) {
      for (pbuf = spout + chan, frame = 0; frame < ksmps; frame++) {
        p->ar[chan][frame] = *pbuf;
        pbuf += nchnls;
      }
    }

    /* restore spouts */
    spout = saved_spout;
    spoutactive = saved_sa;
    pds = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip)
      while (pds->nxtp) pds = pds->nxtp;        /* loop to last opds */
    return OK;
}

/* IV - Sep 17 2002 -- case 1: local ksmps is used */

int useropcd1(UOPCODE *p)
{
    OPDS    *saved_pds = pds;
    int     g_ksmps, ofs = 0, n;
    MYFLT   g_ensmps, g_ekr, g_onedkr, g_hfkprd, g_kicvt, **tmp, *ptr1, *ptr2;
    long    g_kcounter;

    /* update release flag */
    p->ip->relesing = p->parent_ip->relesing;   /* IV - Nov 16 2002 */
    /* save old globals */
    g_ksmps = ksmps; g_ensmps = ensmps;
    g_ekr = ekr; g_onedkr = onedkr; g_hfkprd = hfkprd; g_kicvt = kicvt;
    g_kcounter = kcounter;
    /* set local ksmps and related values */
    ksmps = p->l_ksmps; ensmps = pool[O.poolcount + 2] = p->l_ensmps;
    ekr = pool[O.poolcount + 1] = p->l_ekr;
    onedkr = p->l_onedkr; hfkprd = p->l_hfkprd; kicvt = p->l_kicvt;
    kcounter = kcounter * p->ksmps_scale;

    if (ksmps == 1) {                   /* special case for local kr == sr */
      do {
        /* copy inputs */
        tmp = p->buf->iobufp_ptrs;
        while (*tmp) {                  /* a-rate */
          ptr1 = *(tmp++) + ofs; *(*(tmp++)) = *ptr1;
        }
        while (*(++tmp)) {              /* k-rate */
          ptr1 = *tmp; *(*(++tmp)) = *ptr1;
        }
        /*  run each opcode  */
        pds = (OPDS *) (p->ip);
        while ((pds = pds->nxtp)) {
          (*pds->opadr)(pds);
        }
        /* copy outputs */
        while (*(++tmp)) {              /* a-rate */
          ptr1 = *tmp; (*(++tmp))[ofs] = *ptr1;
        }
        ++kcounter;
      } while (++ofs < g_ksmps);
    }
    else {                              /* generic case for local kr != sr */
      do {
        /* copy inputs */
        tmp = p->buf->iobufp_ptrs;
        while (*tmp) {                  /* a-rate */
          ptr1 = *(tmp++) + ofs; ptr2 = *(tmp++);
          n = ksmps;
          do {
            *(ptr2++) = *(ptr1++);
          } while (--n);
        }
        while (*(++tmp)) {              /* k-rate */
          ptr1 = *tmp; *(*(++tmp)) = *ptr1;
        }
        /*  run each opcode  */
        pds = (OPDS *) (p->ip);
        while ((pds = pds->nxtp)) {
          (*pds->opadr)(pds);
        }
        /* copy outputs */
        while (*(++tmp)) {              /* a-rate */
          ptr1 = *tmp; ptr2 = *(++tmp) + ofs;
          n = ksmps;
          do {
            *(ptr2++) = *(ptr1++);
          } while (--n);
        }
        ++kcounter;
      } while ((ofs += ksmps) < g_ksmps);
    }
    /* k-rate outputs are copied only in the last sub-kperiod, */
    /* so we do it now */
    while (*(++tmp)) {                  /* k-rate */
      ptr1 = *tmp; *(*(++tmp)) = *ptr1;
    }

    /* restore globals */
    ksmps = g_ksmps; ensmps = pool[O.poolcount + 2] = g_ensmps;
    ekr = pool[O.poolcount + 1] = g_ekr;
    onedkr = g_onedkr; hfkprd = g_hfkprd; kicvt = g_kicvt;
    kcounter = g_kcounter;
    pds = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip)
      while (pds->nxtp) pds = pds->nxtp;    /* loop to last opds */
    return OK;
}

/* IV - Sep 17 2002 -- case 2: simplified routine for no local ksmps */

int useropcd2(UOPCODE *p)
{
    OPDS    *saved_pds = pds;
    int     n;
    MYFLT   **tmp, *ptr1, *ptr2;

    if (!(pds = (OPDS*) (p->ip->nxtp))) goto endop;     /* no perf code */
    /* IV - Nov 16 2002: update release flag */
    p->ip->relesing = p->parent_ip->relesing;

    tmp = p->buf->iobufp_ptrs;
    if (ksmps != 1) {                   /* generic case for kr != sr */
      /* copy inputs */
      while (*tmp) {                    /* a-rate */
        ptr1 = *(tmp++); ptr2 = *(tmp++);
        n = ksmps;
        do {
          *(ptr2++) = *(ptr1++);
        } while (--n);
      }
      while (*(++tmp)) {                /* k-rate */
        ptr1 = *tmp; *(*(++tmp)) = *ptr1;
      }
      /*  run each opcode  */
      do {
        (*pds->opadr)(pds);
      } while ((pds = pds->nxtp));
      /* copy outputs */
      while (*(++tmp)) {                /* a-rate */
        ptr1 = *tmp; ptr2 = *(++tmp);
        n = ksmps;
        do {
          *(ptr2++) = *(ptr1++);
        } while (--n);
      }
    }
    else {                      /* special case for kr == sr */
      /* copy inputs */
      while (*tmp) {                    /* a-rate */
        ptr1 = *(tmp++); *(*(tmp++)) = *ptr1;
      }
      while (*(++tmp)) {                /* k-rate */
        ptr1 = *tmp; *(*(++tmp)) = *ptr1;
      }
      /*  run each opcode  */
      do {
        (*pds->opadr)(pds);
      } while ((pds = pds->nxtp));
      /* copy outputs */
      while (*(++tmp)) {                /* a-rate */
        ptr1 = *tmp; *(*(++tmp)) = *ptr1;
      }
    }
    while (*(++tmp)) {                  /* k-rate */
      ptr1 = *tmp; *(*(++tmp)) = *ptr1;
    }
 endop:
    /* restore globals */
    pds = saved_pds;
    /* check if instrument was deactivated (e.g. by perferror) */
    if (!p->ip)
      while (pds->nxtp) pds = pds->nxtp;    /* loop to last opds */
    return OK;
}
