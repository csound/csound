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
#include "oload.h" /* for TRNON */
#include "soundio.h"
#include "schedule.h"
#include <math.h>
#include "namedins.h"   /* IV - Oct 31 2002 */

#ifdef mills_macintosh
extern long IsNotAPowerOf2(unsigned long);
extern unsigned long RoundDownToPowerOF2(unsigned long);
#include <Types.h>
Str255 PPCVER;
extern void GetVersNumString(Str255 versStr);
#endif
int cleanup(void*);
extern  void    kperf(ENVIRON*);
extern  int     csoundYield(void*);

#define SEGAMPS 01
#define SORMSG  02

long    srngcnt[MAXCHNLS], orngcnt[MAXCHNLS];
short   srngflg = 0;

extern  OPARMS  O;
static  EVTBLK  *scorevtblk;
static  short   offonly = 0;
static  short   sectno = 0;

static  MYFLT   betsiz, ekrbetsiz;
extern  int     MIDIinsert(ENVIRON *, int, MCHNBLK*, MEVENT*);
extern  int     insert(ENVIRON *, int, EVTBLK*);

static void settempo(MYFLT tempo)
{
    if (tempo > FL(0.0)) {
      betsiz = FL(60.0) / tempo;
      ekrbetsiz = ekr * betsiz;
    }
}

int gettempo(ENVIRON *csound, GTEMPO *p)
{
    if (O.Beatmode) *p->ans = FL(60.0) / betsiz;
    else *p->ans = FL(60.0);
    return OK;
}

int tempset(ENVIRON *csound, TEMPO *p)
 {
    MYFLT tempo;

    if ((tempo = *p->istartempo) <= FL(0.0)) {
      return initerror(Str("illegal istartempo value"));
    }
    else {
      settempo(tempo);
      p->prvtempo = tempo;
    return OK;
    }
}

int tempo(ENVIRON *csound, TEMPO *p)
{
    if (*p->ktempo != p->prvtempo) {
      settempo(*p->ktempo);
      p->prvtempo = *p->ktempo;
    }
    return OK;
}

extern  void    RTLineset(void), MidiOpen(void *), FMidiOpen(void *);
extern  void    scsort(FILE*, FILE*), oload(ENVIRON *), cscorinit(void);
extern  void    schedofftim(INSDS *), infoff(MYFLT);
extern  void    orcompact(ENVIRON*), rlsmemfiles(void), timexpire(double);
extern  void    beatexpire(double), deact(INSDS*), fgens(ENVIRON *,EVTBLK *);
extern  void    sfopenin(void*), sfopenout(void*), sfnopenout(void);
extern  void    iotranset(void), sfclosein(void*), sfcloseout(void*);
extern  void    MidiClose(ENVIRON*);

extern  OPARMS  O;

static  int     playevents(ENVIRON *);
static  int     lplayed = 0;
static  int     segamps, sormsg;
static  EVTBLK  *e = NULL;
static  MYFLT   *maxp;
static  long    *rngp;
static  EVENT   **ep, **epend;  /* pointers for stepping through lplay list */
static  EVENT   *lsect = NULL;
void    beep(void);

/* #define MAXTIM 3600. */
#define MAXONS 5

static TRNON turnons[MAXONS];
static TRNON *tpend = turnons + MAXONS;
TRNON *frsturnon = NULL;
void xturnon(int, long);
void kturnon(ENVIRON*);

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
    sensType   = 0;
    scorevtblk = NULL;
    offonly    = 0;
    sectno     = 0;
    betsiz     = FL(0.0);
    ekrbetsiz  = FL(0.0);

    lplayed    = 0;
    segamps    = 0;
    sormsg     = 0;
    e          = NULL;
    maxp       = NULL;
    rngp       = NULL;
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
    if (O.Midiin) {
      /* Enable musmon to handle external MIDI input, if it has been enabled. */
      O.RTevents = 1;
      O.Midiin = 1;
      O.ksensing = 1;
      MidiOpen(csound);         /*   alloc bufs & open files    */
    }
    dispinit();                 /* initialise graphics or character display */
    oload(csound);              /* set globals and run inits */
    if (O.FMidiin) FMidiOpen(csound);
    printf(Str("orch now loaded\n"));
#ifdef mills_macintosh
    fflush(stdout);
#endif

    multichan = (nchnls > 1) ? 1:0;
    maxampend = &maxamp[nchnls];
    segamps = O.msglevel & SEGAMPS;
    sormsg = O.msglevel & SORMSG;

    if (O.Linein)  RTLineset();     /* if realtime input expected   */
    if (O.outbufsamps < 0) {        /* if k-aligned iobufs requestd */
      O.inbufsamps = O.outbufsamps *= -ksmps; /*  set from absolute value */
      printf(Str("k-period aligned audio buffering\n"));
#ifdef mills_macintosh
      if (O.msglevel & WARNMSG)
        printf(Str("WARNING: Will probably not work with playback routines\n"));
#endif
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
      if (O.inbufsamps >= (O.oMaxLag >> 1))
        O.inbufsamps = (O.oMaxLag >> 1);
    }
    if (O.outfilename != NULL &&
        (strncmp(O.outfilename, "dac", 3) == 0 ||
         strncmp(O.outfilename, "devaudio", 8) == 0)) {
      if (O.outbufsamps >= (O.oMaxLag >> 1))
        O.outbufsamps = (O.oMaxLag >> 1);
    }
#ifdef mills_macintosh
    if (IsNotAPowerOf2(O.outbufsamps) != 0) {
      O.outbufsamps = RoundDownToPowerOf2(O.outbufsamps);
      printf(Str("reset output buffer blocksize to power of 2 (%ld)\n"),
             O.outbufsamps);
    }
    if (IsNotAPowerOf2(O.inbufsamps) != 0) {
      O.inbufsamps = RoundDownToPowerOf2(O.inbufsamps);
      printf(Str("reset input buffers blocksize to power of 2 (%ld)\n"),
             O.inbufsamps);
    }
#endif
    printf(Str("audio buffered in %d sample-frame blocks\n"),
           O.outbufsamps);
    O.inbufsamps *= nchnls;         /* now adjusted for n channels  */
    O.outbufsamps *= nchnls;
    if (O.sfread)                   /* if audio-in requested,       */
      sfopenin(csound);             /*   open/read? infile or device */
    if (O.sfwrite)                  /* if audio-out requested,      */
      sfopenout(csound);            /*   open the outfile or device */
    else sfnopenout();
    iotranset();                    /* point recv & tran to audio formatter */

    if (O.Beatmode)                 /* if performing from beats */
      settempo((MYFLT)O.cmdTempo);  /*   set the initial tempo  */

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
    if (e == NULL)
      e = scorevtblk = (EVTBLK *) mmalloc(csound, (long)sizeof(EVTBLK));
    printf(Str("SECTION %d:\n"),++sectno);
#ifdef mills_macintosh
    fflush(stdout);
#endif

    if (frsturnon != NULL)           /* if something in turnon list  */
      kturnon(csound);               /*   turnon now                 */

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

int turnon(ENVIRON *csound, TURNON *p) /* make list to turn on instrs for indef */
{                            /* perf called from i0 for execution in playevents */
    int insno = (int)*p->insno;
    long xtratim = (long)(*p->itime * ekr);
    xturnon(insno, xtratim);
    return OK;
}

void xturnon(int insno, long xtratim) /* schedule a later turnon        */
{                               /* called by above & ctrlr 111  */
    TRNON *tp = turnons;
    long ktime = kcounter + xtratim;

    while (tp->insno && ++tp < tpend);  /* quick see if list is full */
    if (tp >= tpend)
      printf(Str("too many turnons waiting\n"));
    else {
      TRNON *prvtp = tp - 1;
      while (prvtp >= turnons && prvtp->ktime > ktime) {
        tp->insno = prvtp->insno;
        tp->ktime = prvtp->ktime;       /* else move later ones down */
        tp--; prvtp--;
      }
      tp->insno = insno;                        /*   & insert the new one */
      tp->ktime = ktime;
    }
    frsturnon = turnons;
}

void kturnon(ENVIRON *csound)/* turn on instrs due in turnon list */
{                            /* called by kperf when frsturnon set & ktime ready */
    int insno;
    TRNON *tp = turnons;
    EVTBLK *e = (EVTBLK *) mmalloc(csound, (long)sizeof(EVTBLK));
    e->opcod = 'i';
    e->pcnt = 3;
    /*    e->strlen = 0; */
    e->offtim = -FL(1.0);     /* indef perf */
    e->p[2] = FL(0.0);
    e->p[3] = -FL(1.0);
    while (tp < tpend && (insno = tp->insno) && tp->ktime <= kcounter) {
      int n;
      e->p[1] = (MYFLT)insno;                   /* for each instr due */
      if (instrtxtp[insno] == NULL) {           /*     turnon now     */
        printf(Str("turnon deleted. instr %d undefined\n"), insno);
        perferrcnt++;
      }
      else if ((n = insert(csound,insno, e)) != 0) {
        printf(Str("turnon deleted. instr %d "), insno);
        if (n < 0) printf(Str("memory fault\n"));
        else printf(Str("had %d init errors\n"), n);
        perferrcnt++;
      }
      tp->insno = 0;                            /*      & mark it done */
      tp++;
    }
    mfree(csound, (char *)e);
    if (tp < tpend && tp->insno) {              /*   if list non-empty  */
      TRNON *newtp = turnons;
      do {
        newtp->insno = tp->insno;               /*   shuffle to the top */
        newtp->ktime = tp->ktime;
        tp->insno = 0;
      } while (++tp < tpend && tp->insno && newtp++);
      frsturnon = turnons;                      /*   & set gbl pointer */
    } else frsturnon = NULL;
}

/* IV - Feb 05 2005 */
#define RNDINT(x) ((int) ((double) (x) + ((double) (x) < 0.0 ? -0.5 : 0.5)))

extern  int     sensLine(void);
extern  int     sensMidi(ENVIRON *), sensFMidi(ENVIRON *);

/* sense events for one k-period            */
/* return value is one of the following:    */
/*   0: continue performance                */
/*   1: terminate (e.g. end of MIDI file)   */
/*   2: normal end of score                 */

int sensevents(ENVIRON *csound)
{
    sensEvents_t  *p;
    int           n, insno;
    MYFLT         *smaxp;
    unsigned long *maxps, *smaxps;
    long          *srngp;
    char          opcod = '\0';

    p = &(csound->sensEvents_state);
    if (!p->init_done) {
      /* at beginning of performance: */
      p->prvbt = p->curbt = p->nxtbt = p->curp2 = p->nxtim = p->timtot = 0.0;
      p->init_done = 1;
      p->cyclesRemaining = p->kDone = p->saved_opcod = 0;
      sensType = 0;
    }
    else {
      if (--(p->cyclesRemaining) <= 0)
        p->cyclesRemaining = 0;
      p->kDone++;
      /* sense real-time events */
      n = 1;
      if (p->cyclesRemaining && O.RTevents) {
        if ((O.Midiin && (sensType = sensMidi(csound))) ||  /* if MIDI note message */
            (O.FMidiin && kcounter >= csound->midiGlobals->FMidiNxtk &&
             (sensType = sensFMidi(csound))) ||
            (O.Linein && (sensType = sensLine())) ||  /* or Linein event */
            (O.OrcEvts && (sensType = sensOrcEvent()))) /* or triginstr event */
        n = 0;                                          /*   (re Aug 1999) */
      }
      if (!p->cyclesRemaining)
        n = 0;
      if (n)            /* if there are no real-time events, and still have */
        return 0;       /* k-periods to perform, continue performance */
      /* otherwise process events now */
      smaxp = NULL; maxps = NULL; smaxps = NULL; srngp = NULL;
      opcod = (char) p->saved_opcod;
      goto kperf_cont;
    }

    if (frsturnon != NULL)         /* if something in turnon list  */
      kturnon(csound);             /*   turnon now                 */
    while (1) {                    /* read each score event:       */
      if (O.usingcscore) {         /*   i.e. get next lplay event  */
        if (ep < epend)
          e = (EVTBLK *) &((*ep++)->strarg);         /* nxt event  */
        else e = (EVTBLK *) &(lsect->strarg);        /* else lcode */
      }
      else if (!(rdscor(e)))       /*   or rd nxt evt from scorfil */
        e->opcod = 'e';
    retest:
      offonly = 0;
      currevent = e;
      switch (e->opcod) {
      case 'w':
        if (O.Beatmode)                     /* Beatmode: read 'w'  */
          settempo(e->p2orig);              /*   to init the tempo */
        continue;                           /*   for this section  */
      case 'q':
      case 'i':
      case 'f':
      case 'a':
        if (frstoff != NULL) {
          if (O.Beatmode && frstoff->offbet < e->p2orig)
            goto setoff;
          else if (!O.Beatmode && frstoff->offtim < e->p[2])
            goto setoff;
        }
        p->nxtim = (double) e->p[2];
        p->nxtbt = (double) e->p2orig;
        if (O.Beatmode && ((p->nxtbt - p->curbt) * (double) ekrbetsiz) > 0.51)
                        /* if we havent gotten to the next starttime then */
          offonly = 1;  /* need to set this to avoid inserting too early */
        else if (!O.Beatmode && ((p->nxtim - p->curp2) * (double) ekr) > 0.51)
                        /* if we havent gotten to the next starttime then */
          offonly = 1;  /* need to set this to avoid inserting too early */
        break;
      case 'l':
        if (frstoff != NULL)
          goto setoff;
        goto lcode;
      case 's':
      case 'e':
        if (frstoff == NULL)
          goto scode;
      setoff:
        p->nxtim = (double) frstoff->offtim;
        p->nxtbt = (double) frstoff->offbet;
        offonly = 1;
        break;
      default:
        printf(Str("error in score.  illegal opcode %c (ASCII %d)\n"),
               e->opcod, e->opcod);
        perferrcnt++;
        continue;
      }
      if (MTrkend && O.termifend && frstoff == NULL) {
        printf(Str("terminating. "));
        return 1;              /* aborting with perf incomplete */
      }
      if (O.Beatmode)
        p->cyclesRemaining = RNDINT(((double) p->nxtbt - (double) p->curbt)
                                    * (double) ekrbetsiz);
      else
        p->cyclesRemaining = RNDINT(((double) p->nxtim - (double) p->curp2)
                                    * (double) ekr);
      if (p->cyclesRemaining > 0) {                 /* perf for kcnt kprds  */
        p->kDone = 0;
        /* sense real-time events */
        if (O.RTevents &&
            ((O.Midiin && (sensType = sensMidi(csound))) || /* if MIDI note message */
             (O.FMidiin && kcounter >= csound->midiGlobals->FMidiNxtk &&
              (sensType = sensFMidi(csound))) ||
             (O.Linein && (sensType = sensLine())) || /* or Linein event */
             (O.OrcEvts && (sensType = sensOrcEvent()))))   /* or triginstr */
          goto kperf_cont;                          /*  event (re Aug 1999) */
        /* no events, return to continue performance */
        p->saved_opcod = (int) opcod;
        return 0;
 kperf_cont:
        /* ======== IV - Feb 05 2005: return from performance ======== */
        if (p->cyclesRemaining) {               /* early rtn:  RTevent  */
          p->curp2 += (double) p->kDone
                      * (double) onedkr;        /*    update only curp2 */
          if (sensType == 1) {                  /*    for Linein,       */
            e = Linevtblk;                      /*      get its evtblk  */
            e->p[2] = p->curp2;                 /*      & insert curp2  */
          }
          else if (sensType == 4) {   /* Realtime orc event (re Aug 1999) */
            EVTNODE *evtlist = OrcTrigEvts.nxtevt;
            /* Events are sorted on insertion, so just check the first */
            if (evtlist && evtlist->kstart <= kcounter) {
              e = &evtlist->evt;
              /* Pop from the list, as e is mfree'd below */
              evtlist = OrcTrigEvts.nxtevt = evtlist->nxtevt;
            }
            if (OrcTrigEvts.nxtevt == NULL) O.OrcEvts = 0;
          }
          if (!p->kDone)                 /* if null duration,    */
            goto mtest;               /*  chk for midi on-off */
          if (segamps || (sormsg && rngflg))
            printf("  rtevent:\t   T%7.3f TT%7.3f M:",
                   p->curp2, p->timtot + p->curp2);
        }
        else {                        /* else a score event:  */
          p->prvbt = p->curbt;
          p->curbt = p->nxtbt;              /*    update beats too  */
          p->curp2 = p->nxtim;
          if (segamps || (sormsg && rngflg))
            printf("B%7.3f ..%7.3f T%7.3f TT%7.3f M:",
                   p->prvbt,  p->curbt,  p->curp2,  p->timtot + p->curp2);
        }
        if (segamps || (sormsg && rngflg)) {
          for (n=nchnls, maxp=maxamp; n--;)
            print_maxamp (*maxp++);             /* IV - Jul 9 2002 */
          printf("\n");
          if (rngflg) {
            printf(Str("\t number of samples out of range:"));
            for (n=nchnls, rngp=rngcnt; n--;)
              printf("%9ld", *rngp++);
            printf("\n");
            rngflg = 0;
            srngflg++;
          }
        }
        for (n=nchnls,maxp=maxamp-1,smaxp=smaxamp-1,
               maxps=maxpos-1,smaxps=smaxpos-1,
               rngp=rngcnt,srngp=srngcnt; n--; ) {
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
      if (sensType == 0) { /* if this was a score or turnoff time:  */
        if (frstoff != NULL) {            /* if turnoffs pending,   */
          if (O.Beatmode)                 /*  rm any expired instrs */
            beatexpire((double) p->curbt + (0.51 / (double) ekrbetsiz));
          else
            timexpire((double) p->curp2 + (0.51 / (double) ekr));
        }
        if (offonly)
          goto retest;                    /*  if offonly, loop back */
      }
    mtest:
      /* New version (re Aug 1999): */
      if (sensType == 2 || sensType == 3) {        /* Midievent:    */
        MEVENT *mep;
        MCHNBLK *chn;

        if (sensType == 2)                 /* realtime or Midifile  */
          mep = csound->midiGlobals->Midevtblk;
        else
          mep = csound->midiGlobals->FMidevtblk;
        chn = M_CHNBP[mep->chan];
        insno = chn->pgmno;
        if (mep->type == NOTEON_TYPE && mep->dat2) { /* midi note ON: */
          if ((n = MIDIinsert(csound,insno,chn,mep))) {  /* alloc,init,activ */
            printf(Str("\t\t   T%7.3f - note deleted. "), p->curp2);
            printf(Str("instr %d had %d init errors\n"), insno, n);
            perferrcnt++;
          }
        }
        else {                         /* else midi note OFF:    */
          short pch = mep->dat1;
          INSDS *ip = chn->kinsptr[pch];
          if (ip == NULL)              /*  if already off, done  */
            Mxtroffs++;
          else {
            chn->kinsptr[pch] = NULL;  /*  mark this key off     */
            if (chn->sustaining) {     /*  if sustain pedal on   */
              chn->ksusptr[pch] = ip;
              chn->ksuscnt++;          /*    let the note ring   */
            }
            else {                     /*  else some kind of off */
              do {
                if (!csoundYield(&cenviron)) longjmp(cenviron.exitjmp_,1);
                if (ip->xtratim) {     /*    if offtime delayed  */
                  ip->relesing = 1;    /*     enter reles phase  */
                  ip->offtim = (kcounter + ip->xtratim) * onedkr;
                  schedofftim(ip);     /*      & put in offqueue */
                }
                else deact(ip);        /*    else off now        */
              } while ((ip = ip->nxtolap) != NULL);
            }
          }
        }
      }
      else switch (e->opcod) {         /* scorevt or Linevt:     */
      case 'e':
        goto scode;             /* quit realtime */
      case 'q':
        if (e->p[1] == SSTRCOD && e->strarg) {          /* IV - Oct 31 2002 */
          if ((insno = (int) named_instr_find(e->strarg)) < 1) {
            if (sensType) printf("\t\t   T%7.3f",p->curp2);
            else  printf("\t  B%7.3f",p->curbt);
            printf(Str(" - note deleted. instr %s undefined\n"), e->strarg);
            perferrcnt++; break;
          }
          printf(Str("Setting instrument %s %s\n"),
                 e->strarg, (e->p[3]==0 ? Str("off") : Str("on")));
          instrtxtp[insno]->muted = (short)e->p[3];
        }
        else {                                          /* IV - Oct 31 2002 */
          insno = abs((int)e->p[1]);
          if (insno > maxinsno || instrtxtp[insno] == NULL) {
            if (sensType) printf("\t\t   T%7.3f",p->curp2);
            else  printf("\t  B%7.3f",p->curbt);
            printf(Str(" - note deleted. instr %d(%d) undefined\n"),
                   insno, maxinsno);
            perferrcnt++;
          }
          else {
            printf(Str("Setting instrument %d %s\n"),
                   insno, (e->p[3]==0 ? Str("off") : (Str("on"))));
            instrtxtp[insno]->muted = (short)e->p[3];
          }
        }
        break;
      case 'i':
        if (e->p[1] == SSTRCOD && e->strarg) {          /* IV - Oct 31 2002 */
          if ((insno = (int) named_instr_find(e->strarg)) < 1) {
            if (sensType) printf("\t\t   T%7.3f",p->curp2);
            else  printf("\t  B%7.3f",p->curbt);
            printf(Str(" - note deleted. instr %s undefined\n"), e->strarg);
            perferrcnt++; break;
          }
          e->p[1] = (MYFLT) insno;
          if (O.Beatmode && e->p3orig >= 0.)
            e->p[3] = e->p3orig * betsiz;
          if ((n = insert(csound, insno,e))) {  /* else aloc,init,activat */
            if (sensType) printf("\t\t   T%7.3f",p->curp2);
            else  printf("\t  B%7.3f",p->curbt);
            printf(Str(" - note deleted.  i%d (%s) had %d init errors\n"),
                   insno, e->strarg, n);
            perferrcnt++;
          }
        }
        else {                                          /* IV - Oct 31 2002 */
          insno = abs((int)e->p[1]);
          if (insno > maxinsno || instrtxtp[insno] == NULL) {
            if (sensType) printf("\t\t   T%7.3f",p->curp2);
            else  printf("\t  B%7.3f",p->curbt);
            printf(Str(" - note deleted. instr %d(%d) undefined\n"),
                   insno, maxinsno);
            perferrcnt++;
          }
          else if (e->p[1] < FL(0.0))  /* if p1 neg,             */
            infoff(-e->p[1]);   /*  turnoff any infin cpy */
          else {
            if (O.Beatmode && e->p3orig >= FL(0.0))
              e->p[3] = e->p3orig * betsiz;
            if ((n = insert(csound,insno,e))) {  /* else aloc,init,activat */
              if (sensType) printf("\t\t   T%7.3f",p->curp2);
              else  printf("\t  B%7.3f",p->curbt);
              printf(Str(" - note deleted.  i%d had %d init errors\n"),
                     insno, n);
              perferrcnt++;
            }
          }
        }
        break;
      case 'f':
        fgens(csound, e);
        break;
      case 'a':
        p->curp2 = e->p[2] + e->p[3];
        p->curbt = e->p2orig + e->p3orig;
        printf(Str("time advanced %5.3f beats by score request\n"),
               e->p3orig);
        break;
      }
      if (sensType) {                /* RT event now done:          */
        if (sensType == 4)           /* Free sched(k)when event     */
          mfree(csound, e);
        sensType = 0;
        e = scorevtblk;              /*    return to score context  */
        goto retest;                 /*    and resume the kperf     */
      }
      continue;                      /* else get next score event   */

    lcode:
      printf(Str("end of lplay event list\t      peak amps:"));
      for (n=nchnls, maxp=smaxamp; n--; )
        print_maxamp (*maxp++);                 /* IV - Jul 9 2002 */
      printf("\n");
      if (srngflg) {
        printf(Str("\t number of samples out of range:"));
        for (n=nchnls, srngp=srngcnt; n--; )
          printf("%9ld", *srngp++);
        printf("\n");
        srngflg = 0;
      }
      return 1;

    scode:
      if ((opcod = e->opcod) == 's'       /* for s, or e after s   */
          || (opcod == 'e' && sectno > 1)) {
        p->timtot += p->curp2;
        p->curp2 = p->nxtim = 0.0;
        p->prvbt = p->curbt = p->nxtbt = 0.0;
        printf(Str("end of section %d\t sect peak amps:"),sectno);
        for (n=nchnls, maxp=smaxamp; n--; )
          print_maxamp (*maxp++);               /* IV - Jul 9 2002 */
        printf("\n");
        if (srngflg) {
          printf(Str("\t number of samples out of range:"));
          for (n=nchnls, srngp=srngcnt; n--; )
            printf("%9ld", *srngp++);
          printf("\n");
          srngflg = 0;
        }
      }
      for (n=nchnls,
             smaxp=smaxamp-1, maxp=omaxamp-1,
             smaxps=smaxpos-1,maxps=omaxpos-1,
             srngp=srngcnt,   rngp=orngcnt; n--; ) {
        ++maxps; ++smaxps;
        if (*++smaxp > *++maxp) {
          *maxp = *smaxp;                       /* keep ovrl maxamps */
          *maxps = *smaxps;                     /* And where */
        }
        *smaxp = FL(0.0);
        *smaxps = 0;
        *rngp++ += *srngp;          /*   and orng counts */
        *srngp++ = 0;
      }
      if (opcod == 's') {                       /* if s code,        */
        orcompact(csound);                      /*   rtn inactiv spc */
        if (actanchor.nxtact == NULL)           /*   if no indef ins */
          rlsmemfiles();                        /*    purge memfiles */
        p->curp2 = p->nxtim = 0.0;              /*   reset sec times */
        p->prvbt = p->curbt = p->nxtbt = 0.0;
        printf(Str("SECTION %d:\n"), ++sectno);
#ifdef mills_macintosh
        fflush(stdout);
#endif
      }                                         /*   & back for more */
      else break;
    }
    return 2;   /* done with entire score */
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

