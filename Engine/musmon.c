/*
    musmon.c:

    Copyright (C) 1991,2002 Barry Vercoe, John ffitch, Istvan Varga, rasmus ekman

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
int cleanup(void);
extern int sensMidi(void);
extern int sensFMidi(void);
extern long kperf(long);
extern  int      csoundYield(void*);

#define SEGAMPS 01
#define SORMSG  02

static int  insno;      /* IV - Oct 31 2002 */

long    srngcnt[MAXCHNLS], orngcnt[MAXCHNLS];
short   srngflg = 0;

extern  OPARMS  O;
/* extern  MEVENT  *Midevtblk, *FMidevtblk; */
static  EVTBLK  *scorevtblk;
static  short   offonly = 0;
static  short   sectno = 0;
/**
* This needs to be external to implement the  Csound API.
*/
long    kcnt = 0L;
static  MYFLT   timtot = FL(0.0);
static  MYFLT   betsiz, ekrbetsiz;
extern  int     MIDIinsert(int, MCHNBLK*, MEVENT*);
extern  int insert(int, EVTBLK*);

static void settempo(MYFLT tempo)
{
    if (tempo > FL(0.0)) {
      betsiz = FL(60.0) / tempo;
      ekrbetsiz = ekr * betsiz;
    }
}

int gettempo(GTEMPO *p)
{
    if (O.Beatmode) *p->ans = FL(60.0) / betsiz;
    else *p->ans = FL(60.0);
    return OK;
}

int tempset(TEMPO *p)
 {
    MYFLT tempo;

    if ((tempo = *p->istartempo) <= FL(0.0)) {
      return initerror(Str(X_864,"illegal istartempo value"));
    }
    else {
      settempo(tempo);
      p->prvtempo = tempo;
    return OK;
    }
}

int tempo(TEMPO *p)
{
    if (*p->ktempo != p->prvtempo) {
      settempo(*p->ktempo);
      p->prvtempo = *p->ktempo;
    }
    return OK;
}

extern  void    RTLineset(void), MidiOpen(void), FMidiOpen(void);
extern  void    scsort(FILE*, FILE*), oload(void), cscorinit(void);
extern  void    cscore(void), schedofftim(INSDS *), infoff(MYFLT);
extern  void    orcompact(void), rlsmemfiles(void), timexpire(MYFLT);
extern  void    beatexpire(MYFLT), deact(INSDS*), fgens(EVTBLK *);
extern  void    sfopenin(void), sfopenout(void), sfnopenout(void);
extern  void    iotranset(void), sfclosein(void), sfcloseout(void);
extern  int csoundIsExternalMidiEnabled(void *);
extern  void csoundExternalMidiOpen(void *);

extern  OPARMS  O;

static  int     playevents(void);
static  int     lplayed = 0;
static  int     segamps, sormsg;
static  EVTBLK  *e = NULL;
static  MYFLT   *maxp;
static  long    *rngp;
static  EVENT   **ep, **epend;  /* pointers for stepping through lplay list */
static  EVENT   *lsect = NULL;
static  MYFLT   curbt, curp2;
void    beep(void);
extern  void    RTclose(void);

#define MAXTIM 3600.
#define MAXONS 5

static TRNON turnons[MAXONS];
static TRNON *tpend = turnons + MAXONS;
TRNON *frsturnon = NULL;
void xturnon(int, long);
void kturnon(void);

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
    kcnt       = 0L;
    timtot     = FL(0.0);
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
    curbt      = FL(0.0);
    curp2      = FL(0.0);
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
    else {                                    /* dB values */
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

int musmon2(void);

int musmon(void)
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
    if (O.Midiin || csoundIsExternalMidiEnabled(&cenviron)) {
      /* Enable musmon to handle external MIDI input, if it has been enabled. */
      if(csoundIsExternalMidiEnabled(&cenviron)) {
        O.RTevents = 1;
        O.Midiin = 1;
        O.ksensing = 1;
      }
     MidiOpen();                     /*   alloc bufs & open files    */
     }
    dispinit();                 /* initialise graphics or character display */
    oload();                    /* set globals and run inits */
    if (O.FMidiin) FMidiOpen();
    printf(Str(X_1099,"orch now loaded\n"));
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
      printf(Str(X_956,"k-period aligned audio buffering\n"));
#ifdef mills_macintosh
      if (O.msglevel & WARNMSG)
        printf(Str(X_536,"WARNING: Will probably not work with playback routines\n"));
#endif
    }
    if (!O.inbufsamps) {                  /* else keep the user values    */
      if (!O.oMaxLag)  O.inbufsamps = IOBUFSAMPS;
      else             O.inbufsamps = O.oMaxLag;
      if (!O.outbufsamps) O.outbufsamps = IOBUFSAMPS; /* or get defaults  */
    }
#ifdef mills_macintosh
    if (IsNotAPowerOf2(O.outbufsamps) != 0) {
      O.outbufsamps = RoundDownToPowerOf2(O.outbufsamps);
      printf(Str(X_1164,"reset output buffer blocksize to power of 2 (%ld)\n"),
             O.outbufsamps);
    }
    if (IsNotAPowerOf2(O.inbufsamps) != 0) {
      O.inbufsamps = RoundDownToPowerOf2(O.inbufsamps);
      printf(Str(X_1163,"reset input buffers blocksize to power of 2 (%ld)\n"),
             O.inbufsamps);
    }
#endif
    printf(Str(X_602,"audio buffered in %d sample-frame blocks\n"),
           O.outbufsamps);
    O.inbufsamps *= nchnls;         /* now adjusted for n channels  */
    O.outbufsamps *= nchnls;
    if (O.sfread)                   /* if audio-in requested,       */
      sfopenin();                   /*   open/read? infile or device */
    if (O.sfwrite)                  /* if audio-out requested,      */
      sfopenout();                  /*   open the outfile or device */
    else sfnopenout();
    iotranset();                    /* point recv & tran to audio formatter */

    curp2 = curbt = FL(0.0);
    if (O.Beatmode)                 /* if performing from beats */
      settempo((MYFLT)O.cmdTempo);  /*   set the initial tempo  */

    /*        if (!O.Linein) { */   /*  *************** */
    if (!(scfp = fopen(O.playscore, "r"))) {
      if (!O.Linein) {
        dies(Str(X_649,"cannot reopen %s"), O.playscore);
      }
    }
    if (O.usingcscore) {
      if (lsect == NULL) {
        lsect = (EVENT *) mmalloc((long)sizeof(EVENT));
        lsect->op = 'l';
      }
      printf(Str(X_1361,"using Cscore processing\n"));
      if (!(oscfp = fopen("cscore.out", "w")))  /* override stdout in   */
        die(Str(X_631,"cannot create cscore.out"));/* rdscor for cscorefns */
      cscorinit();
      cscore();         /* call cscore, optionally re-enter via lplay() */
      fclose(oscfp);
      fclose(scfp); scfp = NULL;
      if (lplayed) return 0;

      if (!(scfp = fopen("cscore.out", "r")))   /*   rd from cscore.out */
        die(Str(X_650,"cannot reopen cscore.out"));
      if (!(oscfp = fopen("cscore.srt", "w")))  /*   writ to cscore.srt */
        die(Str(X_651,"cannot reopen cscore.srt"));
      printf(Str(X_1196,"sorting cscore.out ..\n"));
      scsort(scfp, oscfp);                      /* call the sorter again */
      fclose(scfp); scfp = NULL;
      fclose(oscfp);
      printf(Str(X_564,"\t... done\n"));
      if (!(scfp = fopen("cscore.srt", "r")))   /*   rd from cscore.srt */
        die(Str(X_651,"cannot reopen cscore.srt"));
      printf(Str(X_1127,"playing from cscore.srt\n"));
      O.usingcscore = 0;
    }
    if (e == NULL)
      e = scorevtblk = (EVTBLK *) mmalloc((long)sizeof(EVTBLK));
    printf(Str(X_448,"SECTION %d:\n"),++sectno);
#ifdef mills_macintosh
    fflush(stdout);
#endif

    if (frsturnon != NULL)           /* if something in turnon list  */
      kturnon();                     /*   turnon now                 */

                                     /* since we are running in components */
    return 0;                        /* we exit here to playevents later   */
}

int musmon2(void)
{
    playevents();                      /* play all events in the score */

    return cleanup();
}


int cleanup(void)
{
    int     n;

    printf(Str(X_716,"end of score.\t\t   overall amps:"));
    if (scfp) {
      fclose(scfp); scfp = NULL;
    }
    for (maxp=omaxamp, n=nchnls; n--; )
      print_maxamp (*maxp++);                   /* IV - Jul 9 2002 */
    if (O.outformat != AE_FLOAT) {
      printf(Str(X_554,"\n\t   overall samples out of range:"));
      for (rngp=orngcnt, n=nchnls; n--; )
        printf("%9ld", *rngp++);
    }
    /*      if (O.Linein)  RTclose(); */ /* now done via atexit */
    printf(Str(X_548,"\n%d errors in performance\n"),perferrcnt);
    if (O.sfread)
      sfclosein();
    if (O.sfwrite)
      sfcloseout();
    else printf(Str(X_1061,"no sound written to disk\n"));
    if (O.ringbell) beep();
    return dispexit();      /* hold or terminate the display output     */
    /* for Mac, dispexit returns 0 to exit immediately */
}

void beep(void)
{
#ifdef mills_macintosh
    SysBeep(10000);
#else
    printf(Str(X_28,"%c\tbeep!\n"),'\007');
#endif
}

int lplay(EVLIST *a)           /* cscore re-entry into musmon */
{
    lplayed = 1;
    if (!sectno)  printf(Str(X_448,"SECTION %d:\n"),++sectno);
    ep = &a->e[1];            /* from 1st evlist member */
    epend = ep + a->nevents;  /*   to last              */
    playevents();             /* play list members      */
    return OK;
}

int turnon(TURNON *p)          /* make list to turn on instrs for indef perf */
{                               /* called from i0 for execution in playevents */
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
      printf(Str(X_1293,"too many turnons waiting\n"));
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

void kturnon(void)      /* turn on instrs due in turnon list */
{                       /* called by kperf when frsturnon set & ktime ready */
    int insno;
    TRNON *tp = turnons;
    EVTBLK *e = (EVTBLK *) mmalloc((long)sizeof(EVTBLK));
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
        printf(Str(X_1299,"turnon deleted. instr %d undefined\n"), insno);
        perferrcnt++;
      }
      else if ((n = insert(insno, e)) != 0) {
        printf(Str(X_1298,"turnon deleted. instr %d "), insno);
        if (n < 0) printf(Str(X_990,"memory fault\n"));
        else printf(Str(X_822,"had %d init errors\n"), n);
        perferrcnt++;
      }
      tp->insno = 0;                            /*      & mark it done */
      tp++;
    }
    mfree((char *)e);
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

static int playevents(void)  /* play all events in a score or an lplay list */
{
    int     n;
    MYFLT   prvbt, nxtim, nxtbt;
    MYFLT   *smaxp;
    unsigned long    *maxps, *smaxps;
    long    *srngp;
    char    opcod;

    if (frsturnon != NULL)         /* if something in turnon list  */
      kturnon();                   /*   turnon now                 */
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
#define RNDINT(x) ((int)(x + FL(0.5)))
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
          if (O.Beatmode) {
            if (RNDINT(ekrbetsiz*frstoff->offbet) < RNDINT(ekrbetsiz*e->p2orig))
              goto setoff;
          }
          else {
            if (RNDINT(ekr*frstoff->offtim) < RNDINT(ekr*e->p[2]))
              goto setoff;
          }
        }
        nxtim = e->p[2];
        nxtbt = e->p2orig;
        if (O.Beatmode) {
          if (RNDINT(ekrbetsiz*curp2) < RNDINT(ekrbetsiz*nxtim))
            /* if we havent gotten to the next starttime then */
            offonly = 1;      /* need to set this to avoid inserting too early */
        } else {
          if (RNDINT(ekr*curp2) < RNDINT(ekr*nxtim))
            /* if we havent gotten to the next starttime then */
            offonly = 1;      /* need to set this to avoid inserting too early */
        }
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
        nxtim = frstoff->offtim;
        nxtbt = frstoff->offbet;
        offonly = 1;
        break;
      default:
        printf(Str(X_723,"error in score.  illegal opcode %c (ASCII %d)\n"),
               e->opcod, e->opcod);
        perferrcnt++;
        continue;
      }
      if (MTrkend && O.termifend && frstoff == NULL) {
        printf(Str(X_1278,"terminating. "));
        return(0);             /* aborting with perf incomplete */
      }
      if (O.Beatmode)
        kcnt = (long)((nxtbt - curbt) * ekrbetsiz + FL(0.5));
      else kcnt = (long)((nxtim - curp2) * ekr + FL(0.5));
      if (kcnt > 0) {                 /* perf for kcnt kprds  */
        long kdone, kperf(long);
        if (!O.initonly
            && (kdone = kperf(kcnt)) < kcnt) { /* early rtn:  RTevent  */
          curp2 += kdone * onedkr;             /*    update only curp2 */
          if (sensType == 1) {                 /*    for Linein,       */
            e = Linevtblk;                     /*      get its evtblk  */
            e->p[2] = curp2;                   /*      & insert curp2  */
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
          if (!kdone)                 /* if null duration,    */
            goto mtest;               /*  chk for midi on-off */
          if (segamps || (sormsg && rngflg))
            printf("  rtevent:\t   T%7.3f TT%7.3f M:", curp2,timtot+curp2);
        }
        else {                        /* else a score event:  */
          prvbt = curbt;
          curbt = nxtbt;              /*    update beats too  */
          curp2 = nxtim;
          if (segamps || (sormsg && rngflg))
            printf("B%7.3f ..%7.3f T%7.3f TT%7.3f M:",
                   prvbt,  curbt,  curp2,  timtot+curp2);
        }
        if (segamps || (sormsg && rngflg)) {
          for (n=nchnls, maxp=maxamp; n--;)
            print_maxamp (*maxp++);             /* IV - Jul 9 2002 */
          printf("\n");
          if (rngflg) {
            printf(Str(X_560,"\t number of samples out of range:"));
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
/*            printf("sectionpos[%d](%p->%p) %ld -> ",
                      maxp-maxamp, maxps, smaxps, *smaxps); */
            *smaxps = *maxps;
/*                printf("%ld\n", *smaxps); */
          }
          *maxp = 0.0f;
          *maxps = 0;
          *srngp++ += *rngp;
          *rngp++ = 0;
        }
      }
      if (sensType == 0) { /* if this was a score or turnoff time:  */
        if (frstoff != NULL) {            /* if turnoffs pending,   */
          if (O.Beatmode)
            beatexpire(curbt + hfkprd);   /*  rm any expired instrs */
          else timexpire(curp2 + hfkprd);
        }
        if (offonly)
          goto retest;                    /*  if offonly, loop back */
      }
    mtest:
      /* Old test: is faster */
      /* if (sensType >= 2) { */          /* Midievent:             */
      /* New version (re Aug 1999): */
      if (sensType == 2 || sensType == 3) {        /* Midievent:    */
        MEVENT *mep;
        MCHNBLK *chn;

        if (sensType == 2)                 /* realtime or Midifile  */
          mep = Midevtblk;
        else mep = FMidevtblk;
        chn = M_CHNBP[mep->chan];
        insno = chn->pgmno;
        if (mep->type == NOTEON_TYPE && mep->dat2) { /* midi note ON: */
          if ((n = MIDIinsert(insno,chn,mep)))    {  /* alloc,init,activ */
            printf(Str(X_568,"\t\t   T%7.3f - note deleted. "), curp2);
            printf(Str(X_926,"instr %d had %d init errors\n"), insno, n);
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
      else switch(e->opcod) {          /* scorevt or Linevt:     */
      case 'e':
        goto scode;             /* quit realtime */
      case 'q':
        if (e->p[1] == SSTRCOD && e->strarg) {          /* IV - Oct 31 2002 */
          if ((insno = (int) named_instr_find(e->strarg)) < 1) {
            if (sensType) printf("\t\t   T%7.3f",curp2);
            else  printf("\t  B%7.3f",curbt);
            printf(Str(X_1808," - note deleted. instr %s undefined\n"), e->strarg);
            perferrcnt++; break;
          }
          printf(Str(X_1809,"Setting instrument %s %s\n"),
                 e->strarg, (e->p[3]==0 ? Str(X_1811,"off") : Str(X_1810,"on")));
          instrtxtp[insno]->muted = (short)e->p[3];
        }
        else {                                          /* IV - Oct 31 2002 */
          insno = abs((int)e->p[1]);
          if (insno > maxinsno || instrtxtp[insno] == NULL) {
            if (sensType) printf("\t\t   T%7.3f",curp2);
            else  printf("\t  B%7.3f",curbt);
            printf(Str(X_16," - note deleted. instr %d(%d) undefined\n"),
                   insno, maxinsno);
            perferrcnt++;
          }
          else {
            printf(Str(X_1812,"Setting instrument %d %s\n"),
                   insno, (e->p[3]==0 ? Str(X_1811,"off") : (Str(X_1810,"on"))));
            instrtxtp[insno]->muted = (short)e->p[3];
          }
        }
        break;
      case 'i':
        if (e->p[1] == SSTRCOD && e->strarg) {          /* IV - Oct 31 2002 */
          if ((insno = (int) named_instr_find(e->strarg)) < 1) {
            if (sensType) printf("\t\t   T%7.3f",curp2);
            else  printf("\t  B%7.3f",curbt);
            printf(Str(X_1808," - note deleted. instr %s undefined\n"), e->strarg);
            perferrcnt++; break;
          }
          e->p[1] = (MYFLT) insno;
          if (O.Beatmode && e->p3orig >= 0.)
            e->p[3] = e->p3orig * betsiz;
          if ((n = insert(insno,e))) {  /* else aloc,init,activat */
            if (sensType) printf("\t\t   T%7.3f",curp2);
            else  printf("\t  B%7.3f",curbt);
            printf(Str(X_1813," - note deleted.  i%d (%s) had %d init errors\n"),
                   insno, e->strarg, n);
            perferrcnt++;
          }
        }
        else {                                          /* IV - Oct 31 2002 */
          insno = abs((int)e->p[1]);
          if (insno > maxinsno || instrtxtp[insno] == NULL) {
            if (sensType) printf("\t\t   T%7.3f",curp2);
            else  printf("\t  B%7.3f",curbt);
            printf(Str(X_16," - note deleted. instr %d(%d) undefined\n"),
                   insno, maxinsno);
            perferrcnt++;
          }
          else if (e->p[1] < FL(0.0))  /* if p1 neg,             */
            infoff(-e->p[1]);   /*  turnoff any infin cpy */
          else {
            if (O.Beatmode && e->p3orig >= FL(0.0))
              e->p[3] = e->p3orig * betsiz;
            if ((n = insert(insno,e))) {  /* else aloc,init,activat */
              if (sensType) printf("\t\t   T%7.3f",curp2);
              else  printf("\t  B%7.3f",curbt);
              printf(Str(X_15," - note deleted.  i%d had %d init errors\n"),
                     insno, n);
              perferrcnt++;
            }
          }
        }
        break;
      case 'f':
        fgens(e);
        break;
      case 'a':
        curp2 = e->p[2] + e->p[3];
        curbt = e->p2orig + e->p3orig;
        printf(Str(X_1280,"time advanced %5.3f beats by score request\n"),
               e->p3orig);
        break;
      }
      if (sensType) {                /* RT event now done:          */
        if (sensType == 4)           /* Free sched(k)when event     */
          mfree(e);
        sensType = 0;
        e = scorevtblk;              /*    return to score context  */
        goto retest;                 /*    and resume the kperf     */
      }
      continue;                      /* else get next score event   */

    lcode:
      printf(Str(X_714,"end of lplay event list\t      peak amps:"));
      for (n=nchnls, maxp=smaxamp; n--; )
        print_maxamp (*maxp++);                 /* IV - Jul 9 2002 */
      printf("\n");
      if (srngflg) {
        printf(Str(X_560,"\t number of samples out of range:"));
        for (n=nchnls, srngp=srngcnt; n--; )
          printf("%9ld", *srngp++);
        printf("\n");
        srngflg = 0;
      }
      return(0);

    scode:
      if ((opcod = e->opcod) == 's'       /* for s, or e after s   */
          || (opcod == 'e' && sectno > 1)) {
        timtot += curp2;
        prvbt = curbt = curp2 = FL(0.0);
        printf(Str(X_717,"end of section %d\t sect peak amps:"),sectno);
        for (n=nchnls, maxp=smaxamp; n--; )
          print_maxamp (*maxp++);               /* IV - Jul 9 2002 */
        printf("\n");
        if (srngflg) {
          printf(Str(X_560,"\t number of samples out of range:"));
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
/*              printf("overallpos[%d](%f) (%p->%p) %ld -> %ld\n",
                       maxp-omaxamp, *maxp, maxps, smaxps, *maxps, *smaxps); */
          *maxps = *smaxps;                     /* And where */
        }
        *smaxp = 0.0f;
        *smaxps = 0;
        *rngp++ += *srngp;          /*   and orng counts */
        *srngp++ = 0;
      }
      if (opcod == 's') {                       /* if s code,        */
        orcompact();                            /*   rtn inactiv spc */
        if (actanchor.nxtact == NULL)           /*   if no indef ins */
          rlsmemfiles();                        /*    purge memfiles */
        curp2 = curbt = FL(0.0);                /*   reset sec times */
        printf(Str(X_448,"SECTION %d:\n"), ++sectno);
#ifdef mills_macintosh
        fflush(stdout);
#endif
      }                                         /*   & back for more */
      else break;
    }
    return(1);  /* done with entire score */
}

/* a reworking of playevents to process one event at a time
   outside of the audio process loop */
int sensevents(void)
{
    static MYFLT   prvbt = FL(0.0), nxtim = FL(0.0), nxtbt = FL(0.0);
    static MYFLT   *smaxp;
    static unsigned long    *maxps, *smaxps;
    static long    *srngp;

    static int needsCleanup = 0;

    int     n;
    char    opcod;

    if (!csoundYield(&cenviron)) longjmp(cenviron.exitjmp_,1);

    /* read each score event:       */
    if (kcnt <= 0) {
      /* if we just finished a kcnt Run, cleanup */
      if (needsCleanup) {
        prvbt = curbt;
        curbt = nxtbt;              /*    update beats too  */
        curp2 = nxtim;

        if (segamps || (sormsg && rngflg))
          printf("B%7.3f ..%7.3f T%7.3f TT%7.3f M:",
                 prvbt,  curbt,  curp2,  timtot+curp2);
        if (segamps || (sormsg && rngflg)) {
          for (n=nchnls, maxp=maxamp; n--;)
            print_maxamp (*maxp++);             /* IV - Jul 9 2002 */
          printf("\n");
          if (rngflg) {
            printf(Str(X_560,"\t number of samples out of range:"));
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
/*             printf("sectionpos[%d](%p->%p) %ld -> ", */
/*                    maxp-maxamp, maxps, smaxps, *smaxps); */
            *smaxps = *maxps;
/*             printf("%ld\n", *smaxps); */
          }

          *maxp = 0.0f;
          *maxps = 0;
          *srngp++ += *rngp;
          *rngp++ = 0;
        }

        /* if (sensType == 0) {*/ /* if this was a score or turnoff time:  */
        if (frstoff != NULL) {      /* if turnoffs pending,   */
          if (O.Beatmode)
            beatexpire(curbt + hfkprd); /* rm any expired instrs */
          else
            timexpire(curp2 + hfkprd);
        }
        /* if we may have only perfed stuff before the current event,
           which then means the event is still valid, so skip over
           the new event stuff */
        if (offonly)
          goto retest;            /*  if offonly, loop back */
        /*} */
      }

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

      switch(e->opcod) {
      case 'w':
        if (O.Beatmode)                     /* Beatmode: read 'w'  */
          settempo(e->p2orig);              /*   to init the tempo */
        return(0);                          /*   for this section  */
      case 'i':
      case 'f':
      case 'a':
        if (frstoff != NULL) {
          if (O.Beatmode) {
            if (frstoff->offbet < e->p2orig)
              goto setoff;
          }
          else {
            if (frstoff->offtim < e->p[2])
              goto setoff;
          }
        }
        nxtim = e->p[2];
        nxtbt = e->p2orig;
        /* **** This seems to fix a Macintosh problem **** */
#ifndef __MACH__
        if (curp2 < nxtim)  /* if we havent gotten to the next starttime then */
          offonly = 1;      /* need to set this to avoid inserting too early */
#else
        if ((int)(ekr*curp2) < (int)(ekr*nxtim))
                            /* if we havent gotten to the next starttime then */
          offonly = 1;      /* need to set this to avoid inserting too early */
#endif
        /* **** End of Mac fix **** */
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
        nxtim = frstoff->offtim;
        nxtbt = frstoff->offbet;
        offonly = 1;
        break;
      default:
        printf(Str(X_723,"error in score.  illegal opcode %c (ASCII %d)\n"),
               e->opcod, e->opcod);
        perferrcnt++;
        return(0);
      }

      if (MTrkend && O.termifend && frstoff == NULL) {
        printf(Str(X_1278,"terminating. "));
        return(0);             /* aborting with perf incomplete */
      }

      if (O.Beatmode)
        kcnt = (long)((nxtbt - curbt) * ekrbetsiz + FL(0.5));
      else
        kcnt = (long)((nxtim - curp2) * ekr + FL(0.5));

      needsCleanup = kcnt;
    }

    /* sense realtime events */
    if (O.RTevents) {
      if ((O.Midiin && (sensType = sensMidi())) ||      /* if MIDI note message */
          (O.FMidiin && kcounter >= FMidiNxtk && (sensType = sensFMidi())) ||
          (O.Linein && (sensType = sensLine())) ||    /* or Linein event */
          (O.OrcEvts && (sensType = sensOrcEvent()))) /* or triginstr event */
                                                    /*(re Aug 1999) */
        {
          /* calc p2 */
          curp2 = (nxtim - kcnt * onedkr);
          curbt = (nxtbt - kcnt * onedkr);

          if (sensType == 1) {                 /*    for Linein,       */
            e = Linevtblk;                     /*      get its evtblk  */
            e->p[2] = curp2;                   /*      & insert curp2  */
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
/*           else if (sensType == 4) {   /\* Realtime orc event (re Aug 1999) *\/ */
/*             EVTNODE *evtlist = OrcTrigEvts.nxtevt; */
/*             while (evtlist && evtlist->kstart <= kcounter) { */
/*               int insno = evtlist->insno; */
/*               e = &evtlist->evt; */
/*               evtlist = OrcTrigEvts.nxtevt = evtlist->nxtevt; */
/*               /\* If several events pending we must insert all but one *\/ */
/*               if (evtlist && evtlist->kstart <= kcounter) { */
/*                 /\* code copied from switch(e->opcod) case 'i' below *\/ */
/*                 if (e->p[1] < FL(0.0))       /\* if p1 neg,             *\/ */
/*                   infoff(-e->p[1]);          /\*  turnoff any infin cpy *\/ */
/*                 else { */
/*                   if (O.Beatmode && e->p3orig >= FL(0.0)) */
/*                     e->p[3] = e->p3orig * betsiz; */
/*                   if (n = insert(insno, e)) {  /\* alloc,init,activate *\/ */
/*                     printf("\t\t   T%7.3f",curp2); */
/*                     printf(Str(X_1481, */
/*                                "schedkwhen note deleted." */
/*                                "  i%d had %d init errors\n"), */
/*                            insno, n); */
/*                     perferrcnt++; */
/*                   } */
/*                   /\* free event mmalloced in schedule.c *\/ */
/*                   /\* --- end code copy --- *\/ */
/*                   mfree(e); /\* free event allocated in schedule.c *\/ */
/*                 } */
/*               } */
/*             } */
/*             if (OrcTrigEvts.nxtevt == NULL) O.OrcEvts = 0; */
/*           } */

          /* if (!kdone) */                 /* if null duration,    */
          /*  goto mtest; */               /*  chk for midi on-off */

          if (segamps || (sormsg && rngflg))
            printf("  rtevent:\t   T%7.3f TT%7.3f M:", curp2,timtot+curp2);
        }
    }

    /* if no rtEvent */
    if (sensType == 0) {
      if (offonly)
        return(0);
    }

/*  mtest: */
    /* Old test: is faster */
    /* if (sensType >= 2) { */        /* Midievent:             */
    /* New version (re Aug 1999): */
    if (sensType == 2 || sensType == 3) {      /* Midievent:    */
      MEVENT *mep;
      MCHNBLK *chn;

      if (sensType == 2)             /* realtime or Midifile  */
        mep = Midevtblk;
      else mep = FMidevtblk;
      chn = M_CHNBP[mep->chan];
      insno = chn->pgmno;
      if (mep->type == NOTEON_TYPE && mep->dat2) { /* midi note ON: */
        if ((n = MIDIinsert(insno,chn,mep))) {  /* alloc,init,activ */
          printf(Str(X_568,"\t\t   T%7.3f - note deleted. "), curp2);
          printf(Str(X_926,"instr %d had %d init errors\n"), insno, n);
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
    else switch(e->opcod) {          /* scorevt or Linevt:     */
    case 'e':
      goto scode;               /* quit realtime */
    case 'i':
      if (e->p[1] == SSTRCOD && e->strarg) {          /* IV - Oct 31 2002 */
        if ((insno = (int) named_instr_find(e->strarg)) < 1) {
          if (sensType) printf("\t\t   T%7.3f",curp2);
          else  printf("\t  B%7.3f",curbt);
          printf(Str(X_1808," - note deleted. instr %s undefined\n"), e->strarg);
          perferrcnt++; break;
        }
        e->p[1] = (MYFLT) insno;
        if (O.Beatmode && e->p3orig >= 0.)
          e->p[3] = e->p3orig * betsiz;
        if ((n = insert(insno,e))) {  /* else aloc,init,activat */
          if (sensType) printf("\t\t   T%7.3f", curp2);
          else  printf("\t  B%7.3f", curbt);
          printf(Str(X_1813," - note deleted.  i%d (%s) had %d init errors\n"),
                 insno, e->strarg, n);
          perferrcnt++;
        }
      }
      else {                                          /* IV - Oct 31 2002 */
        insno = abs((int)e->p[1]);
        if (insno > maxinsno || instrtxtp[insno] == NULL) {
          if (sensType) printf("\t\t   T%7.3f",curp2);
          else  printf("\t  B%7.3f",curbt);
          printf(Str(X_16," - note deleted. instr %d(%d) undefined\n"),
                 insno, maxinsno);
          perferrcnt++;
        }
        else if (e->p[1] < FL(0.0))  /* if p1 neg,             */
          infoff(-e->p[1]);   /*  turnoff any infin cpy */
        else {
          if (O.Beatmode && e->p3orig >= FL(0.0))
            e->p[3] = e->p3orig * betsiz;
          if ((n = insert(insno,e))) {  /* else aloc,init,activat */
            if (sensType) printf("\t\t   T%7.3f",curp2);
            else  printf("\t  B%7.3f",curbt);
            printf(Str(X_15," - note deleted.  i%d had %d init errors\n"),
                   insno, n);
            perferrcnt++;
          }
        }
      }
      break;
    case 'f':
      fgens(e);
      break;
    case 'a':
      curp2 = e->p[2] + e->p[3];
      curbt = e->p2orig + e->p3orig;
      printf(Str(X_1280,"time advanced %5.3f beats by score request\n"),
             e->p3orig);
      break;
    }

    if (sensType) {                /* RT event now done:          */
      if (sensType == 4)           /* Free sched(k)when event     */
        mfree(e);
      sensType = 0;
      e = scorevtblk;              /*    return to score context  */
      goto retest;                 /*    and resume the kperf     */
    }
    return(0);                     /* else get next score event   */

 lcode:
    printf(Str(X_714,"end of lplay event list\t      peak amps:"));
    for (n=nchnls, maxp=smaxamp; n--; )
      print_maxamp (*maxp++);                   /* IV - Jul 9 2002 */
    printf("\n");
    if (srngflg) {
      printf(Str(X_560,"\t number of samples out of range:"));
      for (n=nchnls, srngp=srngcnt; n--; )
        printf("%9ld", *srngp++);
      printf("\n");
      srngflg = 0;
    }
    return(0);

 scode:
    if ((opcod = e->opcod) == 's' ||      /* for s, or e after s   */
        (opcod == 'e' && sectno > 1)) {
      timtot += curp2;
      prvbt = curbt = curp2 = FL(0.0);
      printf(Str(X_717,"end of section %d\t sect peak amps:"),sectno);
      for (n=nchnls, maxp=smaxamp; n--; )
        print_maxamp (*maxp++);                 /* IV - Jul 9 2002 */
      printf("\n");
      if (srngflg) {
        printf(Str(X_560,"\t number of samples out of range:"));
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
        /*      printf("overallpos[%d](%f) (%p->%p) %ld -> %ld\n", */
        /*            maxp-omaxamp, *maxp, maxps, smaxps, *maxps, *smaxps); */
        *maxps = *smaxps;                     /* And where */
      }
      *smaxp = FL(0.0);
      *smaxps = 0;
      *rngp++ += *srngp;          /*   and orng counts */
      *srngp++ = 0;
    }
    if (opcod == 's') {                       /* if s code,        */
      orcompact();                            /*   rtn inactiv spc */
      if (actanchor.nxtact == NULL)           /*   if no indef ins */
        rlsmemfiles();                        /*    purge memfiles */
      curp2 = curbt = FL(0.0);                /*   reset sec times */
      printf(Str(X_448,"SECTION %d:\n"), ++sectno);
#ifdef mills_macintosh
      fflush(stdout);
#endif
      return(0);
    }

    return(1);  /* done with entire score */
}


/* fillbuffer() calls kperf just enough so that an audioOutBuffer
   was filled [with audwrite() or rtplay()]

   sensEventRate is the number of kpasses per real-time or score event
   processing.  if sensEventRate <= 0, then no event processing will
   take place, allowing it to be called in separate threads, etc.

   on return:  0 = OK, 1 = endOfScore or termination
*/
int fillbuffer(int sensEventRate)
{
    static int sensCount = 0; /* number of k-passes since last event processed */
    static int sampsNeeded = 0; /* number of samps still needed to create
                                   before returning */

    int done = 0;
    int sampsPerKperf = ksmps*nchnls;

    /* if we are handling events outside of this function and we have no
       kcnts to process, then we get out of here now as nothing for us to do */
    if (sensEventRate <= 0 && kcnt <= 0)
      return (0);

    sampsNeeded += O.outbufsamps;

    while (!done && sampsNeeded > 0) {
      if (++sensCount == sensEventRate) {
        done = sensevents();
        sensCount = 0;
      }

      if (done)
        return done;

      if (/*!done &&*/ kcnt) {  /* done must be false cos of previous line */
        /*  we have to override the RTevent processing in kperf(), and rather
            than rewrite it, we override the RTevents param here and then set
            it back after calling kperf
            RTevents could change after a sensevents() call */
        int rtEvents = O.RTevents;
        O.RTevents = 0;
        kperf(1);
        kcnt -= 1;
        sampsNeeded -= sampsPerKperf;
        O.RTevents = rtEvents;
      }
    }

    return(done);
}
