/*
    musmon.c:

    Copyright (C) 1991,2002 Barry Vercoe, John ffitch,
                            Istvan Varga, rasmus ekman

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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "cs.h"                 /*                         MUSMON.C     */
#include "cscore.h"
#include "midiops.h"
#include "soundio.h"
#include "namedins.h"
#include "oload.h"
#include <math.h>

int     cleanup(void*);
extern  void    kperf(ENVIRON*);

#define SEGAMPS 01
#define SORMSG  02

long    srngcnt[MAXCHNLS], orngcnt[MAXCHNLS];
short   srngflg = 0;

extern  OPARMS  O;
static  short   sectno = 0;

extern  int     MIDIinsert(ENVIRON *, int, MCHNBLK*, MEVENT*);
extern  int     insert(ENVIRON *, int, EVTBLK*);

extern  void    print_benchmark_info(void*, const char*);     /* main.c */

extern  void    RTLineset(void), MidiOpen(void *);
extern  void    m_chn_init_all(ENVIRON *);
extern  void    scsort(FILE*, FILE*), oload(ENVIRON *), cscorinit(void);
extern  void    infoff(MYFLT), orcompact(ENVIRON*);
extern  void    beatexpire(ENVIRON *, double), timexpire(ENVIRON *, double);
extern  void    fgens(ENVIRON *,EVTBLK *);
extern  void    sfopenin(void*), sfopenout(void*), sfnopenout(void);
extern  void    iotranset(void), sfclosein(void*), sfcloseout(void*);
extern  void    MidiClose(ENVIRON*);

extern  OPARMS  O;

static  int     playevents(ENVIRON *);
static  int     lplayed = 0;
static  int     segamps, sormsg;
static  EVENT   **ep, **epend;  /* pointers for stepping through lplay list */
static  EVENT   *lsect = NULL;
        void    beep(void);

static void settempo(ENVIRON *csound, MYFLT tempo)
{
    if (tempo > FL(0.0)) {
      csound->sensEvents_state.beatTime = 60.0 / (double) tempo;
      csound->sensEvents_state.curBeat_inc =
                            (double) tempo / (60.0 * (double) csound->ekr_);
    }
}

int gettempo(ENVIRON *csound, GTEMPO *p)
{
    if (O.Beatmode)
      *p->ans = (MYFLT) (60.0 / csound->sensEvents_state.beatTime);
    else
      *p->ans = FL(60.0);
    return OK;
}

int tempset(ENVIRON *csound, TEMPO *p)
{
    MYFLT tempo;

    if ((tempo = *p->istartempo) <= FL(0.0)) {
      return initerror(Str("illegal istartempo value"));
    }
    else {
      settempo(csound, tempo);
      p->prvtempo = tempo;
      return OK;
    }
}

int tempo(ENVIRON *csound, TEMPO *p)
{
    if (*p->ktempo != p->prvtempo) {
      settempo(csound, *p->ktempo);
      p->prvtempo = *p->ktempo;
    }
    return OK;
}

void musRESET(void)
{
    /* belt, braces, and parachute */
    int i;
    for (i=0;i<MAXCHNLS;i++) {
      maxamp[i]  = 0.0f;
      smaxamp[i] = 0.0f;
      omaxamp[i] = 0.0f;
      rngcnt[i]  = 0;
      srngcnt[i] = 0;
      orngcnt[i] = 0;
      omaxpos[i] = smaxpos[i] = maxpos[i] = 1;
    }
    maxampend  = NULL;
    rngflg     = 0;
    srngflg    = 0;
    multichan  = 0;
    sectno     = 0;

    lplayed    = 0;
    segamps    = 0;
    sormsg     = 0;
    ep         = NULL;
    epend      = NULL;
    lsect      = NULL;
}

void print_maxamp(MYFLT x)                     /* IV - Jul 9 2002 */
{
    MYFLT   y;

    if (!(O.msglevel & 0x60)) {                 /* 0x00: raw amplitudes */
      if (e0dbfs > FL(3000.0)) printf("%9.1f", x);
      else if (e0dbfs < FL(3.0)) printf("%9.5f", x);
      else if (e0dbfs > FL(300.0)) printf("%9.2f", x);
      else if (e0dbfs > FL(30.0)) printf("%9.3f", x);
      else printf("%9.4f", x);
    }
    else {                      /* dB values */
      y = x / e0dbfs;           /* relative level */
      if (y < FL(1.0e-10)) {
        printf ("      0  ");   /* less than -200 dB: print zero */
        return;
      }
      y = FL(20.0) * (MYFLT) log10 ((double) y);
      switch (O.msglevel & 0x60) {
      case 0x60:                                /* 0x60: dB with all colors */
        if (y >= FL(0.0)) printf ("\033[31m");          /* red */
        else if (y >= FL(-6.0)) printf ("\033[33m");    /* yellow */
        else if (y >= FL(-24.0)) printf ("\033[32m");   /* green */
        else printf ("\033[34m");                       /* blue */
        printf ("\033[1m%+9.2f\033[m", y);
        break;
      case 0x40:                                /* 0x40: dB with overrange */
        if (y >= FL(0.0)) printf ("\033[31m\033[1m");   /* red */
        printf ("%+9.2f", y);
        if (y >= FL(0.0)) printf ("\033[m");
        break;
      default:                                  /* 0x20: dB with no colors */
        printf ("%+9.2f", y);
      }
    }
}

int musmon2(ENVIRON *csound);

int musmon(ENVIRON *csound)
{
    if (sizeof(MYFLT)==sizeof(float)) {
#ifdef BETA
      err_printf("Csound version %s beta (float samples) %s\n",
                 PACKAGE_VERSION, __DATE__);
#else
      err_printf("Csound version %s (float samples) %s\n",
                 PACKAGE_VERSION, __DATE__);
#endif
    }
    else {
#ifdef BETA
      err_printf("Csound version %s beta (double samples) %s\n",
                 PACKAGE_VERSION, __DATE__);
#else
      err_printf("Csound version %s (double samples) %s\n",
                 PACKAGE_VERSION, __DATE__);
#endif
    }

    m_chn_init_all(csound);     /* allocate MIDI channels */
    dispinit();                 /* initialise graphics or character display */
    oload(csound);              /* set globals and run inits */

    /* kperf() will not call csoundYield() more than 1000 times per second */
    csound->evt_poll_cnt = 0;
    csound->evt_poll_maxcnt = (int) ((double) csound->ekr_ / 1000.0);
    /* initialise sensevents state */
    {
      sensEvents_t  *p;
      p = &(csound->sensEvents_state);
      memset(p, 0, sizeof(sensEvents_t));
      p->prvbt = p->curbt = p->nxtbt = 0.0;
      p->curp2 = p->nxtim = p->timeOffs = p->beatOffs = 0.0;
      p->curTime = p->curBeat = 0.0;
      p->curTime_inc = p->curBeat_inc = 1.0 / (double) csound->ekr_;
    }
    if (O.Beatmode)                             /* if performing from beats */
      settempo(csound, (MYFLT) O.cmdTempo);     /*   set the initial tempo  */
    else
      settempo(csound, FL(60.0));
    /* Enable musmon to handle external MIDI input, if it has been enabled. */
    if (O.Midiin || O.FMidiin) {
      O.RTevents = 1;
      O.ksensing = 1;
      MidiOpen(csound);         /*   alloc bufs & open files    */
    }
    printf(Str("orch now loaded\n"));

    multichan = (nchnls > 1) ? 1:0;
    maxampend = &maxamp[nchnls];
    segamps = O.msglevel & SEGAMPS;
    sormsg = O.msglevel & SORMSG;

    if (O.Linein)  RTLineset();     /* if realtime input expected   */
    if (O.outbufsamps < 0) {        /* if k-aligned iobufs requestd */
      O.inbufsamps = O.outbufsamps *= -ksmps; /*  set from absolute value */
      printf(Str("k-period aligned audio buffering\n"));
    }
    /* else keep the user values    */
    if (!O.oMaxLag)
      O.oMaxLag = IODACSAMPS;
    if (!O.inbufsamps)
      O.inbufsamps = IOBUFSAMPS;
    if (!O.outbufsamps)
      O.outbufsamps = IOBUFSAMPS;
    /* IV - Feb 04 2005: make sure that buffer sizes for real time audio */
    /* are usable */
    if (O.infilename != NULL &&
        (strncmp(O.infilename, "adc", 3) == 0 ||
         strncmp(O.infilename, "devaudio", 8) == 0)) {
      O.oMaxLag = ((O.oMaxLag+O.inbufsamps-1) / O.inbufsamps) * O.inbufsamps;
      if (O.oMaxLag <= O.inbufsamps) O.inbufsamps >>= 1;
      O.outbufsamps = O.inbufsamps;
    }
    if (O.outfilename != NULL &&
        (strncmp(O.outfilename, "dac", 3) == 0 ||
         strncmp(O.outfilename, "devaudio", 8) == 0)) {
      O.oMaxLag = ((O.oMaxLag+O.outbufsamps-1) / O.outbufsamps) * O.outbufsamps;
      if (O.oMaxLag <= O.outbufsamps) O.outbufsamps >>= 1;
      O.inbufsamps = O.outbufsamps;
    }
    printf(Str("audio buffered in %d sample-frame blocks\n"), O.outbufsamps);
    O.inbufsamps *= nchnls;         /* now adjusted for n channels  */
    O.outbufsamps *= nchnls;
    if (O.sfread)                   /* if audio-in requested,       */
      sfopenin(csound);             /*   open/read? infile or device */
    if (O.sfwrite)                  /* if audio-out requested,      */
      sfopenout(csound);            /*   open the outfile or device */
    else sfnopenout();
    iotranset();                    /* point recv & tran to audio formatter */

    /*        if (!O.Linein) { */   /*  *************** */
    if (!(scfp = fopen(O.playscore, "r"))) {
      if (!O.Linein) {
        dies(Str("cannot reopen %s"), O.playscore);
      }
    }
    if (O.usingcscore) {
      if (lsect == NULL) {
        lsect = (EVENT *) mmalloc(csound, (long)sizeof(EVENT));
        lsect->op = 'l';
      }
      printf(Str("using Cscore processing\n"));
      if (!(oscfp = fopen("cscore.out", "w")))  /* override stdout in   */
        die(Str("cannot create cscore.out"));/* rdscor for cscorefns */
      cscorinit();
      cscore(csound);      /* call cscore, optionally re-enter via lplay() */
      fclose(oscfp);
      fclose(scfp); scfp = NULL;
      if (lplayed) return 0;

      if (!(scfp = fopen("cscore.out", "r")))   /*   rd from cscore.out */
        die(Str("cannot reopen cscore.out"));
      if (!(oscfp = fopen("cscore.srt", "w")))  /*   writ to cscore.srt */
        die(Str("cannot reopen cscore.srt"));
      printf(Str("sorting cscore.out ..\n"));
      scsort(scfp, oscfp);                      /* call the sorter again */
      fclose(scfp); scfp = NULL;
      fclose(oscfp);
      printf(Str("\t... done\n"));
      if (!(scfp = fopen("cscore.srt", "r")))   /*   rd from cscore.srt */
        die(Str("cannot reopen cscore.srt"));
      printf(Str("playing from cscore.srt\n"));
      O.usingcscore = 0;
    }
    printf(Str("SECTION %d:\n"), ++sectno);
                                     /* since we are running in components */
    return 0;                        /* we exit here to playevents later   */
}

int musmon2(ENVIRON *csound)
{
    playevents(csound);              /* play all events in the score */
    return cleanup(csound);
}

int cleanup(void *csound)
{
    MYFLT   *maxp;
    long    *rngp;
    int     n;

    /* check if we have already cleaned up */
    if (csoundQueryGlobalVariable(csound, "#CLEANUP") == NULL)
      return 0;
    /* will not clean up more than once */
    csoundDestroyGlobalVariable(csound, "#CLEANUP");

    orcompact(csound);
    printf(Str("end of score.\t\t   overall amps:"));
    if (scfp) {
      fclose(scfp); scfp = NULL;
    }
    for (n = 0; n < nchnls; n++) {
      if (smaxamp[n] > omaxamp[n]) omaxamp[n] = smaxamp[n];
      if (maxamp[n] > omaxamp[n]) omaxamp[n] = maxamp[n];
      orngcnt[n] += (srngcnt[n] + rngcnt[n]);
    }
    for (maxp=omaxamp, n=nchnls; n--; )
      print_maxamp (*maxp++);                   /* IV - Jul 9 2002 */
    if (O.outformat != AE_FLOAT) {
      printf(Str("\n\t   overall samples out of range:"));
      for (rngp=orngcnt, n=nchnls; n--; )
        printf("%9ld", *rngp++);
    }
    printf(Str("\n%d errors in performance\n"),perferrcnt);
    print_benchmark_info(csound, Str("end of performance"));
    /* close MIDI input */
    MidiClose((ENVIRON*) csound);
    /* IV - Feb 03 2005: do not need to call rtclose from here, as */
    /* sfclosein/sfcloseout will do that. Checking O.sfread and */
    /* O.sfwrite is not needed either. */
    sfclosein(csound);
    sfcloseout(csound);
    if (! ((ENVIRON*) csound)->oparms_->sfwrite)
      printf(Str("no sound written to disk\n"));
    if (O.ringbell) beep();
    remove_tmpfiles(csound);
    return dispexit();      /* hold or terminate the display output     */
    /* for Mac, dispexit returns 0 to exit immediately */
}

void beep(void)
{
#ifdef mills_macintosh
    SysBeep(10000);
#else
    printf(Str("%c\tbeep!\n"),'\007');
#endif
}

int lplay(ENVIRON *csound, EVLIST *a) /* cscore re-entry into musmon */
{
    lplayed = 1;
    if (!sectno)  printf(Str("SECTION %d:\n"),++sectno);
    ep = &a->e[1];            /* from 1st evlist member */
    epend = ep + a->nevents;  /*   to last              */
    playevents(csound);       /* play list members      */
    return OK;
}

/* make list to turn on instrs for indef */
/* perf called from i0 for execution in playevents */

int turnon(ENVIRON *csound, TURNON *p)
{
    EVTBLK  evt;

    evt.strarg = NULL;
    evt.opcod = 'i';
    evt.pcnt = 3;
    evt.p[1] = *p->insno;
    evt.p[2] = *p->itime;
    evt.p[3] = FL(-1.0);

    return (insert_score_event(csound, &evt,
                               csound->sensEvents_state.curTime, 0) == 0 ?
            OK : NOTOK);
}

/* Print current amplitude values, and update section amps. */

static void print_amp_values(ENVIRON *csound, int score_evt)
{
    sensEvents_t  *p;
    MYFLT         *maxp, *smaxp;
    unsigned long *maxps, *smaxps;
    long          *rngp, *srngp;
    int           n;

    p = &(csound->sensEvents_state);
    if (segamps || (sormsg && rngflg)) {
      if (score_evt)
        printf("B%7.3f ..%7.3f T%7.3f TT%7.3f M:",
               p->prvbt - p->beatOffs,  p->curbt - p->beatOffs,
               p->curp2 - p->timeOffs,  p->curp2);
      else
        printf("  rtevent:\t   T%7.3f TT%7.3f M:",
               p->curp2 - p->timeOffs,  p->curp2);
      for (n = nchnls, maxp = maxamp; n--; )
        print_maxamp(*maxp++);          /* IV - Jul 9 2002 */
      printf("\n");
      if (rngflg) {
        printf(Str("\t number of samples out of range:"));
        for (n = nchnls, rngp = rngcnt; n--; )
          printf("%9ld", *rngp++);
        printf("\n");
      }
    }
    if (rngflg) {
      rngflg = 0;
      srngflg++;
    }
    for (n = nchnls, maxp = maxamp - 1, smaxp = smaxamp - 1,
         maxps = maxpos - 1, smaxps = smaxpos - 1,
         rngp = rngcnt, srngp = srngcnt; n--; ) {
      ++maxps; ++smaxps;
      if (*++maxp > *++smaxp) {
        *smaxp = *maxp;
        *smaxps = *maxps;
      }
      *maxp = FL(0.0);
      *maxps = 0;
      *srngp++ += *rngp;
      *rngp++ = 0;
    }
}

/* Update overall amplitudes from section values, */
/* and optionally print message (1: section end, 2: lplay end). */

static void section_amps(ENVIRON *csound, int enable_msgs)
{
    sensEvents_t  *p;
    MYFLT         *maxp, *smaxp;
    unsigned long *maxps, *smaxps;
    long          *rngp, *srngp;
    int           n;

    p = &(csound->sensEvents_state);
    if (enable_msgs) {
      if (enable_msgs == 1)
        printf(Str("end of section %d\t sect peak amps:"), sectno);
      else if (enable_msgs == 2)
        printf(Str("end of lplay event list\t      peak amps:"));
      for (n = nchnls, maxp = smaxamp; n--; )
        print_maxamp(*maxp++);          /* IV - Jul 9 2002 */
      printf("\n");
      if (srngflg) {
        printf(Str("\t number of samples out of range:"));
        for (n = nchnls, srngp = srngcnt; n--; )
          printf("%9ld", *srngp++);
        printf("\n");
      }
    }
    srngflg = 0;
    for (n = nchnls,
         smaxp = smaxamp - 1, maxp = omaxamp - 1,
         smaxps = smaxpos - 1, maxps = omaxpos - 1,
         srngp = srngcnt, rngp = orngcnt; n--; ) {
      ++maxps; ++smaxps;
      if (*++smaxp > *++maxp) {
        *maxp = *smaxp;                 /* keep ovrl maxamps */
        *maxps = *smaxps;               /* And where */
      }
      *smaxp = FL(0.0);
      *smaxps = 0;
      *rngp++ += *srngp;                /*   and orng counts */
      *srngp++ = 0;
    }
}

static void print_score_time(ENVIRON *csound, int rtEvt)
{
    sensEvents_t  *p;
    p = &(csound->sensEvents_state);
    if (rtEvt)
      printf("\t\t   T%7.3f", p->curp2 - p->timeOffs);
    else
      printf("\t  B%7.3f", p->curbt - p->beatOffs);
}

static int process_score_event(ENVIRON *csound, EVTBLK *evt, int rtEvt)
{
    sensEvents_t  *p;
    EVTBLK        *saved_currevent;
    int           insno, n;

    p = &(csound->sensEvents_state);
    saved_currevent = currevent;
    currevent = evt;
    switch (evt->opcod) {                       /* scorevt or Linevt:     */
    case 'e':           /* quit realtime */
    case 'l':
    case 's':
      while (frstoff != NULL) {
        xturnoff_now(csound, frstoff);
        frstoff = frstoff->nxtoff;
      }
      currevent = saved_currevent;
      return (evt->opcod == 'l' ? 3 : (evt->opcod == 's' ? 1 : 2));
    case 'q':
      if (evt->p[1] == SSTRCOD && evt->strarg) {    /* IV - Oct 31 2002 */
        if ((insno = (int) named_instr_find(evt->strarg)) < 1) {
          print_score_time(csound, rtEvt);
          printf(Str(" - note deleted. instr %s undefined\n"), evt->strarg);
          perferrcnt++;
          break;
        }
        printf(Str("Setting instrument %s %s\n"),
               evt->strarg, (evt->p[3] == 0 ? Str("off") : Str("on")));
        instrtxtp[insno]->muted = (short) evt->p[3];
      }
      else {                                        /* IV - Oct 31 2002 */
        insno = abs((int) evt->p[1]);
        if (insno > maxinsno || instrtxtp[insno] == NULL) {
          print_score_time(csound, rtEvt);
          printf(Str(" - note deleted. instr %d(%d) undefined\n"),
                 insno, maxinsno);
          perferrcnt++;
          break;
        }
        printf(Str("Setting instrument %d %s\n"),
               insno, (evt->p[3] == 0 ? Str("off") : (Str("on"))));
        instrtxtp[insno]->muted = (short) evt->p[3];
      }
      break;
    case 'i':
      if (evt->p[1] == SSTRCOD && evt->strarg) {    /* IV - Oct 31 2002 */
        if ((insno = (int) named_instr_find(evt->strarg)) < 1) {
          print_score_time(csound, rtEvt);
          printf(Str(" - note deleted. instr %s undefined\n"), evt->strarg);
          perferrcnt++;
          break;
        }
        evt->p[1] = (MYFLT) insno;
        if (O.Beatmode && evt->p3orig >= FL(0.0))
          evt->p[3] = evt->p3orig * p->beatTime;
        if ((n = insert(csound, insno, evt))) { /* else alloc, init, activate */
          print_score_time(csound, rtEvt);
          printf(Str(" - note deleted.  i%d (%s) had %d init errors\n"),
                 insno, evt->strarg, n);
          perferrcnt++;
        }
      }
      else {                                        /* IV - Oct 31 2002 */
        insno = abs((int) evt->p[1]);
        if (insno > maxinsno || instrtxtp[insno] == NULL) {
          print_score_time(csound, rtEvt);
          printf(Str(" - note deleted. instr %d(%d) undefined\n"),
                 insno, maxinsno);
          perferrcnt++;
        }
        else if (evt->p[1] < FL(0.0))           /* if p1 neg,             */
          infoff(-evt->p[1]);                   /*  turnoff any infin cpy */
        else {
          if (O.Beatmode && evt->p3orig >= FL(0.0))
            evt->p[3] = evt->p3orig * p->beatTime;
          if ((n = insert(csound, insno, evt))) { /* else alloc,init,activat */
            print_score_time(csound, rtEvt);
            printf(Str(" - note deleted.  i%d had %d init errors\n"), insno, n);
            perferrcnt++;
          }
        }
      }
      break;
    case 'f':
      fgens(csound, evt);
      break;
    case 'a':
      p->curp2 = p->timeOffs + (double) evt->p[2] + (double) evt->p[3];
      p->curbt = p->beatOffs + (double) evt->p2orig + (double) evt->p3orig;
      printf(Str("time advanced %5.3f beats by score request\n"), evt->p3orig);
      break;
    }
    currevent = saved_currevent;
    return 0;
}

static int process_rt_event(ENVIRON *csound, int sensType)
{
    sensEvents_t  *p;
    EVTBLK        *evt;
    int           retval, insno, n;

    p = &(csound->sensEvents_state);
    retval = 0;
    if (p->curp2 < p->curTime) {
      p->curp2 = p->curTime;
      print_amp_values(csound, 0);
    }
    if (sensType == 1) {                  /*    for Linein,       */
      evt = Linevtblk;                    /*      get its evtblk  */
      evt->p[2] = p->curp2 - p->timeOffs; /*      & insert curp2  */
      retval = process_score_event(csound, evt, 1);
    }
    else if (sensType == 4) {             /* Realtime orc event   */
      EVTNODE *e = csound->OrcTrigEvts;
      /* Events are sorted on insertion, so just check the first */
      evt = &(e->evt);
      /* pop from the list */
      csound->OrcTrigEvts = e->nxt;
      if (e->nxt == NULL)
        csound->oparms_->OrcEvts = 0;
      retval = process_score_event(csound, evt, 1);
      if (evt->strarg != NULL)
        mfree(csound, evt->strarg);
      /* push back to free alloc stack so it can be reused later */
      e->nxt = csound->freeEvtNodes;
      csound->freeEvtNodes = e;
    }
    else if (sensType == 2) {                           /* Midievent:    */
      MEVENT *mep;
      MCHNBLK *chn;
      /* realtime or Midifile  */
      mep = csound->midiGlobals->Midevtblk;
      chn = M_CHNBP[mep->chan];
      insno = chn->insno;
      if (mep->type == NOTEON_TYPE && mep->dat2) {      /* midi note ON: */
        if ((n = MIDIinsert(csound,insno,chn,mep))) {   /* alloc,init,activ */
          printf(Str("\t\t   T%7.3f - note deleted. "), p->curp2);
          printf(Str("instr %d had %d init errors\n"), insno, n);
          perferrcnt++;
        }
      }
      else {                            /* else midi note OFF:    */
        INSDS *ip = chn->kinsptr[mep->dat1];
        if (ip == NULL) {               /*  if already off, done  */
          Mxtroffs++;
          return retval;
        }
        if (chn->sustaining) {          /*  if sustain pedal on   */
          while (ip != NULL && ip->m_sust)
            ip = ip->nxtolap;
          if (ip != NULL) {
            ip->m_sust = 1;             /*    let the note ring   */
            chn->ksuscnt++;
          }
        }
        else                            /*  else some kind of off */
          xturnoff(csound, ip);
      }
    }
    return retval;
}

#define SENSEORCEVT(x) ((x)->OrcTrigEvts != NULL &&                     \
                        (x)->OrcTrigEvts->start_kcnt                    \
                          <= (unsigned long) (x)->global_kcounter_ ? 4 : 0)

#define RNDINT(x) ((int) ((double) (x) + ((double) (x) < 0.0 ? -0.5 : 0.5)))

extern  int     sensLine(void);
extern  int     sensMidi(ENVIRON *);

/* sense events for one k-period            */
/* return value is one of the following:    */
/*   0: continue performance                */
/*   1: terminate (e.g. end of MIDI file)   */
/*   2: normal end of score                 */

int sensevents(ENVIRON *csound)
{
    sensEvents_t  *p;
    EVTBLK        *e;
    int           retval, sensType;

    p = &(csound->sensEvents_state);
    if (MTrkend && O.termifend) {       /* end of MIDI file: */
      while (frstoff != NULL) {
        xturnoff_now(csound, frstoff);
        frstoff = frstoff->nxtoff;
      }
      printf(Str("terminating.\n"));
      return 1;                         /* abort with perf incomplete */
    }
    /* if turnoffs pending, remove any expired instrs */
    if (frstoff != NULL) {
      double  tval;
      if (O.Beatmode) {
        tval = p->curBeat + (0.51 * p->curBeat_inc);
        if (frstoff->offbet <= tval) beatexpire(csound, tval);
      }
      else {
        tval = p->curTime + (0.51 * p->curTime_inc);
        if (frstoff->offtim <= tval) timexpire(csound, tval);
      }
    }
    e = &(csound->sensEvents_state.evt);
    if (--(p->cyclesRemaining) <= 0) {  /* if done performing score segment: */
      if (!p->cyclesRemaining) {
        p->prvbt = p->curbt;            /* update beats and times */
        p->curbt = p->nxtbt;
        p->curp2 = p->nxtim;
        print_amp_values(csound, 1);    /* print amplitudes for this segment */
      }
      else                              /* this should only happen at */
        p->cyclesRemaining = 0;         /* beginning of performance   */
    }

 retest:
    while (p->cyclesRemaining <= 0) {   /* read each score event:       */
      if (e->opcod != '\0') {
        /* if there is a pending score event, handle it now */
        switch (e->opcod) {
          case 'e':                         /* end of score, */
          case 'l':                         /* lplay list,   */
          case 's':                         /* or section:   */
            if (frstoff != NULL) {          /*   if still have notes with  */
              p->nxtim = frstoff->offtim;   /*   finite length, wait until */
              p->nxtbt = frstoff->offbet;   /*   all are turned off        */
              break;
            }
            /* end of: 1: section, 2: score, 3: lplay list */
            retval = (e->opcod == 'l' ? 3 : (e->opcod == 's' ? 1 : 2));
            goto scode;
          default:                              /* q, i, f, a:              */
            process_score_event(csound, e, 0);  /*   handle event now       */
            e->opcod = '\0';                    /*   and get next one       */
            continue;
        }
      }
      else {
        /* else read next score event */
        if (O.usingcscore) {         /*   get next lplay event       */
          if (ep < epend)                              /* nxt event  */
            memcpy(e, (EVTBLK *) &((*ep++)->strarg), sizeof(EVTBLK));
          else                                         /* else lcode */
            memcpy(e, (EVTBLK *) &(lsect->strarg), sizeof(EVTBLK));
        }
        else if (!(rdscor(e)))       /*   or rd nxt evt from scorfil */
          e->opcod = 'e';
        currevent = e;
        switch (e->opcod) {
        case 'w':
          if (O.Beatmode)                     /* Beatmode: read 'w'  */
            settempo(csound, e->p2orig);      /*   to init the tempo */
          continue;                           /*   for this section  */
        case 'q':
        case 'i':
        case 'f':
        case 'a':
          p->nxtim = (double) e->p[2] + p->timeOffs;
          p->nxtbt = (double) e->p2orig + p->beatOffs;
          break;
        case 'e':
        case 'l':
        case 's':
          continue;
        default:
          printf(Str("error in score.  illegal opcode %c (ASCII %d)\n"),
                 e->opcod, e->opcod);
          perferrcnt++;
          continue;
        }
      }
      /* calculate the number of k-periods remaining until next event */
      if (O.Beatmode)
        p->cyclesRemaining = RNDINT((p->nxtbt - p->curBeat) / p->curBeat_inc);
      else
        p->cyclesRemaining = RNDINT((p->nxtim - p->curTime) / p->curTime_inc);
    }

    /* handle any real time events now: */
    /* FIXME: the initialisation pass of real time */
    /*   events is not sorted by instrument number */
    /*   (although it never was sorted anyway...)  */
    if (O.RTevents) {
      if (O.OrcEvts) {                      /* orchestra rtevents */
        while ((sensType = SENSEORCEVT(csound)) != 0) {
          if ((retval = process_rt_event(csound, sensType)) != 0)
            goto scode;
        }
      }
      if (O.Midiin || O.FMidiin) {          /* MIDI note messages */
        while ((sensType = sensMidi(csound)) != 0) {
          if ((retval = process_rt_event(csound, sensType)) != 0)
            goto scode;
        }
      }
      if (O.Linein) {                       /* Linein events      */
        while ((sensType = sensLine()) != 0) {
          if ((retval = process_rt_event(csound, sensType)) != 0)
            goto scode;
        }
      }
    }

    /* no score event at this time, return to continue performance */
    return 0;

 scode:
    /* end of section (retval == 1), score (retval == 2), */
    /* or lplay list (retval == 3) */
    e->opcod = '\0';
    if (retval == 3) {
      section_amps(csound, 2);
      return 1;
    }
    /* for s, or e after s */
    if (retval == 1 || (retval == 2 && sectno > 1)) {
      p->nxtim = p->timeOffs = p->curp2;
      p->prvbt = p->nxtbt = p->beatOffs = p->curbt;
      section_amps(csound, 1);
    }
    else
      section_amps(csound, 0);
    if (retval == 1) {                        /* if s code,        */
      orcompact(csound);                      /*   rtn inactiv spc */
      if (actanchor.nxtact == NULL)           /*   if no indef ins */
        rlsmemfiles(csound);                  /*    purge memfiles */
      printf(Str("SECTION %d:\n"), ++sectno);
      goto retest;                            /*   & back for more */
    }
    return 2;                   /* done with entire score */
}

/* play all events in a score or an lplay list */

static int playevents(ENVIRON *csound)
{
    int retval;

    while ((retval = sensevents(csound)) == 0) {
      if (!O.initonly)
        kperf(csound);
    }
    return (retval - 1);
}

/* Schedule new score event to be played. 'time_ofs' is the amount of */
/* time in seconds to add to evt->p[2] to get the actual start time   */
/* of the event (measured from the beginning of performance, and not  */
/* section) in seconds.                                               */
/* If 'allow_now' is non-zero and event type is 'f', the function     */
/* table may be created immediately depending on start time.          */
/* Required parameters in 'evt':                                      */
/*   char   *strarg   string argument of event (NULL if none)         */
/*   char   opcod     event opcode (a, e, f, i, l, q, s)              */
/*   short  pcnt      number of p-fields (>=3 for q, i, a; >=4 for f) */
/*   MYFLT  p[]       array of p-fields, p[1]..p[pcnt] should be set  */
/*  p2orig and p3orig are calculated from p[2] and p[3].              */
/* The contents of 'evt', including the string argument, need not be  */
/* preserved after calling this function, as a copy of the event is   */
/* made.                                                              */
/* Return value is zero on success.                                   */

int insert_score_event(ENVIRON *csound, EVTBLK *evt, double time_ofs,
                       int allow_now)
{
    double        start_time;
    EVTNODE       *e, *prv;
    sensEvents_t  *st;
    MYFLT         *p;
    unsigned long start_kcnt;
    int           i, retval;

    retval = -1;
    st = &(csound->sensEvents_state);
    /* make a copy of the event... */
    if (csound->freeEvtNodes != NULL) {             /* pop alloc from stack */
      e = csound->freeEvtNodes;                     /*   if available       */
      csound->freeEvtNodes = e->nxt;
    }
    else
      e = (EVTNODE*) mcalloc(csound, sizeof(EVTNODE));  /* or alloc new one */
    if (evt->strarg != NULL) {  /* copy string argument if present */
      e->evt.strarg =
            (char*) mmalloc(csound, (size_t) strlen(evt->strarg) + (size_t) 1);
      strcpy(e->evt.strarg, evt->strarg);
    }
    else
      e->evt.strarg = NULL;
    e->evt.opcod = evt->opcod;
    e->evt.pcnt = evt->pcnt;
    p = &(e->evt.p[0]);
    p[0] = FL(0.0);             /* FIXME: is this ever used ? */
    i = 0;
    while (++i <= evt->pcnt)    /* copy p-field list */
      p[i] = evt->p[i];
    /* ...and use the copy from now on */
    evt = &(e->evt);

    /* check for required p-fields */
    switch (evt->opcod) {
      case 'f':
        if (evt->pcnt < 4)
          goto pfld_err;
      case 'i':
      case 'q':
      case 'a':
        if (evt->pcnt < 3)
          goto pfld_err;
        /* calculate actual start time in seconds and k-periods */
        start_time = (double) p[2] + time_ofs;
        start_kcnt = (unsigned long)
                            (start_time * (double) csound->global_ekr_ + 0.5);
        /* correct p2 value for section offset */
        p[2] = (MYFLT) (start_time - st->timeOffs);
        if (p[2] < FL(0.0))
          p[2] = FL(0.0);
        /* start beat: this is possibly wrong */
        evt->p2orig = (MYFLT) (((start_time - st->curTime) / st->beatTime)
                               + (st->curBeat - st->beatOffs));
        if (evt->p2orig < FL(0.0))
          evt->p2orig = FL(0.0);
        evt->p3orig = p[3];
        break;
      default:
        start_kcnt = 0UL;   /* compiler only */
    }

    switch (evt->opcod) {
      case 'f':                         /* function table */
        /* if event should be handled now: */
        if (allow_now &&
            start_kcnt <= (unsigned long) csound->global_kcounter_) {
          int initErr = csound->inerrcnt_;
          int perfErr = csound->perferrcnt_;
          process_score_event(csound, evt, 1);
          /* check for errors */
          if (csound->inerrcnt_ <= initErr && csound->perferrcnt_ <= perfErr)
            retval = 0;   /* no error */
          goto err_return;
        }
        break;
      case 'i':                         /* note event */
        /* calculate the length in beats */
        if (evt->p3orig > FL(0.0))
          evt->p3orig = (MYFLT) ((double) evt->p3orig / st->beatTime);
      case 'q':                         /* mute instrument */
        /* check for a valid instrument number or name */
        if (evt->strarg != NULL && p[1] == SSTRCOD)
          i = (int) named_instr_find(evt->strarg);
        else
          i = (int) fabs((double) p[1]);
        if (i < 1 || i > csound->maxinsno_ || csound->instrtxtp_[i] == NULL) {
          csoundMessage(csound, Str("insert_score_event(): invalid instrument "
                                    "number or name\n"));
          goto err_return;
        }
        break;
      case 'a':                         /* advance score time */
        /* calculate the length in beats */
        evt->p3orig = (MYFLT) ((double) evt->p3orig / st->beatTime);
        break;
      case 'e':                         /* end of score, */
      case 'l':                         /*   lplay list, */
      case 's':                         /*   section:    */
        evt->pcnt = 0;      /* no p-fields for these */
        start_kcnt = (time_ofs <= 0.0 ?
                      0UL : (unsigned long)
                              (time_ofs * (double) csound->global_ekr_ + 0.5));
        break;
      default:
        csoundMessage(csound, Str("insert_score_event(): unknown opcode: %c\n"),
                              evt->opcod);
        goto err_return;
    }
    /* queue new event */
    e->start_kcnt = start_kcnt;
    prv = csound->OrcTrigEvts;
    /* if list is empty, or at beginning of list: */
    if (prv == NULL || start_kcnt < prv->start_kcnt) {
      e->nxt = prv;
      csound->OrcTrigEvts = e;
    }
    else {                                      /* otherwise sort by time */
      while (prv->nxt != NULL && start_kcnt >= prv->nxt->start_kcnt)
        prv = prv->nxt;
      e->nxt = prv->nxt;
      prv->nxt = e;
    }
    /* Make sure sensevents() looks for RT events */
    csound->oparms_->RTevents = 1;
    csound->oparms_->ksensing = 1;
    csound->oparms_->OrcEvts  = 1;      /* - of the appropriate type */
    return 0;

 pfld_err:
    csoundMessage(csound, Str("insert_score_event(): insufficient p-fields\n"));
 err_return:
    /* clean up */
    if (e->evt.strarg != NULL)
      mfree(csound, e->evt.strarg);
    e->evt.strarg = NULL;
    e->nxt = csound->freeEvtNodes;
    csound->freeEvtNodes = e;
    return retval;
}

