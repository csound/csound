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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include "csoundCore.h"         /*                         MUSMON.C     */
#include "midiops.h"
#include "soundio.h"
#include "namedins.h"
#include "oload.h"
#include "remote.h"
#include <math.h>
#include "corfile.h"

#include "csdebug.h"

#define SEGAMPS CS_AMPLMSG
#define SORMSG  CS_RNGEMSG

int     MIDIinsert(CSOUND *, int, MCHNBLK*, MEVENT*);
  int     insert(CSOUND *, int, EVTBLK*);
  void    MidiOpen(CSOUND *);
  void    m_chn_init_all(CSOUND *);
//  char *  scsortstr(CSOUND *, CORFIL *);
  void    infoff(CSOUND*, MYFLT), orcompact(CSOUND*);
  void    beatexpire(CSOUND *, double), timexpire(CSOUND *, double);
  void    sfopenin(CSOUND *), sfopenout(CSOUND*), sfnopenout(CSOUND*);
  void    iotranset(CSOUND *), sfclosein(CSOUND*), sfcloseout(CSOUND*);
  void    MidiClose(CSOUND *);
  void    RTclose(CSOUND *);
  void    remote_Cleanup(CSOUND *);
  char    **csoundGetSearchPathFromEnv(CSOUND *, const char *);
void    openMIDIout(CSOUND *);
void print_csound_version(CSOUND*);

#ifdef HAVE_PTHREAD_SPIN_LOCK
#define RT_SPIN_TRYLOCK { int trylock = CSOUND_SUCCESS; \
  if(csound->oparms->realtime)             \
  trylock = csoundSpinTryLock(&csound->alloc_spinlock);      \
  if(trylock == CSOUND_SUCCESS) {
#else
#define RT_SPIN_TRYLOCK csoundSpinLock(&csound->alloc_spinlock);
#endif

#ifdef HAVE_PTHREAD_SPIN_LOCK
#define RT_SPIN_UNLOCK \
if(csound->oparms->realtime) \
  csoundSpinUnLock(&csound->alloc_spinlock); \
  trylock = CSOUND_SUCCESS; } }
#else
#define RT_SPIN_UNLOCK csoundSpinUnLock(&csound->alloc_spinlock);
#endif

/* extern  void    initialize_instrument0(CSOUND *); */

typedef struct evt_cb_func {
  void    (*func)(CSOUND *, void *);
  void    *userData;
  struct evt_cb_func  *nxt;
} EVT_CB_FUNC;

#define STA(x)   (csound->musmonStatics.x)

/**
  Open and Initialises the input/output
  returns the HW sampling rate if it has been
  set, -1.0 otherwise.
*/
MYFLT csoundInitialiseIO(CSOUND *csound) {
    OPARMS *O = csound->oparms;
    if (csound->enableHostImplementedAudioIO &&
        csound->hostRequestedBufferSize) {
      int bufsize    = (int) csound->hostRequestedBufferSize;
      int ksmps      = (int) csound->ksmps;
      bufsize        = (bufsize + (ksmps >> 1)) / ksmps;
      bufsize        = (bufsize ? bufsize * ksmps : ksmps);
      O->outbufsamps = O->inbufsamps = bufsize;
    }
    else {
      if (!O->oMaxLag)
        O->oMaxLag = IODACSAMPS;
      if (!O->outbufsamps)
        O->outbufsamps = IOBUFSAMPS;
      else if (UNLIKELY(O->outbufsamps < 0)) { /* if k-aligned iobufs requested  */
        /* set from absolute value */
        O->outbufsamps *= -((int64_t)csound->ksmps);
        csound->ErrorMsg(csound, Str("k-period aligned audio buffering\n"));
        if (O->oMaxLag <= O->outbufsamps)
          O->oMaxLag = O->outbufsamps << 1;
      }
      /* else keep the user values */
      /* IV - Feb 04 2005: make sure that buffer sizes for real time audio */
      /* are usable */
      if (check_rtaudio_name(O->infilename, NULL, 0) >= 0 ||
          check_rtaudio_name(O->outfilename, NULL, 1) >= 0) {
        O->oMaxLag = ((O->oMaxLag + O->outbufsamps - 1) / O->outbufsamps)
          * O->outbufsamps;
        if (O->oMaxLag <= O->outbufsamps && O->outbufsamps > 1)
          O->outbufsamps >>= 1;
      }
      O->inbufsamps = O->outbufsamps;
    }
     csound->ErrorMsg(csound, Str("audio buffered in %d sample-frame blocks\n"),
                    (int) O->outbufsamps);
    O->inbufsamps  *= csound->inchnls;    /* now adjusted for n channels  */
    O->outbufsamps *= csound->nchnls;
    iotranset(csound);          /* point recv & tran to audio formatter */
    /* open audio file or device for input first, and then for output */
    if (!csound->enableHostImplementedAudioIO) {
      if (O->sfread)
        sfopenin(csound);
      if (O->sfwrite && !csound->initonly)
        sfopenout(csound);
      else
       sfnopenout(csound);
    }
    csound->io_initialised = 1;
    return csound->system_sr(csound, 0);
 }


/* IV - Jan 28 2005 */
void print_benchmark_info(CSOUND *csound, const char *s)
{
  double  rt, ct;

  if ((csound->oparms->msglevel & CS_TIMEMSG) == 0 || csound->csRtClock == NULL)
    return;
  rt = csoundGetRealTime(csound->csRtClock);
  ct = csoundGetCPUTime(csound->csRtClock);
  csound->ErrorMsg(csound,
                  Str("Elapsed time at %s: real: %.3fs, CPU: %.3fs\n"),
                  (char*) s, rt, ct);
}

static void settempo(CSOUND *csound, double tempo)
{
    if (tempo <= 0.0) return;
    if (csound->oparms->Beatmode==1)
      csound->ibeatTime = (int64_t)(csound->esr*60.0 / tempo);
    csound->curBeat_inc = tempo / (60.0 * (double) csound->ekr);
}

int gettempo(CSOUND *csound, GTEMPO *p)
{
    if (LIKELY(csound->oparms->Beatmode)) {
      *p->ans = FL(60.0) * csound->esr / (MYFLT)csound->ibeatTime;
    }
    else
      *p->ans = FL(60.0);
    return OK;
}

int tempset(CSOUND *csound, TEMPO *p)
{
    double tempo;

    if (UNLIKELY((tempo = (double)*p->istartempo) <= FL(0.0))) {
      return csound->InitError(csound, Str("illegal istartempo value"));
    }
    if (UNLIKELY(csound->oparms->Beatmode==0))
      return csound->InitError(csound, Str("Beat mode not in force"));
    settempo(csound, tempo);
    p->prvtempo = (MYFLT)tempo;
    return OK;
}

int tempo(CSOUND *csound, TEMPO *p)
{
    if (*p->ktempo != p->prvtempo) {
      settempo(csound, (double)*p->ktempo);
      p->prvtempo = *p->ktempo;
    }
    return OK;
}

static void print_maxamp(CSOUND *csound, MYFLT x)
{
    int   attr = 0;
    if (!(csound->oparms->msglevel & 0x60)) {   /* 0x00: raw amplitudes */
      if (csound->oparms->msglevel & 0x300) {
        MYFLT y = x / csound->e0dbfs;     /* relative level */
        if (UNLIKELY(y >= FL(1.0)))                    /* >= 0 dB: red */
          attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_RED;
        else if (csound->oparms->msglevel & 0x200) {
          if (y >= FL(0.5))                            /* -6..0 dB: yellow */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_CYAN; /* was yellow but... */
          else if (y >= FL(0.125))                      /* -24..-6 dB: green */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_GREEN;
          else                                          /* -200..-24 dB: blue */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_BLUE;
        }
      }
      if (csound->e0dbfs > FL(3000.0))
        csoundMessageS(csound, attr, "%9.1f", x);
      else if (csound->e0dbfs < FL(3.0))
        csoundMessageS(csound, attr, "%9.5f", x);
      else if (csound->e0dbfs > FL(300.0))
       csoundMessageS(csound, attr, "%9.2f", x);
      else if (csound->e0dbfs > FL(30.0))
        csoundMessageS(csound, attr, "%9.3f", x);
      else
        csoundMessageS(csound, attr, "%9.4f", x);
    }
    else {                              /* dB values */
      MYFLT y = x / csound->e0dbfs;     /* relative level */
      if (UNLIKELY(y < FL(1.0e-10))) {
        /* less than -200 dB: print zero */
        csound->Message(csound, "      0  ");
        return;
      }
      y = FL(20.0) * (MYFLT) log10((double) y);
      if (csound->oparms->msglevel & 0x40) {
        if (UNLIKELY(y >= FL(0.0)))                     /* >= 0 dB: red */
          attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_RED;
        else if (csound->oparms->msglevel & 0x20) {
          if (y >= FL(-6.0))                            /* -6..0 dB: yellow */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_YELLOW;
          else if (y >= FL(-24.0))                      /* -24..-6 dB: green */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_GREEN;
          else                                          /* -200..-24 dB: blue */
            attr = CSOUNDMSG_FG_BOLD | CSOUNDMSG_FG_BLUE;
        }
      }
      csoundMessageS(csound, attr, "%+9.2f", y);
    }
}

void print_engine_parameters(CSOUND *csound);
void print_sndfile_version(CSOUND* csound);

int musmon(CSOUND *csound)
{
    OPARMS  *O = csound->oparms;
    /* VL - 08-07-21 messages moved here so we can switch them off */
    print_csound_version(csound);
    print_sndfile_version(csound);

    /* initialise search path cache */
    csoundGetSearchPathFromEnv(csound, "SNAPDIR");
    csoundGetSearchPathFromEnv(csound, "SFDIR;SSDIR;INCDIR");
    csoundGetSearchPathFromEnv(csound, "SFDIR");
    csoundGetSearchPathFromEnv(csound, "SADIR");
    csoundGetSearchPathFromEnv(csound, "SFDIR;SSDIR");

    m_chn_init_all(csound);     /* allocate MIDI channels */
    dispinit(csound);           /* initialise graphics or character display */

    reverbinit(csound);
    dbfs_init(csound, csound->e0dbfs);
    csound->nspout = csound->ksmps * csound->nchnls;  /* alloc spin & spout */
    csound->nspin = csound->ksmps * csound->inchnls; /* JPff: in preparation */
    csound->spin  = (MYFLT *) csound->Calloc(csound, csound->nspin*sizeof(MYFLT));
    csound->spraw = (MYFLT *) csound->Calloc(csound, csound->nspout*sizeof(MYFLT));
    csound->spout = (MYFLT *) csound->Calloc(csound, csound->nspout*sizeof(MYFLT));
    csound->auxspin = (MYFLT *) csound->Calloc(csound, csound->nspin*sizeof(MYFLT));
    /* memset(csound->maxamp, '\0', sizeof(MYFLT)*MAXCHNLS); */
    /* memset(csound->smaxamp, '\0', sizeof(MYFLT)*MAXCHNLS); */
    /* memset(csound->omaxamp, '\0', sizeof(MYFLT)*MAXCHNLS); */

    /* initialise sensevents state */
    csound->prvbt = csound->curbt = csound->nxtbt = 0.0;
    csound->curp2 = csound->nxtim = csound->timeOffs = csound->beatOffs = 0.0;
    csound->icurTime = 0L;
    if (O->Beatmode && O->cmdTempo > 0.0) {
      /* if performing from beats, set the initial tempo */
      csound->curBeat_inc = O->cmdTempo / (60.0 * (double) csound->ekr);
      csound->ibeatTime = (int64_t)(csound->esr*60.0 / O->cmdTempo);
    }
    else {
      csound->curBeat_inc = 1.0 / (double) csound->ekr;
      csound->ibeatTime = 1;
    }
    csound->cyclesRemaining = 0;
    memset(&(csound->evt), 0, sizeof(EVTBLK));

    print_engine_parameters(csound);

    /* run instr 0 inits */
    if (UNLIKELY(init0(csound) != 0))
      csoundDie(csound, Str("header init errors"));

    /* kperf() will not call csoundYield() more than 250 times per second */
    csound->evt_poll_cnt    = 0;
    csound->evt_poll_maxcnt =
      (int)(250.0 /(double) csound->ekr); /* VL this was wrong: kr/250 originally */
    /* Enable musmon to handle external MIDI input, if it has been enabled. */
    if (O->Midiin || O->FMidiin || O->RMidiin) {
      O->RTevents = 1;
      MidiOpen(csound);                 /*   alloc bufs & open files    */
    }
    /* open MIDI output (moved here from argdecode) */
    if (O->Midioutname != NULL && O->Midioutname[0] == (char) '\0')
      O->Midioutname = NULL;
    if (O->FMidioutname != NULL && O->FMidioutname[0] == (char) '\0')
      O->FMidioutname = NULL;
    if (O->Midioutname != NULL || O->FMidioutname != NULL)
      openMIDIout(csound);
    if(O->msglevel) {
      csound->ErrorMsg(csound, Str("orch now loaded\n"));
    }

    csound->multichan = (csound->nchnls > 1 ? 1 : 0);
    STA(segamps) = O->msglevel & SEGAMPS;
    STA(sormsg)  = O->msglevel & SORMSG;

    if (O->Linein)
      RTLineset(csound);                /* if realtime input expected   */

    // VL 01-05-2019
    // if --use-system-sr, this gets called earlier to override
    // the sampling rate. Otherwise it gets called here.
    if(!csound->io_initialised)
         csoundInitialiseIO(csound);

    if (O->playscore!=NULL) corfile_flush(csound, O->playscore);
    //csound->scfp
    if (UNLIKELY(O->usingcscore)) {
      if (STA(lsect) == NULL) {
        STA(lsect) = (EVENT*) csound->Malloc(csound, sizeof(EVENT));
        STA(lsect)->op = 'l';
      }
      csound->ErrorMsg(csound, Str("using Cscore processing\n"));
      /* override stdout in */
      if (UNLIKELY(!(csound->oscfp = fopen("cscore.out", "w"))))
        csoundDie(csound, Str("cannot create cscore.out"));
      csoundNotifyFileOpened(csound, "cscore.out", CSFTYPE_SCORE_OUT, 1, 0);
      /* rdscor for cscorefns */
      csoundInitializeCscore(csound, csound->scfp, csound->oscfp);
      /* call cscore, optionally re-enter via lplay() */
      csound->cscoreCallback_(csound);
      fclose(csound->oscfp); csound->oscfp = NULL;
      if (csound->scfp != NULL) {
        fclose(csound->scfp);
        csound->scfp = NULL;
      }
      if (STA(lplayed))
        return 0;

      /*  read from cscore.out */
      if (UNLIKELY(!(csound->scfp = fopen("cscore.out", "r")))) {
        csoundDie(csound, Str("cannot reopen cscore.out"));
      }
      else {
        CORFIL *inf = corfile_create_w(csound);
        int c;
        while ((c=getc(csound->scfp))!=EOF) corfile_putc(csound, c, inf);
        corfile_rewind(inf);
        csound->scorestr = inf;
        corfile_rm(csound, &csound->scstr);
      }
      csoundNotifyFileOpened(csound, "cscore.out", CSFTYPE_SCORE_OUT, 0, 0);
      /* write to cscore.srt */
      if (UNLIKELY(!(csound->oscfp = fopen("cscore.srt", "w"))))
        csoundDie(csound, Str("cannot reopen cscore.srt"));
      csoundNotifyFileOpened(csound, "cscore.srt", CSFTYPE_SCORE_OUT, 1, 0);
      csound->ErrorMsg(csound, Str("sorting cscore.out ..\n"));
      /* csound->scorestr = copy_to_corefile(csound, "cscore.srt", NULL, 1); */
      scsortstr(csound, csound->scorestr);  /* call the sorter again */
      fclose(csound->scfp); csound->scfp = NULL;
      fputs(corfile_body(csound->scstr), csound->oscfp);
      fclose(csound->oscfp); csound->oscfp = NULL;
      csound->ErrorMsg(csound, Str("\t... done\n"));
      csound->ErrorMsg(csound, Str("playing from cscore.srt\n"));
      O->usingcscore = 0;
    }

     csound->ErrorMsg(csound, Str("SECTION %d:\n"), ++STA(sectno));
    /* apply score offset if non-zero */
    if (csound->csoundScoreOffsetSeconds_ > FL(0.0))
      csoundSetScoreOffsetSeconds(csound, csound->csoundScoreOffsetSeconds_);

#ifndef __EMSCRIPTEN__
    if (csound->oparms->realtime && csound->event_insert_loop == 0){
      extern uintptr_t event_insert_thread(void *);
      csound->init_pass_threadlock = csoundCreateMutex(0);
      csound->ErrorMsg(csound, "Initialising spinlock...\n");
      csoundSpinLockInit(&csound->alloc_spinlock);
      csound->event_insert_loop = 1;
      csound->alloc_queue = (ALLOC_DATA *)
        csound->Calloc(csound, sizeof(ALLOC_DATA)*MAX_ALLOC_QUEUE);
      csound->event_insert_thread =
        csound->CreateThread(event_insert_thread,
                             (void*)csound);
      csound->ErrorMsg(csound, "Starting realtime mode queue: %p thread: %p\n",
                      csound->alloc_queue, csound->event_insert_thread );
    }
#endif

    /* since we are running in components, we exit here to playevents later */
    return 0;
}

static void deactivate_all_notes(CSOUND *csound)
{
    INSDS *ip = csound->actanchor.nxtact;

    while (ip != NULL) {
      INSDS *nxt = ip->nxtact;
#ifdef BETA
      csound->Message(csound, "deativate: ip, nxt = %p , %p\n", ip, nxt);
#endif
      xturnoff_now(csound, ip);
      // should not be needed -- if (ip == nxt) break;
      ip = nxt;
    }
}

static void delete_pending_rt_events(CSOUND *csound)
{
  EVTNODE *ep = csound->OrcTrigEvts;

  while (ep != NULL) {
    EVTNODE *nxt = ep->nxt;
    if (ep->evt.strarg != NULL) {
      csound->Free(csound,ep->evt.strarg);
      ep->evt.strarg = NULL;
    }
    /* push to stack of free event nodes */
    ep->nxt = csound->freeEvtNodes;
    csound->freeEvtNodes = ep;
    ep = nxt;
  }
  csound->OrcTrigEvts = NULL;
}

void delete_selected_rt_events(CSOUND *csound, MYFLT instr)
{
  EVTNODE *ep = csound->OrcTrigEvts;
  EVTNODE *last = NULL;
  while (ep != NULL) {
    EVTNODE *nxt = ep->nxt;
    //printf("*** delete_selected_rt_events: instr = %f, p[1] = %f\n",
    //instr, ep->evt.p[1]);
    if (ep->evt.opcod=='i' &&
        (((int)(ep->evt.p[1]) == instr) || (ep->evt.p[1] == instr))) {
      //printf(" ** found\n");
      // Found an event to cancel
      if (ep->evt.strarg != NULL) {
        // clearstring if necessary
        csound->Free(csound,ep->evt.strarg);
        ep->evt.strarg = NULL;
      }
      if (last) last->nxt = nxt; else csound->OrcTrigEvts = nxt;
      /* push to stack of free event nodes */
      ep->nxt = csound->freeEvtNodes;
      csound->freeEvtNodes = ep;
    }
    else last = ep;
    ep = nxt;
  }
  //csound->OrcTrigEvts = NULL;
}

static inline void cs_beep(CSOUND *csound)
{
    csound->ErrorMsg(csound, Str("%c\tbeep!\n"), '\a');
}

PUBLIC int csoundCleanup(CSOUND *csound)
{
    void    *p;
    MYFLT   *maxp;
    int32   *rngp;
    uint32_t n;

    csoundLockMutex(csound->API_lock);
    if (csound->QueryGlobalVariable(csound,"::UDPCOM")
        != NULL) csoundUDPServerClose(csound);



    while (csound->evtFuncChain != NULL) {
      p = (void*) csound->evtFuncChain;
      csound->evtFuncChain = ((EVT_CB_FUNC*) p)->nxt;
      csound->Free(csound,p);
    }

    /* check if we have already cleaned up */
    if (!(csound->engineStatus & CS_STATE_CLN)){
      csoundUnlockMutex(csound->API_lock);
      return 0;
    }
    /* will not clean up more than once */
    csound->engineStatus &= ~(CS_STATE_CLN);

    deactivate_all_notes(csound);

    if (csound->engineState.instrtxtp &&
        csound->engineState.instrtxtp[0] &&
        csound->engineState.instrtxtp[0]->instance &&
        csound->engineState.instrtxtp[0]->instance->actflg)
      xturnoff_now(csound, csound->engineState.instrtxtp[0]->instance);
    delete_pending_rt_events(csound);

#ifndef __EMSCRIPTEN__
    if (csound->event_insert_loop == 1) {
      csound->event_insert_loop = 0;
      csound->JoinThread(csound->event_insert_thread);
      csoundDestroyMutex(csound->init_pass_threadlock);
      csound->event_insert_thread = 0;
    }
#endif

    while (csound->freeEvtNodes != NULL) {
      p = (void*) csound->freeEvtNodes;
      csound->freeEvtNodes = ((EVTNODE*) p)->nxt;
      csound->Free(csound,p);
    }

    orcompact(csound);

    corfile_rm(csound, &csound->scstr);

    /* print stats only if musmon was actually run */
    /* NOT SURE HOW   ************************** */
    // if(csound->oparms->msglevel)
    {
      csound->ErrorMsg(csound, Str("end of score.\t\t   overall amps:"));
      corfile_rm(csound, &csound->expanded_sco);
      for (n = 0; n < csound->nchnls; n++) {
        if (csound->smaxamp[n] > csound->omaxamp[n])
          csound->omaxamp[n] = csound->smaxamp[n];
        if (csound->maxamp[n] > csound->omaxamp[n])
          csound->omaxamp[n] = csound->maxamp[n];
        STA(orngcnt)[n] += (STA(srngcnt)[n] + csound->rngcnt[n]);
      }
      for (maxp = csound->omaxamp, n = csound->nchnls; n--; )
        print_maxamp(csound, *maxp++);
      if (csound->oparms->outformat != AE_FLOAT) {
        csound->ErrorMsg(csound, Str("\n\t   overall samples out of range:"));
        for (rngp = STA(orngcnt), n = csound->nchnls; n--; )
          csound->ErrorMsg(csound, "%9d", *rngp++);
      }
      csound->ErrorMsg(csound, Str("\n%d errors in performance\n"),
                      csound->perferrcnt);
      print_benchmark_info(csound, Str("end of performance"));
      if (csound->print_version) print_csound_version(csound);
    }
    /* close line input (-L) */
    RTclose(csound);
    /* close MIDI input */
    MidiClose(csound);

    /* IV - Feb 03 2005: do not need to call rtclose from here, */
    /* as sfclosein/sfcloseout will do that. */
    if (!csound->enableHostImplementedAudioIO) {
      sfclosein(csound);
      sfcloseout(csound);
      if (UNLIKELY(!csound->oparms->sfwrite)) {
        if(csound->oparms->msglevel ||csound->oparms->odebug)
         csound->ErrorMsg(csound, Str("no sound written to disk\n"));
      }
    }
    /* close any remote.c sockets */
    if (csound->remoteGlobals) remote_Cleanup(csound);
    if (UNLIKELY(csound->oparms->ringbell))
      cs_beep(csound);

    csoundUnlockMutex(csound->API_lock);
    return dispexit(csound);    /* hold or terminate the display output     */
}

int lplay(CSOUND *csound, EVLIST *a)    /* cscore re-entry into musmon */
{
  /* if (csound->musmonGlobals == NULL) */
  /*  csound->musmonGlobals = csound->Calloc(csound, sizeof(MUSMON_GLOBALS)); */
  STA(lplayed) = 1;
  if (!STA(sectno)) {
    if(csound->oparms->msglevel ||csound->oparms->odebug)
    csound->ErrorMsg(csound, Str("SECTION %d:\n"), ++STA(sectno));
    }
  STA(ep) = &a->e[1];                  /* from 1st evlist member */
  STA(epend) = STA(ep) + a->nevents;    /*   to last              */
  while (csoundPerform(csound) == 0)  /* play list members      */
    ;                                 /* NB: empoty loop */
  return OK;
}

/* make list to turn on instrs for indef */
/* perf called from i0 for execution in playevents */

int turnon(CSOUND *csound, TURNON *p)
{
  EVTBLK  evt;
  int insno;
  memset(&evt, 0, sizeof(EVTBLK));
  evt.strarg = NULL; evt.scnt = 0;
  evt.opcod = 'i';
  evt.pcnt = 3;

  if (csound->ISSTRCOD(*p->insno)) {
    char *ss = get_arg_string(csound,*p->insno);
    insno = csound->strarg2insno(csound,ss,1);
    if (insno == NOT_AN_INSTRUMENT)
      return NOTOK;
  } else insno = *p->insno;
  evt.p[1] = (MYFLT) insno;
  evt.p[2] = *p->itime;
  evt.p[3] = FL(-1.0);
  evt.c.extra = NULL;
  return insert_score_event_at_sample(csound, &evt, csound->icurTime);
}

/* make list to turn on instrs for indef */
/* perf called from i0 for execution in playevents */

int turnon_S(CSOUND *csound, TURNON *p)
{
  EVTBLK  evt;
  int     insno;
  memset(&evt, 0, sizeof(EVTBLK));
  evt.strarg = NULL; evt.scnt = 0;
  evt.opcod = 'i';
  evt.pcnt = 3;
  insno = csound->strarg2insno(csound, ((STRINGDAT *)p->insno)->data, 1);
  if (UNLIKELY(insno == NOT_AN_INSTRUMENT))
    return NOTOK;
  evt.p[1] = (MYFLT) insno;
  evt.p[2] = *p->itime;
  evt.p[3] = FL(-1.0);
  evt.c.extra = NULL;
  return insert_score_event_at_sample(csound, &evt, csound->icurTime);
}

/* Print current amplitude values, and update section amps. */

static void print_amp_values(CSOUND *csound, int score_evt)
{
  CSOUND        *p = csound;
  MYFLT         *maxp, *smaxp;
  uint32        *maxps, *smaxps;
  int32         *rngp, *srngp;
  int           n;

  if (UNLIKELY(STA(segamps) || (p->rngflg && STA(sormsg)))) {
    if (score_evt > 0)
      p->Message(p, "B%7.3f ..%7.3f T%7.3f TT%7.3f M:",
                 p->prvbt - p->beatOffs,  p->curbt - p->beatOffs,
                 p->curp2 - p->timeOffs,  p->curp2);
    else
      p->Message(p, "  rtevent:\t   T%7.3f TT%7.3f M:",
                 p->curp2 - p->timeOffs,  p->curp2);

    for (n = p->nchnls, maxp = p->maxamp; n--; )
      print_maxamp(p, *maxp++);               /* IV - Jul 9 2002 */
    p->Message(p, "\n");
    if (UNLIKELY(p->rngflg)) {
      p->Message(p, Str("\t number of samples out of range:"));
      for (n = p->nchnls, rngp = p->rngcnt; n--; )
        p->Message(p, "%9d", *rngp++);
      p->Message(p, "\n");
    }
  }
  if (p->rngflg) {
    p->rngflg = 0;
    STA(srngflg)++;
  }
  for (n = p->nchnls,
         maxp = p->maxamp - 1, smaxp = p->smaxamp - 1,
         maxps = p->maxpos - 1, smaxps = p->smaxpos - 1,
         rngp = p->rngcnt, srngp = STA(srngcnt); n--; ) {
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

static void section_amps(CSOUND *csound, int enable_msgs)
{
  CSOUND        *p = csound;
  MYFLT         *maxp, *smaxp;
  uint32        *maxps, *smaxps;
  int32         *rngp, *srngp;
  int           n;

  if (enable_msgs) {
    if (enable_msgs == 1)
      p->Message(p, Str("end of section %d\t sect peak amps:"), STA(sectno));
    else if (enable_msgs == 2)
      p->Message(p, Str("end of lplay event list\t      peak amps:"));
    for (n = p->nchnls, maxp = p->smaxamp; n--; )
      print_maxamp(p, *maxp++);               /* IV - Jul 9 2002 */
    p->Message(p, "\n");
    if (UNLIKELY(STA(srngflg))) {
      p->Message(p, Str("\t number of samples out of range:"));
      for (n = p->nchnls, srngp = STA(srngcnt); n--; )
        p->Message(p, "%9d", *srngp++);
      p->Message(p, "\n");
    }
  }
  STA(srngflg) = 0;
  for (n = p->nchnls,
         smaxp = p->smaxamp - 1, maxp = p->omaxamp - 1,
         smaxps = p->smaxpos - 1, maxps = p->omaxpos - 1,
         srngp = STA(srngcnt), rngp = STA(orngcnt); n--; ) {
    ++maxps; ++smaxps;
    if (UNLIKELY(*++smaxp > *++maxp)) {
      *maxp = *smaxp;                 /* keep ovrl maxamps */
      *maxps = *smaxps;               /* And where */
    }
    *smaxp = FL(0.0);
    *smaxps = 0;
    *rngp++ += *srngp;                /*   and orng counts */
    *srngp++ = 0;
  }
}

static CS_NOINLINE void printScoreError(CSOUND *p, int rtEvt,
                                        const char *fmt, ...)
{
  va_list args;

  if (rtEvt)
    p->Message(p, "\t\t   T%7.3f", p->curp2 - p->timeOffs);
  else
    p->Message(p, "\t  B%7.3f", p->curbt - p->beatOffs);
  va_start(args, fmt);
  p->ErrMsgV(p, NULL, fmt, args);
  va_end(args);
  p->perferrcnt++;
}

static int process_score_event(CSOUND *csound, EVTBLK *evt, int rtEvt)
{
  EVTBLK  *saved_currevent;
  int     insno, rfd, n;

  saved_currevent = csound->currevent;
  csound->currevent = evt;
  switch (evt->opcod) {                       /* scorevt or Linevt:     */
  case 'e':           /* quit realtime */
    csound->event_insert_loop = 0;
    /* fall through */
  case 'l':
  case 's':
    while (csound->frstoff != NULL) {
      INSDS *nxt = csound->frstoff->nxtoff;
      xturnoff_now(csound, csound->frstoff);
      csound->frstoff = nxt;
    }
    csound->currevent = saved_currevent;
    return (evt->opcod == 'l' ? 3 : (evt->opcod == 's' ? 1 : 2));
  case 'q':
    if (csound->ISSTRCOD(evt->p[1]) && evt->strarg) {    /* IV - Oct 31 2002 */
      MYFLT n = named_instr_find(csound, evt->strarg);
      if (UNLIKELY((insno = (int) n) == 0)) {
        printScoreError(csound, rtEvt,
                        Str(" - note deleted. instr %s undefined"),
                        evt->strarg);
        break;
      }
      evt->p[1] = n;
      csound->ErrorMsg(csound, Str("Setting instrument %s %s\n"),
                      evt->strarg, (evt->p[3] == 0 ? Str("off") : Str("on")));
      csound->engineState.instrtxtp[insno]->muted = (int16) evt->p[3];
    }
    else {                                        /* IV - Oct 31 2002 */
      insno = abs((int) evt->p[1]);
      if (UNLIKELY((unsigned int)(insno-1) >=
                   (unsigned int) csound->engineState.maxinsno ||
                   csound->engineState.instrtxtp[insno] == NULL)) {
        printScoreError(csound, rtEvt,
                        Str(" - note deleted. instr %d(%d) undefined"),
                        insno, csound->engineState.maxinsno);
        break;
      }
      csound->ErrorMsg(csound, Str("Setting instrument %d %s\n"),
                      insno, (evt->p[3] == 0 ? Str("off") : (Str("on"))));
      csound->engineState.instrtxtp[insno]->muted = (int16) evt->p[3];
    }
    break;
  case 'i':
  case 'd':
    if (csound->ISSTRCOD(evt->p[1]) && evt->strarg) {    /* IV - Oct 31 2002 */
      MYFLT n = named_instr_find(csound, evt->strarg);
      if (UNLIKELY((insno = (int)n) == 0)) {
        printScoreError(csound, rtEvt,
                        Str(" - note deleted. instr %s undefined"),
                        evt->strarg);
        break;
      }
      evt->p[1] = n;
      if (insno<0) {
        evt->p[1] = insno; insno = -insno;
      }
      else if (evt->opcod=='d') evt->p[1]=-insno;
      if ((rfd = getRemoteInsRfd(csound, insno))) {
        /* RM: if this note labeled as remote */
        if (rfd == GLOBAL_REMOT)
          insGlobevt(csound, evt);  /* RM: do a global send and allow local */
        else {
          insSendevt(csound, evt, rfd);/* RM: or send to single remote Csound */
          break;                       /* RM: and quit */
        }
      }
      evt->p[1] = (MYFLT) insno;
      if (csound->oparms->Beatmode && !rtEvt && evt->p3orig > FL(0.0))
        evt->p[3] = evt->p3orig * (MYFLT) csound->ibeatTime/csound->esr;
      /* else alloc, init, activate */
      if (UNLIKELY((n = insert(csound, insno, evt)))) {
        printScoreError(csound, rtEvt,
                        Str(" - note deleted.  i%d (%s) had %d init errors"),
                        insno, evt->strarg, n);
      }
    }
    else {                                        /* IV - Oct 31 2002 */
      insno = abs((int) evt->p[1]);
      if (UNLIKELY((unsigned int)(insno-1) >=
                   (unsigned int)csound->engineState.maxinsno ||
                   csound->engineState.instrtxtp[insno] == NULL)) {
        printScoreError(csound, rtEvt,
                        Str(" - note deleted. instr %d(%d) undefined"),
                        insno, csound->engineState.maxinsno);
        break;
      }
      if ((rfd = getRemoteInsRfd(csound, insno))) {
        /* RM: if this note labeled as remote  */
        if (rfd == GLOBAL_REMOT)
          insGlobevt(csound, evt);    /* RM: do a global send and allow local */
        else {
          insSendevt(csound, evt, rfd);/* RM: or send to single remote Csound */
          break;                      /* RM: and quit              */
        }
      }
      if (evt->p[1] < FL(0.0))         /* if p1 neg,             */
        infoff(csound, -evt->p[1]);    /*  turnoff any infin cpy */
      else {
        if (csound->oparms->Beatmode && !rtEvt && evt->p3orig > FL(0.0))
          evt->p[3] = evt->p3orig * (MYFLT) csound->ibeatTime/csound->esr;
        if (UNLIKELY((n = insert(csound, insno, evt)))) {
          /* else alloc, init, activate */
          printScoreError(csound, rtEvt,
                          Str(" - note deleted.  i%d had %d init errors"),
                          insno, n);
        }
      }
    }
    break;
  case 'f':                   /* f event: */
    {
      FUNC  *dummyftp;
      csound->hfgens(csound, &dummyftp, evt, 0); /* construct locally */
      if (getRemoteInsRfdCount(csound))
        insGlobevt(csound, evt); /* RM: & optionally send to all remotes      */
    }
    break;
  case 'a':
    {
      int64_t kCnt;
      kCnt = (int64_t) ((double) csound->ekr * (double) evt->p[3] + 0.5);
      if (kCnt > csound->advanceCnt) {
        csound->advanceCnt = kCnt;
        csound->ErrorMsg(csound,
                        Str("time advanced %5.3f beats by score request\n"),
                        evt->p3orig);
      }
    }
    break;
  }
  csound->currevent = saved_currevent;
  return 0;
}

/* RM: this now broken out for access from process_rt_event & sensevents -- bv  */
static void process_midi_event(CSOUND *csound, MEVENT *mep, MCHNBLK *chn)
{
  int n, insno = chn->insno;
  if (mep->type == NOTEON_TYPE && mep->dat2) {      /* midi note ON: */
    if (UNLIKELY((n = MIDIinsert(csound, insno, chn, mep)))) {
      /* alloc,init,activ */
      csound->ErrorMsg(csound,
                      Str("\t\t   T%7.3f - note deleted. "), csound->curp2);
      {
        char *name = csound->engineState.instrtxtp[insno]->insname;
        if (name)
          csound->ErrorMsg(csound, Str("instr %s had %d init errors\n"),
                          name, n);
        else
          csound->ErrorMsg(csound, Str("instr %d had %d init errors\n"),
                          insno, n);
      }
      csound->perferrcnt++;
    }
  }
  else {                                          /* else midi note OFF:    */

    INSDS *ip = chn->kinsptr[mep->dat1];
    if (ip == NULL)                               /*  if already off, done  */
      csound->Mxtroffs++;
    else if (chn->sustaining) {                   /*  if sustain pedal on   */
      while (ip != NULL && ip->m_sust)
        ip = ip->nxtolap;
      if (ip != NULL) {
        ip->m_sust = 1;                           /*    let the note ring   */
        chn->ksuscnt++;
      } else csound->Mxtroffs++;
    }
    else xturnoff(csound, ip);                    /*  else some kind of off */
  }
}

static int process_rt_event(CSOUND *csound, int sensType)
{
  EVTBLK  *evt;
  int     retval, insno, rfd;

  retval = 0;
  if (csound->curp2 * csound->esr < (double)csound->icurTime) {
    csound->curp2 = (double)csound->icurTime/csound->esr;
    //if(sensType != 2)
      print_amp_values(csound, 0);
  }
  if (sensType == 4) {                  /* RM: Realtime orc event   */
    EVTNODE *e = csound->OrcTrigEvts;
    /* RM: Events are sorted on insertion, so just check the first */
    evt = &(e->evt);
    insno = MYFLT2LONG(evt->p[1]);
    if ((rfd = getRemoteInsRfd(csound, insno))) {
      if (rfd == GLOBAL_REMOT)
        insGlobevt(csound, evt);       /* RM: do a global send and allow local */
      else
        insSendevt(csound, evt, rfd);  /* RM: or send to single remote Csound */
      return 0;
    }
    /* pop from the list */
    csound->OrcTrigEvts = e->nxt;
    retval = process_score_event(csound, evt, 1);
    if (evt->strarg != NULL) {
      csound->Free(csound, evt->strarg);
      evt->strarg = NULL;
    }
    /* push back to free alloc stack so it can be reused later */
    e->nxt = csound->freeEvtNodes;
    csound->freeEvtNodes = e;
  }
  else if (sensType == 2) {                      /* Midievent:    */
    MEVENT *mep;
    MCHNBLK *chn;
    /* realtime or Midifile  */
    mep = csound->midiGlobals->Midevtblk;
    chn = csound->m_chnbp[mep->chan];
    if ((rfd = getRemoteChnRfd(csound, mep->chan+1))) { /* RM: USE CHAN + 1 */
      if (rfd == GLOBAL_REMOT)
        MIDIGlobevt(csound, mep);
      else MIDIsendevt(csound, mep, rfd);
      return 0;
    }
    else  /* RM: this part is broken out  -- bv  */
      process_midi_event(csound, mep, chn);
  }
  return retval;
}

#define RNDINT64(x) ((int64_t) ((double) (x) + ((double) (x) < 0.0 ? -0.5 : 0.5)))

extern  int     sensMidi(CSOUND *);

/* sense events for one k-period            */
/* return value is one of the following:    */
/*   0: continue performance                */
/*   1: terminate (e.g. end of MIDI file)   */
/*   2: normal end of score                 */
int sensevents(CSOUND *csound)
{
  EVTBLK  *e;
  OPARMS  *O = csound->oparms;
  int     retval =  0, sensType;
  int     conn, *sinp, end_check=1;

  csdebug_data_t *data = (csdebug_data_t *) csound->csdebug_data;
  if (UNLIKELY(data && data->status == CSDEBUG_STATUS_STOPPED)) {
    return 0; /* don't process events if we're in debug mode and stopped */
  }
  if (UNLIKELY(csound->MTrkend && O->termifend)) {   /* end of MIDI file:  */
    deactivate_all_notes(csound);
    csound->ErrorMsg(csound, Str("terminating.\n"));
    return 1;                         /* abort with perf incomplete */
  }
  /* if turnoffs pending, remove any expired instrs */
  RT_SPIN_TRYLOCK
  if (UNLIKELY(csound->frstoff != NULL)) {
    double  tval;
    /* the following comparisons must match those in schedofftim() */
    if (O->Beatmode) {
      tval = csound->curBeat + (0.505 * csound->curBeat_inc);
      if (csound->frstoff->offbet <= tval) beatexpire(csound, tval);
    }
    else {
      tval = ((double)csound->icurTime + csound->ksmps * 0.505)/csound->esr;
      if (csound->frstoff->offtim <= tval)
        timexpire(csound, tval);
    }
  }
  RT_SPIN_UNLOCK

  e = &(csound->evt);
  if (--(csound->cyclesRemaining) <= 0) { /* if done performing score segment: */
    if (!csound->cyclesRemaining) {
      csound->prvbt = csound->curbt;      /* update beats and times */
      csound->curbt = csound->nxtbt;
      csound->curp2 = csound->nxtim;
      print_amp_values(csound, 1);        /* print amplitudes for this segment */
    }
    else                                  /* this should only happen at */
      csound->cyclesRemaining = 0;        /* beginning of performance   */
  }

 retest:
  /* in daemon mode, we will ignore the end of
     the score, but allow for a realtime event
     to stop Csound */
    while (csound->cyclesRemaining <= 0 &&
           (e->opcod != 'e' || !csound->oparms->daemon)){
      /* read each score event:     */
    if (e->opcod != '\0') {
      /* if there is a pending score event, handle it now */
      switch (e->opcod) {
      case 'e':                     /* end of score, */
      case 'l':                     /* lplay list,   */
      case 's':                     /* or section:   */
        if (csound->frstoff != NULL) {    /* if still have notes
                                             with finite length, wait
                                             until all are turned off */
          RT_SPIN_TRYLOCK
          csound->nxtim = csound->frstoff->offtim;
          csound->nxtbt = csound->frstoff->offbet;
          RT_SPIN_UNLOCK
          break;
        }
        /* end of: 1: section, 2: score, 3: lplay list */
        retval = (e->opcod == 'l' ? 3 : (e->opcod == 's' ? 1 : 2));
        if(csound->oparms->realtime && end_check == 1) {
          csoundSleep(5); // wait for 5ms for any first events to go through
          end_check = 0;  // reset first time check
          goto retest;    // loop back
        }
        goto scode;
      default:                            /* q, i, f, a:              */
        process_score_event(csound, e, 0);/*   handle event now       */
        e->opcod = '\0';                  /*   and get next one       */
        continue;
      }
    }
    else {
      /* else read next score event */
      if (UNLIKELY(O->usingcscore)) {       /*    get next lplay event  */
        /* FIXME: this may be non-portable */
        if (STA(ep) < STA(epend))           /* nxt event    */
          memcpy((void*) e, (void*) &((*STA(ep)++)->strarg), sizeof(EVTBLK));
        else                                /* else lcode   */
          memcpy((void*) e, (void*) &(STA(lsect)->strarg), sizeof(EVTBLK));
      } else
        if (!(rdscor(csound, e))){
          /* or rd nxt evt from scstr */
          e->opcod = 'e';
        }
      csound->currevent = e;

      switch (e->opcod) {
      case 'w':
        if (!O->Beatmode)                   /* Not beatmode: read 'w' */
          settempo(csound, (double)e->p2orig); /* to init the tempo   */
        continue;                           /*   for this section     */
      case 'q':
      case 'i':
      case 'd':
      case 'f':
      case 'a':
        csound->nxtim = (double) e->p[2] + csound->timeOffs;
        csound->nxtbt = (double) e->p2orig + csound->beatOffs;
        if (e->opcod=='i'||e->opcod=='d')
          if (UNLIKELY(csound->oparms->odebug))
            csound->ErrorMsg(csound, "new event: %16.13lf %16.13lf\n",
                            csound->nxtim, csound->nxtbt);
        break;
      case 'e':
      case 'l':
      case 's':
        continue;
      default:
        csound->ErrorMsg(csound,
                        Str("error in score.  illegal opcode %c (ASCII %d)\n"),
                        e->opcod, e->opcod);
        csound->perferrcnt++;
        continue;
      }
    }
    /* calculate the number of k-periods remaining until next event */
    if (!O->sampleAccurate) {
      if (O->Beatmode)
        csound->cyclesRemaining =
          RNDINT64((csound->nxtbt - csound->curBeat) / csound->curBeat_inc);
      else {
        csound->cyclesRemaining =
          RNDINT64((csound->nxtim*csound->esr - csound->icurTime)/csound->ksmps);
        csound->nxtim =
          (csound->cyclesRemaining*csound->ksmps+csound->icurTime)/csound->esr;
      }
    }
    else {
      /* VL 30-11-2012
         new code for sample-accurate timing needs to truncate cyclesRemaining
      */
      if (O->Beatmode)
        csound->cyclesRemaining = (int64_t)
          ((csound->nxtbt - csound->curBeat) / csound->curBeat_inc);
      else {
        csound->cyclesRemaining = (int64_t)
          FLOOR((csound->nxtim*csound->esr -
                 csound->icurTime+csound->onedsr*0.5) / csound->ksmps);
        csound->nxtim =
          (csound->cyclesRemaining*csound->ksmps+csound->icurTime)/csound->esr;
      }
    }
  }

  /* handle any real time events now: */
  /* FIXME: the initialisation pass of real time */
  /*   events is not sorted by instrument number */
  /*   (although it never was sorted anyway...)  */
  if (UNLIKELY(O->RTevents || getRemoteSocksIn(csound))) {
    int nrecvd;
    /* run all registered callback functions */
    if (csound->evtFuncChain != NULL && !csound->advanceCnt) {
      EVT_CB_FUNC *fp = (EVT_CB_FUNC*) csound->evtFuncChain;
      do {
        fp->func(csound, fp->userData);
        fp = fp->nxt;
      } while (fp != NULL);
    }

    /* check for pending real time events */
    while (csound->OrcTrigEvts != NULL &&
           csound->OrcTrigEvts->start_kcnt <=
           (uint32) csound->global_kcounter) {

      if ((retval = process_rt_event(csound, 4)) != 0){
        goto scode;
      }
    }
    /* RM */
    if ((sinp = getRemoteSocksIn(csound))) {
      while ((conn = *sinp++)) {
        while ((nrecvd = SVrecv(csound, conn,
                                (void*)&(csound->SVrecvbuf),
                                sizeof(REMOT_BUF) )) > 0) {
          int lentot = 0;
          do {
            REMOT_BUF *bp = (REMOT_BUF*)((char*)(&(csound->SVrecvbuf))+lentot);

            if (bp->type == SCOR_EVT) {
              EVTBLK *evt = (EVTBLK*)bp->data;
              evt->p[2] = (double)csound->icurTime/csound->esr;
              if ((retval = process_score_event(csound, evt, 1)) != 0) {
                e->opcod = evt->opcod;        /* pass any s, e, or l */

                goto scode;
              }
            }
            else if (bp->type == MIDI_EVT) {
              MEVENT *mep = (MEVENT *)bp->data;
              MCHNBLK *chn = csound->m_chnbp[mep->chan];
              process_midi_event(csound, mep, chn);
            }
            else if (bp->type == MIDI_MSG) {
              MEVENT *mep = (MEVENT *)bp->data;
              if (UNLIKELY(mep->type == 0xFF && mep->dat1 == 0x2F)) {
                csound->MTrkend = 1;                     /* catch a Trkend    */
                csound->ErrorMsg(csound, "SERVER%c: ", remoteID(csound));
                csound->ErrorMsg(csound, "caught a Trkend\n");
                /*csoundCleanup(csound);
                  exit(0);*/
                return 2;  /* end of performance */
              }
              else m_chanmsg(csound, mep);               /* or a chan msg     */
            }
            lentot+=bp->len;
          } while (lentot < nrecvd);
        }
      }
    }

    /* MIDI note messages */
    if (O->Midiin || O->FMidiin)
      while ((sensType = sensMidi(csound)) != 0)
        if ((retval = process_rt_event(csound, sensType)) != 0) {
          goto scode;
        }
  }
  /* no score event at this time, return to continue performance */

  return 0;
 scode:
  /* end of section (retval == 1), score (retval == 2), */
  /* or lplay list (retval == 3) */
  if (getRemoteInsRfdCount(csound))
    insGlobevt(csound, e);/* RM: send s,e, or l to any remotes */
  e->opcod = '\0';
  if (retval == 3) {
    section_amps(csound, 2);
    return 1;
  }
  /* for s, or e after s */
  if (retval == 1 || (retval == 2 && STA(sectno) > 1)) {
    delete_pending_rt_events(csound);
    if (O->Beatmode)
      csound->curbt = csound->curBeat;
    csound->curp2 = csound->nxtim =
      csound->timeOffs = csound->icurTime/csound->esr;
    csound->prvbt = csound->nxtbt = csound->beatOffs = csound->curbt;
    section_amps(csound, 1);
  }
  else{
    section_amps(csound, 0);
  }
  if (retval == 1) {                        /* if s code,        */
    RT_SPIN_TRYLOCK
    orcompact(csound);                      /*   rtn inactiv spc */
    if (csound->actanchor.nxtact == NULL)   /*   if no indef ins */
      rlsmemfiles(csound);                  /*    purge memfiles */
    csound->ErrorMsg(csound, Str("SECTION %d:\n"), ++STA(sectno));
    RT_SPIN_UNLOCK
    goto retest;                            /*   & back for more */
  }

  return retval;                   /* done with entire score */
}

static inline uint64_t time2kcnt(CSOUND *csound, double tval)
{
  if (tval > 0.0) {
    tval *= (double) csound->ekr;
#ifdef HAVE_C99
    return (uint64_t) llrint(tval);
#else
    return (uint64_t) (tval + 0.5);
#endif
  }
  return 0UL;
}


/* Schedule new score event to be played. 'time_ofs' is the amount of */
/* time in seconds to add to evt->p[2] to get the actual start time   */
/* of the event (measured from the beginning of performance, and not  */
/* section) in seconds.                                               */
/* Required parameters in 'evt':                                      */
/*   char   *strarg   string argument of event (NULL if none)         */
/*   char   opcod     event opcode (a, e, f, i, l, q, s)              */
/*   int16  pcnt      number of p-fields (>=3 for q, i, a; >=4 for f) */
/*   MYFLT  p[]       array of p-fields, p[1]..p[pcnt] should be set  */
/*  p2orig and p3orig are calculated from p[2] and p[3].              */
/* The contents of 'evt', including the string argument, need not be  */
/* preserved after calling this function, as a copy of the event is   */
/* made.                                                              */
/* Return value is zero on success.                                   */

int insert_score_event_at_sample(CSOUND *csound, EVTBLK *evt, int64_t time_ofs)
{
  double        start_time;
  EVTNODE       *e, *prv;
  CSOUND        *st = csound;
  MYFLT         *p;
  uint32        start_kcnt;
  int           i, retval;

  retval = -1;
  /* make a copy of the event... */
  if (csound->freeEvtNodes != NULL) {             /* pop alloc from stack */
    e = csound->freeEvtNodes;                     /*   if available       */
    csound->freeEvtNodes = e->nxt;
  }
  else {
    e = (EVTNODE*) csound->Calloc(csound, sizeof(EVTNODE)); /* or alloc new one */
    if (UNLIKELY(e == NULL))
      return CSOUND_MEMORY;
  }
  if (evt->strarg != NULL) {  /* copy string argument if present */
    /* NEED TO COPY WHOLE STRING STRUCTURE */
    int n = evt->scnt;
    char *p = evt->strarg;
    while (n--) { p += strlen(p)+1; };
    e->evt.strarg = (char*) csound->Malloc(csound, (size_t) (p-evt->strarg)+1);
    if (UNLIKELY(e->evt.strarg == NULL)) {
      csound->Free(csound, e);
      return CSOUND_MEMORY;
    }
    memcpy(e->evt.strarg, evt->strarg, p-evt->strarg+1 );
    e->evt.scnt = evt->scnt;
  }
  e->evt.pinstance = evt->pinstance;
  e->evt.opcod = evt->opcod;
  e->evt.pcnt = evt->pcnt;
  p = &(e->evt.p[0]);
  i = 0;
  while (++i <= evt->pcnt)    /* copy p-field list */
    p[i] = evt->p[i];
  /* ...and use the copy from now on */
  evt = &(e->evt);

  /* check for required p-fields */
  switch (evt->opcod) {
  case 'f':
    if (UNLIKELY((evt->pcnt < 4) && (p[1]>0)))
      goto pfld_err;
    goto cont;
  case 'i':
  case 'q':
  case 'a':
    if (UNLIKELY(evt->pcnt < 3))
      goto pfld_err;
    /* fall through */
  case 'd':
  cont:
    /* calculate actual start time in seconds and k-periods */
    start_time = (double) p[2] + (double)time_ofs/csound->esr;
    start_kcnt = time2kcnt(csound, start_time);
    /* correct p2 value for section offset */
    p[2] = (MYFLT) (start_time - st->timeOffs);
    if (p[2] < FL(0.0))
      p[2] = FL(0.0);
    /* start beat: this is possibly wrong */
    evt->p2orig = (MYFLT) (((start_time - st->icurTime/st->esr) /
                            st->ibeatTime)
                           + (st->curBeat - st->beatOffs));
    if (evt->p2orig < FL(0.0))
      evt->p2orig = FL(0.0);
    evt->p3orig = p[3];
    break;
  default:
    start_kcnt = 0UL;   /* compiler only */
  }

  switch (evt->opcod) {
  case 'i':                         /* note event */
  case 'd':
    /* calculate the length in beats */
    if (evt->p3orig > FL(0.0))
      evt->p3orig = (MYFLT) ((double) evt->p3orig / st->ibeatTime);
    /* fall through */
  case 'q':                         /* mute instrument */
    /* check for a valid instrument number or name */
    if (evt->opcod=='d') {
      if (evt->strarg != NULL && csound->ISSTRCOD(p[1])) {
        i = (int) named_instr_find(csound, evt->strarg);
        //printf("d opcode %s -> %d\n", evt->strarg, i);
        p[1] = -i;
      }
      else {
        i = (int) fabs((double) p[1]);
        p[1] = -i;
      }
    }
    else if (evt->strarg != NULL && csound->ISSTRCOD(p[1])) {
      MYFLT n = named_instr_find(csound, evt->strarg);
      p[1] = n;
      i =(int) n;
      if (n<0) {i= -i;}
    }
    else
      i = (int) fabs((double) p[1]);
    if (UNLIKELY((unsigned int) (i - 1) >=
                 (unsigned int) csound->engineState.maxinsno ||
                 csound->engineState.instrtxtp[i] == NULL)) {
      if (i > INT32_MAX-10)
        csoundErrorMsg(csound, "%s",
                      Str("insert_score_event(): invalid named instrument\n"));
      else
        csoundErrorMsg(csound, Str("insert_score_event(): invalid instrument "
                                  "number or name %d\n" ), i);
      goto err_return;
    }
    break;
  case 'a':                         /* advance score time */
    /* calculate the length in beats */
    evt->p3orig = (MYFLT) ((double) evt->p3orig *csound->esr/ st->ibeatTime);
    /* fall through */
  case 'f':                         /* function table */
    break;
  case 'e':                         /* end of score, */
  case 'l':                         /*   lplay list, */
  case 's':                         /*   section:    */
    start_time = (double)time_ofs/csound->esr;
    if (evt->pcnt >= 2)
      start_time += (double) p[2];
    evt->pcnt = 0;
    start_kcnt = time2kcnt(csound, start_time);
    break;
  default:
    csoundErrorMsg(csound, Str("insert_score_event(): unknown opcode: %c\n"),
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
  csound->oparms->RTevents = 1;
  return 0;

 pfld_err:
  csoundErrorMsg(csound, Str("insert_score_event(): insufficient p-fields\n"));
 err_return:
  /* clean up */
  if (e->evt.strarg != NULL)
    csound->Free(csound, e->evt.strarg);
  e->evt.strarg = NULL;
  e->nxt = csound->freeEvtNodes;
  csound->freeEvtNodes = e;
  return retval;
}

int insert_score_event(CSOUND *csound, EVTBLK *evt, double time_ofs)
{
  return insert_score_event_at_sample(csound, evt, time_ofs*csound->esr);
}

/* called by csoundRewindScore() to reset performance to time zero */

void musmon_rewind_score(CSOUND *csound)
{
  /* deactivate all currently playing notes */
  deactivate_all_notes(csound);
  /* flush any pending real time events */
  delete_pending_rt_events(csound);

  if (csound->global_kcounter != 0L) {
    /* reset score time */
    csound->global_kcounter = csound->kcounter = 0L;
    csound->nxtbt = csound->curbt = csound->prvbt = 0.0;
    csound->nxtim = csound->curp2 = 0.0;
    csound->beatOffs = csound->timeOffs = 0.0;
    csound->curBeat  = 0.0;
    csound->icurTime = 0L;
    csound->cyclesRemaining = 0;
    csound->evt.strarg = NULL;
    csound->evt.scnt = 0;
    csound->evt.opcod  = '\0';
    /* reset tempo */
    if (csound->oparms->Beatmode)
      settempo(csound, csound->oparms->cmdTempo);
    else
      settempo(csound, 60.0);
    /* update section/overall amplitudes, reset to section 1 */
    section_amps(csound, 1);
    STA(sectno) = 1;
    csound->ErrorMsg(csound, Str("SECTION %d:\n"), STA(sectno));
  }

  /* apply score offset if non-zero */
  csound->advanceCnt = 0;
  if (csound->csoundScoreOffsetSeconds_ > FL(0.0))
    csoundSetScoreOffsetSeconds(csound, csound->csoundScoreOffsetSeconds_);
  if (csound->scstr)
    corfile_rewind(csound->scstr);
  else csound->Warning(csound, Str("cannot rewind score: no score in memory\n"));
}

/**
 * Register a function to be called once in every control period
 * by sensevents(). Any number of functions may be registered,
 * and will be called in the order of registration.
 * The callback function takes two arguments: the Csound instance
 * pointer, and the userData pointer as passed to this function.
 * Returns zero on success.
 */
PUBLIC int csoundRegisterSenseEventCallback(CSOUND *csound,
                                            void (*func)(CSOUND *, void *),
                                            void *userData)
{
  EVT_CB_FUNC *fp = (EVT_CB_FUNC*) csound->evtFuncChain;

  if (fp == NULL) {
    fp = (EVT_CB_FUNC*) csound->Calloc(csound, sizeof(EVT_CB_FUNC));
    csound->evtFuncChain = (void*) fp;
  }
  else {
    while (fp->nxt != NULL)
      fp = fp->nxt;
    fp->nxt = (EVT_CB_FUNC*) csound->Calloc(csound, sizeof(EVT_CB_FUNC));
    fp = fp->nxt;
  }
  if (UNLIKELY(fp == NULL))
    return CSOUND_MEMORY;
  fp->func = func;
  fp->userData = userData;
  fp->nxt = NULL;
  csound->oparms->RTevents = 1;

  return 0;
}
