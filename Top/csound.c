/*
 * C S O U N D
 *
 * An auto-extensible system for making music on computers
 * by means of software alone.
 *
 * Copyright (C) 2001-2006 Michael Gogins, Matt Ingalls, John D. Ramsdell,
 *                         John P. ffitch, Istvan Varga
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * 29 May 2002 - ma++ merge with CsoundLib.
 * 30 May 2002 - mkg add csound "this" pointer argument back into merge.
 * 27 Jun 2002 - mkg complete Linux dl code and Makefile
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_UNISTD_H) || defined (__unix) || defined(__unix__)
#include <unistd.h>
#endif
#include "csoundCore.h"
#include "csmodule.h"
#include "csGblMtx.h"
#include <stdarg.h>
#include <signal.h>
#ifndef mac_classic
#include <pthread.h>
#endif
#include <time.h>
#include <ctype.h>
#include <limits.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef WIN32
# include <windows.h>
# include <winsock2.h>
#endif
#include <math.h>
#include "oload.h"
#include "fgens.h"
#include "namedins.h"
#include "pvfileio.h"
#include "fftlib.h"
#ifdef ENABLE_NEW_PARSER
#include "csound_orc.h"
#endif
#ifdef PARCS
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#include "cs_par_dispatch.h"

  /* **** MAJOR PROBLEM: PTHREAD_SPINLOCK_INITIALIZER is not defined in Linux */

#ifdef linux
  #define PTHREAD_SPINLOCK_INITIALIZER 0
#endif
#endif /* PARCS */

#if defined(USE_OPENMP)
#include <omp.h>
#endif /* USE_OPENMP */

  MYFLT csoundPow2(CSOUND *csound, MYFLT a);
  extern void MakeAscii(CSOUND *, WINDAT *, const char *);
  extern void DrawAscii(CSOUND *, WINDAT *);
  extern void KillAscii(CSOUND *, WINDAT *);
  extern int csoundInitStaticModules(CSOUND *);

  static void SetInternalYieldCallback(CSOUND *, int (*yieldCallback)(CSOUND *));
  static int  playopen_dummy(CSOUND *, const csRtAudioParams *parm);
  static void rtplay_dummy(CSOUND *, const MYFLT *outBuf, int nbytes);
  static int  recopen_dummy(CSOUND *, const csRtAudioParams *parm);
  static int  rtrecord_dummy(CSOUND *, MYFLT *inBuf, int nbytes);
  static void rtclose_dummy(CSOUND *);
  static void csoundDefaultMessageCallback(CSOUND *, int, const char *, va_list);
  static void defaultCsoundMakeXYin(CSOUND *, XYINDAT *, MYFLT, MYFLT);
  static void defaultCsoundReadKillXYin(CSOUND *, XYINDAT *);
  static int  defaultCsoundExitGraph(CSOUND *);
  static int  defaultCsoundYield(CSOUND *);
  static int  csoundDoCallback_(CSOUND *, void *, unsigned int);

  extern void close_all_files(CSOUND *);

  static const CSOUND cenviron_ = {
    /* ----------------- interface functions (322 total) ----------------- */
    csoundGetVersion,
    csoundGetAPIVersion,
    csoundGetHostData,
    csoundSetHostData,
    csoundCreate,
    csoundCompile,
    csoundPerform,
    csoundPerformKsmps,
    csoundPerformBuffer,
    csoundCleanup,
    csoundReset,
    csoundDestroy,
    csoundGetSr,
    csoundGetKr,
    csoundGetKsmps,
    csoundGetNchnls,
    csoundGetSampleFormat,
    csoundGetSampleSize,
    csoundGetInputBufferSize,
    csoundGetOutputBufferSize,
    csoundGetInputBuffer,
    csoundGetOutputBuffer,
    csoundGetSpin,
    csoundGetSpout,
    csoundGetScoreTime,
    csoundSetMakeXYinCallback,
    csoundSetReadXYinCallback,
    csoundSetKillXYinCallback,
    csoundIsScorePending,
    csoundSetScorePending,
    csoundGetScoreOffsetSeconds,
    csoundSetScoreOffsetSeconds,
    csoundRewindScore,
    csoundMessage,
    csoundMessageS,
    csoundMessageV,
    csoundDeleteUtilityList,
    csoundDeleteChannelList,
    csoundSetMessageCallback,
    csoundDeleteCfgVarList,
    csoundGetMessageLevel,
    csoundSetMessageLevel,
    csoundInputMessage,
    csoundKeyPress,
    csoundSetInputValueCallback,
    csoundSetOutputValueCallback,
    csoundScoreEvent,
    csoundSetExternalMidiInOpenCallback,
    csoundSetExternalMidiReadCallback,
    csoundSetExternalMidiInCloseCallback,
    csoundSetExternalMidiOutOpenCallback,
    csoundSetExternalMidiWriteCallback,
    csoundSetExternalMidiOutCloseCallback,
    csoundSetExternalMidiErrorStringCallback,
    csoundSetIsGraphable,
    csoundSetMakeGraphCallback,
    csoundSetDrawGraphCallback,
    csoundSetKillGraphCallback,
    csoundSetExitGraphCallback,
    csoundNewOpcodeList,
    csoundDisposeOpcodeList,
    csoundAppendOpcode,
    csoundAppendOpcodes,
    csoundOpenLibrary,
    csoundCloseLibrary,
    csoundGetLibrarySymbol,
    csoundYield,
    csoundSetYieldCallback,
    csoundGetEnv,
    csoundFindInputFile,
    csoundFindOutputFile,
    csoundSetPlayopenCallback,
    csoundSetRtplayCallback,
    csoundSetRecopenCallback,
    csoundSetRtrecordCallback,
    csoundSetRtcloseCallback,
    csoundAuxAlloc,
    mmalloc,
    mcalloc,
    mrealloc,
    mfree,
    dispset,
    display,
    dispexit,
    intpow,
    ldmemfile,
    strarg2insno,
    strarg2name,
    hfgens,
    insert_score_event,
    csoundFTAlloc,
    csoundFTDelete,
    csoundFTFind,
    csoundFTFindP,
    csoundFTnp2Find,
    csoundGetTable,
    csoundLoadSoundFile,
    getstrformat,
    sfsampsize,
    type2string,
    SAsndgetset,
    sndgetset,
    getsndin,
    rewriteheader,
    csoundRand31,
    fdrecord,
    fdclose,
    csoundSetDebug,
    csoundGetDebug,
    csoundTableLength,
    csoundTableGet,
    csoundTableSet,
    csoundCreateThread,
    csoundJoinThread,
    csoundCreateThreadLock,
    csoundDestroyThreadLock,
    csoundWaitThreadLock,
    csoundNotifyThreadLock,
    csoundWaitThreadLockNoTimeout,
    csoundSleep,
    csoundInitTimerStruct,
    csoundGetRealTime,
    csoundGetCPUTime,
    csoundGetRandomSeedFromTime,
    csoundSeedRandMT,
    csoundRandMT,
    csoundPerformKsmpsAbsolute,
    csoundLocalizeString,
    csoundCreateGlobalVariable,
    csoundQueryGlobalVariable,
    csoundQueryGlobalVariableNoCheck,
    csoundDestroyGlobalVariable,
    csoundCreateConfigurationVariable,
    csoundSetConfigurationVariable,
    csoundParseConfigurationVariable,
    csoundQueryConfigurationVariable,
    csoundListConfigurationVariables,
    csoundDeleteConfigurationVariable,
    csoundCfgErrorCodeToString,
    csoundGetSizeOfMYFLT,
    csoundGetRtRecordUserData,
    csoundGetRtPlayUserData,
    csoundGetInverseComplexFFTScale,
    csoundGetInverseRealFFTScale,
    csoundComplexFFT,
    csoundInverseComplexFFT,
    csoundRealFFT,
    csoundInverseRealFFT,
    csoundRealFFTMult,
    csoundRealFFTnp2,
    csoundInverseRealFFTnp2,
    csoundAddUtility,
    csoundRunUtility,
    csoundListUtilities,
    csoundSetUtilityDescription,
    csoundGetUtilityDescription,
    csoundRegisterSenseEventCallback,
    csoundRegisterDeinitCallback,
    csoundRegisterResetCallback,
    csoundCreateFileHandle,
    csoundFileOpen,
    csoundGetFileName,
    csoundFileClose,
    pvoc_createfile,
    (int (*)(CSOUND *, const char *, void *, void *)) pvoc_openfile,
    pvoc_closefile,
    pvoc_putframes,
    pvoc_getframes,
    pvoc_framecount,
    pvoc_fseek,
    pvoc_errorstr,
    PVOCEX_LoadFile,
    csoundGetOpcodeName,
    csoundGetInputArgCnt,
    csoundGetInputArgAMask,
    csoundGetInputArgSMask,
    csoundGetInputArgName,
    csoundGetOutputArgCnt,
    csoundGetOutputArgAMask,
    csoundGetOutputArgSMask,
    csoundGetOutputArgName,
    csoundSetReleaseLength,
    csoundSetReleaseLengthSeconds,
    csoundGetMidiChannelNumber,
    csoundGetMidiChannel,
    csoundGetMidiNoteNumber,
    csoundGetMidiVelocity,
    csoundGetReleaseFlag,
    csoundGetOffTime,
    csoundGetPFields,
    csoundGetInstrumentNumber,
    csoundDie,
    csoundInitError,
    csoundPerfError,
    csoundWarning,
    csoundDebugMsg,
    csoundLongJmp,
    csoundErrorMsg,
    csoundErrMsgV,
    csoundGetChannelPtr,
    csoundListChannels,
    csoundSetControlChannelParams,
    csoundGetControlChannelParams,
    csoundChanIKSet,
    csoundChanOKGet,
    csoundChanIASet,
    csoundChanOAGet,
    dispinit,
    csoundCreateMutex,
    csoundLockMutexNoWait,
    csoundLockMutex,
    csoundUnlockMutex,
    csoundDestroyMutex,
    csoundRunCommand,
    csoundGetCurrentThreadId,
    csoundSetChannelIOCallback,
    csoundSetCallback,
    csoundRemoveCallback,
    csoundPvsinSet,
    csoundPvsoutGet,
    SetInternalYieldCallback,
    csoundCreateBarrier,
    csoundDestroyBarrier,
    csoundWaitBarrier,
    csoundFileOpenWithType,
    type2csfiletype,
    ldmemfile2,
    csoundNotifyFileOpened,
    sftype2csfiletype,
    insert_score_event_at_sample,
    csoundGetChannelLock,
    ldmemfile2withCB,
    csoundAddSpinSample,
    csoundGetSpoutSample,
    csoundChanIKSetValue,
    csoundChanOKGetValue,
    csoundChanIASetSample,
    csoundChanOAGetSample,
    csoundStop,
    csoundGetNamedGens,
    csoundPow2,
    /* NULL, */
    {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL, NULL, NULL, NULL, NULL
    },
    0,                          /* dither_output */
    NULL,  /*  flgraphsGlobals */
    NULL, NULL,             /* Delayed messages */
    /* ----------------------- public data fields ----------------------- */
    (OPDS*) NULL,   /*  ids                 */
    (OPDS*) NULL,   /*  pds                 */
    DFLT_KSMPS,     /*  ksmps               */
    DFLT_KSMPS,     /*  global_ksmps        */
    DFLT_NCHNLS,    /*  nchnls              */
    0,              /*  spoutactive         */
    0L,             /*  kcounter            */
    0L,             /*  global_kcounter     */
    0,              /*  reinitflag          */
    0,              /*  tieflag             */
    DFLT_SR,        /*  esr                 */
    FL(0.0),        /*  onedsr              */
    FL(0.0),        /*  sicvt               */
    FL(-1.0),       /*  tpidsr              */
    FL(-1.0),       /*  pidsr               */
    FL(-1.0),       /*  mpidsr              */
    FL(-1.0),       /*  mtpdsr              */
    FL(0.0),        /*  onedksmps           */
    DFLT_KR,        /*  ekr                 */
    DFLT_KR,        /*  global_ekr          */
    FL(0.0),        /*  onedkr              */
    FL(0.0),        /*  kicvt               */
    DFLT_DBFS,      /*  e0dbfs              */
    FL(1.0) / DFLT_DBFS, /* dbfs_to_float ( = 1.0 / e0dbfs) */
    0.0,            /*  timeOffs            */
    0.0,            /*  beatOffs            */
    0l,             /*  curTime             */
    0l,             /*  curTime_inc         */
    0.0,            /*  curBeat             */
    0.0,            /*  curBeat_inc         */
    0.0,            /*  beatTime            */
#if defined(HAVE_PTHREAD_SPIN_LOCK) && defined(PARCS)
    PTHREAD_SPINLOCK_INITIALIZER,              /*  spoutlock           */
    PTHREAD_SPINLOCK_INITIALIZER,              /*  spinlock            */
#else
    0,              /*  spoutlock           */
    0,              /*  spinlock            */
#endif
    NULL,           /*  widgetGlobals       */
    NULL,           /*  stdOp_Env           */
    NULL,           /*  zkstart             */
    NULL,           /*  zastart             */
    0L,             /*  zklast              */
    0L,             /*  zalast              */
    NULL,           /*  spin                */
    NULL,           /*  spout               */
    0,              /*  nspin               */
    0,              /*  nspout              */
    (OPARMS*) NULL, /*  oparms              */
    (EVTBLK*) NULL, /*  currevent           */
    (INSDS*) NULL,  /*  curip               */
    NULL,           /*  hostdata            */
    NULL,           /*  rtRecord_userdata   */
    NULL,           /*  rtPlay_userdata     */
    NULL, NULL,     /*  orchname, scorename */
    NULL, NULL,     /*  orchstr, *scorestr  */
    2345678,        /*  holdrand            */
    256,            /*  strVarMaxLen        */
    MAXINSNO,       /*  maxinsno            */
    0,              /*  strsmax             */
    (char**) NULL,  /*  strsets             */
    NULL,           /*  instrtxtp           */
    { NULL },       /*  m_chnbp             */
    NULL,           /*  csRtClock           */
    NULL,           /*  csRandState         */
    0,              /*  randSeed1           */
    0,              /*  randSeed2           */
#if defined(HAVE_PTHREAD_SPIN_LOCK) && defined(PARCS)
    PTHREAD_SPINLOCK_INITIALIZER,              /*  memlock           */
#else
    0,              /*  memlock             */
#endif
    sizeof(MYFLT),  /*  floatsize           */
    -1,             /*  inchns              */
    {0, 0, 0, 0, 0, 0, 0}, /* dummyint[7]; */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* dummyint32[10]; */
    /* ------- private data (not to be used by hosts or externals) ------- */
    /* callback function pointers */
    (SUBR) NULL,    /*  first_callback_     */
    (void (*)(CSOUND *, const char *, MYFLT *)) NULL,
    (void (*)(CSOUND *, const char *, MYFLT)) NULL,
    csoundDefaultMessageCallback,
    (int (*)(CSOUND *)) NULL,
    MakeAscii,
    DrawAscii,
    KillAscii,
    defaultCsoundExitGraph,
    defaultCsoundYield,
    defaultCsoundMakeXYin,
    defaultCsoundReadKillXYin,
    defaultCsoundReadKillXYin,
    cscore_,        /*  cscoreCallback_     */
    (void(*)(CSOUND*, const char*, int, int, int)) NULL, /* FileOpenCallback_ */
    (SUBR) NULL,    /*  last_callback_      */
    /* these are not saved on RESET */
    playopen_dummy,
    rtplay_dummy,
    recopen_dummy,
    rtrecord_dummy,
    rtclose_dummy,
    /* end of callbacks */
    0, 0,           /*  nchanik, nchania    */
    0, 0,           /*  nchanok, nchanoa    */
    NULL, NULL,     /*  chanik, chania      */
    NULL, NULL,     /*  chanok, chanoa      */
    FL(0.0),        /*  cpu_power_busy      */
    (char*) NULL,   /*  xfilename           */
    NLABELS,        /*  nlabels             */
    NGOTOS,         /*  ngotos              */
    1,              /*  peakchunks          */
    0,              /*  keep_tmp            */
    (OENTRY*) NULL, /*  opcodlst            */
    (int*) NULL,    /*  opcode_list         */
    (OENTRY*) NULL, /*  opcodlstend         */
    -1,             /*  maxopcno            */
    0,              /*  nrecs               */
    NULL,           /*  Linepipe            */
    0,              /*  Linefd              */
    NULL,           /*  csoundCallbacks_    */
    (FILE*)NULL,    /*  scfp                */
    (CORFIL*)NULL,  /*  scstr               */
    NULL,           /*  oscfp               */
    { FL(0.0) },    /*  maxamp              */
    { FL(0.0) },    /*  smaxamp             */
    { FL(0.0) },    /*  omaxamp             */
    {0}, {0}, {0},  /*  maxpos, smaxpos, omaxpos */
    NULL, NULL,     /*  scorein, scoreout   */
    NULL,           /*  pool                */
    NULL,           /*  argoffspace         */
    NULL,           /*  frstoff             */
#if defined(__WATCOMC__) || defined(MSVC) ||defined(__POWERPC__) || defined(mac_classic) || \
  (defined(_WIN32) && defined(__GNUC__))
    {0},
#else
    {{{0}}},        /*  exitjmp of type jmp_buf */
#endif
    NULL,           /*  frstbp              */
    0,              /*  sectcnt             */
    0, 0, 0,        /*  inerrcnt, synterrcnt, perferrcnt */
    {NULL},         /*  instxtanchor        */
    {NULL},         /*  actanchor           */
    {0L },          /*  rngcnt              */
    0, 0,           /*  rngflg, multichan   */
    NULL,           /*  evtFuncChain        */
    NULL,           /*  OrcTrigEvts         */
    NULL,           /*  freeEvtNodes        */
    1,              /*  csoundIsScorePending_ */
    0,              /*  advanceCnt          */
    0,              /*  initonly            */
    0,              /*  evt_poll_cnt        */
    0,              /*  evt_poll_maxcnt     */
    0, 0, 0,        /*  Mforcdecs, Mxtroffs, MTrkend */
    FL(-1.0), FL(-1.0), /*  tran_sr,tran_kr */
    FL(-1.0),       /*  tran_ksmps          */
    DFLT_DBFS,      /*  tran_0dbfs          */
    DFLT_NCHNLS,    /*  tran_nchnls         */
    NULL,           /*  opcodeInfo          */
    NULL,           /*  instrumentNames     */
    NULL,           /*  strsav_str          */
    NULL,           /*  strsav_space        */
    NULL,           /*  flist               */
    0,              /*  maxfnum             */
    NULL,           /*  gensub              */
    GENMAX+1,       /*  genmax              */
    100,            /*  ftldno              */
    NULL,           /*  namedGlobals        */
    0,              /*  namedGlobalsCurrLimit */
    0,              /*  namedGlobalsMaxLimit */
    NULL,           /*  cfgVariableDB       */
    0.0, 0.0, 0.0,  /*  prvbt, curbt, nxtbt */
    0.0, 0.0,       /*  curp2, nxtim        */
    0,              /*  cyclesRemaining     */
    { NULL, '\0', 0, FL(0.0), FL(0.0), { FL(0.0) }, NULL },   /*  evt     */
    NULL,           /*  memalloc_db         */
    (MGLOBAL*) NULL, /* midiGlobals         */
    NULL,           /*  envVarDB            */
    (MEMFIL*) NULL, /*  memfiles            */
    NULL,           /*  pvx_memfiles        */
    0,              /*  FFT_max_size        */
    NULL,           /*  FFT_table_1         */
    NULL,           /*  FFT_table_2         */
    NULL, NULL, NULL, /* tseg, tpsave, tplim */
    /* express.c */
    0L,             /*  polmax              */
    0L,             /*  toklen              */
    NULL,           /*  tokenstring         */
    NULL,           /*  polish              */
    NULL,           /*  token               */
    NULL,           /*  tokend              */
    NULL,           /*  tokens              */
    NULL,           /*  tokenlist           */
    TOKMAX,         /*  toklength           */
    0, 0, 0, 0, 0,  /*  acount, kcount, icount, Bcount, bcount */
    (char*) NULL,   /*  stringend           */
    NULL, NULL,     /*  revp, pushp         */
    NULL, NULL,     /*  argp, endlist       */
    (char*) NULL,   /*  assign_outarg       */
    0, 0, 0,        /*  argcnt_offs, opcode_is_assign, assign_type */
    0,              /*  strVarSamples       */
    (MYFLT*) NULL,  /*  gbloffbas           */
    NULL,           /*  otranGlobals        */
    NULL,           /*  rdorchGlobals       */
    NULL,           /*  sreadGlobals        */
    NULL,           /*  extractGlobals      */
    NULL,           /*  oneFileGlobals      */
    NULL,           /*  lineventGlobals     */
    NULL,           /*  musmonGlobals       */
    NULL,           /*  libsndGlobals       */
    (void (*)(CSOUND *)) NULL,                      /*  spinrecv    */
    (void (*)(CSOUND *)) NULL,                      /*  spoutran    */
    (int (*)(CSOUND *, MYFLT *, int)) NULL,         /*  audrecv     */
    (void (*)(CSOUND *, const MYFLT *, int)) NULL,  /*  audtran     */
    0,              /*  warped              */
    0,              /*  sstrlen             */
    (char*) NULL,   /*  sstrbuf             */
    1,              /*  enableMsgAttr       */
    0,              /*  sampsNeeded         */
    FL(0.0),        /*  csoundScoreOffsetSeconds_   */
    -1,             /*  inChar_             */
    0,              /*  isGraphable_        */
    0,              /*  delayr_stack_depth  */
    NULL,           /*  first_delayr        */
    NULL,           /*  last_delayr         */
    { 0L, 0L, 0L, 0L, 0L, 0L },     /*  revlpsiz    */
    0L,             /*  revlpsum            */
    0.5,            /*  rndfrac             */
    NULL,           /*  logbase2            */
    NULL, NULL,     /*  omacros, smacros    */
    NULL,           /*  namedgen            */
    NULL,           /*  open_files          */
    NULL,           /*  searchPathCache     */
    NULL,           /*  sndmemfiles         */
    NULL,           /*  reset_list          */
    NULL,           /*  pvFileTable         */
    0,              /*  pvNumFiles          */
    0,              /*  pvErrorCode         */
    NULL,           /*  pluginOpcodeFiles   */
    NULL,           /*  pluginOpcodeDB      */
    0,              /*  enableHostImplementedAudioIO  */
    0,              /*  hostRequestedBufferSize       */
    0,              /*  engineState         */
    0,              /*  stdin_assign_flg    */
    0,              /*  stdout_assign_flg   */
    0,              /*  orcname_mode        */
    NULL,           /*  csmodule_db         */
    (char*) NULL,   /*  dl_opcodes_oplibs   */
    (char*) NULL,   /*  SF_csd_licence      */
    (char*) NULL,   /*  SF_id_title         */
    (char*) NULL,   /*  SF_id_copyright     */
    (char*) NULL,   /*  SF_id_software      */
    (char*) NULL,   /*  SF_id_artist        */
    (char*) NULL,   /*  SF_id_comment       */
    (char*) NULL,   /*  SF_id_date          */
    NULL,           /*  utility_db          */
    (int16*) NULL,  /*  isintab             */
    NULL,           /*  lprdaddr            */
    0,              /*  currentLPCSlot      */
    0,              /*  max_lpc_slot        */
    NULL,           /*  chn_db              */
    1,              /*  opcodedirWasOK      */
    0,              /*  disable_csd_options */
    { 0, { 0U } },  /*  randState_          */
    0,              /*  performState        */
    1000,           /*  ugens4_rand_16      */
    1000,           /*  ugens4_rand_15      */
    NULL,           /*  schedule_kicked     */
    (LBLBLK**) NULL, /* lopds               */
    NULL,           /*  larg                */
    (MYFLT*) NULL,  /*  disprep_fftcoefs    */
    NULL,           /*  winEPS_globals      */
    {               /*  oparms_             */
      0,            /*    odebug            */
      0, 1, 1, 0,   /*    sfread, ...       */
      0, 0, 0, 0,   /*    inbufsamps, ...   */
      0,            /*    sfsampsize        */
      1,            /*    displays          */
      0, 0, 135, /*    disp.. graphsoff ... */
      0, 0, 0,      /*    Beatmode, ...     */
      0, 0,         /*    usingcscore, ...  */
      0, 0, 0, 0,   /*    RTevents, ...     */
      0, 0,         /*    ringbell, ...     */
      0, 0, 0,      /*    rewrt_hdr, ...    */
      0,            /*    expr_opt          */
      0.0f, 0.0f,   /*    sr_override ...  */
      (char*) NULL, (char*) NULL, NULL,
      (char*) NULL, (char*) NULL, (char*) NULL,
      (char*) NULL, (char*) NULL,
      0,            /*    midiKey           */
      0,            /*    midiKeyCps        */
      0,            /*    midiKeyOct        */
      0,            /*    midiKeyPch        */
      0,            /*    midiVelocity      */
      0,            /*    midiVelocityAmp   */
      0,            /*    noDefaultPaths    */
      1,            /*    numThreads        */
      0,            /*    syntaxCheckOnly   */
      1,            /*    useCsdLineCounts  */
#if defined(PARCS) ||  defined(ENABLE_NEW_PARSER)
#ifdef BETA
      1,            /*    newParser   */
#else
      0,            /*    newParser   */
#endif
#endif
      0,            /*    calculateWeights   */
    },
    0L, 0L,         /*  instxtcount, optxtsize  */
    0L, 0L,         /*  poolcount, gblfixed     */
    0L, 0L,         /*  gblacount, gblscount    */
    (CsoundChannelIOCallback_t) NULL,   /*  channelIOCallback_  */
    csoundDoCallback_,  /*  doCsoundCallback    */
    &(strhash_tabl_8[0]),   /*  strhash_tabl_8  */
    csound_str_hash_32, /*  strHash32           */
    {0, 0, {0}}, /* REMOT_BUF */
    NULL,           /* remoteGlobals        */
    0, 0,           /* nchanof, nchanif     */
    NULL, NULL,     /* chanif, chanof       */
    defaultCsoundYield, /* csoundInternalYieldCallback_*/
    NULL,           /* multiThreadedBarrier1 */
    NULL,           /* multiThreadedBarrier2 */
    0,              /* multiThreadedComplete */
    NULL,           /* multiThreadedThreadInfo */
    NULL,           /* multiThreadedStart */
    NULL,           /* multiThreadedEnd */
#ifdef PARCS
    NULL,           /* weight_info */
    NULL,           /* weight_dump */
    NULL,           /* weights */
    NULL,           /* multiThreadedDag */
    NULL,           /* barrier1 */
    NULL,           /* barrier2 */
    NULL,           /* global_var_lock_root */
    NULL,           /* global_var_lock_cache */
    0,              /* global_var_lock_count */
    0,              /* opcode_weight_cache_ctr */
    {NULL,NULL},    /* opcode_weight_cache[OPCODE_WEIGHT_CACHE_SIZE] */
    0,              /* opcode_weight_have_cache */
    {NULL,NULL},    /* ache[DAG_2_CACHE_SIZE] */
    /* statics from cs_par_orc_semantic_analysis */
    NULL,           /* instCurr */
    NULL,           /* instRoot */
    0,              /* inInstr */
#endif /* PARCS */
    0,              /* tempStatus */
    0,              /* orcLineOffset */
    0,              /* scoLineOffset */
    NULL,           /* csdname */
    -1,             /*  parserUdoflag */
    0,              /*  parserNamedInstrFlag */
    0,              /*  tran_nchnlsi */
    0,              /* Count of extra strings */
    {NULL, NULL, NULL}, /* For extra strings in scores */
    {0, 0, 0},      /* For extra strings in scores */
    300,             /* Count for generated labels */
    NULL,
    NULL
  };

  /* from threads.c */
  void csoundLock(void);
  void csoundUnLock(void);
  /* aops.c */
  /*void aops_init_tables(void);*/
  void csound_aops_init_tables(CSOUND *cs);

  typedef struct csInstance_s {
    CSOUND              *csound;
    struct csInstance_s *nxt;
  } csInstance_t;

  /* initialisation state: */
  /* 0: not done yet, 1: complete, 2: in progress, -1: failed */
  static  volatile  int init_done = 0;
  /* chain of allocated Csound instances */
  static  volatile  csInstance_t  *instance_list = NULL;
  /* non-zero if performance should be terminated now */
  static  volatile  int exitNow_ = 0;

  static void destroy_all_instances(void)
  {
      volatile csInstance_t *p;

      csoundLock();
      init_done = -1;     /* prevent the creation of any new instances */
      if (instance_list == NULL) {
        csoundUnLock();
        return;
      }
      csoundUnLock();
      csoundSleep(250);
      while (1) {
        csoundLock();
        p = instance_list;
        csoundUnLock();
        if (p == NULL) {
          break;
        }
        csoundDestroy(p->csound);
      }
  }

#if defined(ANDROID) || (!defined(LINUX) && !defined(SGI) && !defined(__BEOS__) && !defined(__MACH__))
  static char *signal_to_string(int sig)
  {
      switch(sig) {
#ifdef SIGHUP
      case SIGHUP:
        return "Hangup";
#endif
#ifdef SIGINT
      case SIGINT:
        return "Interrupt";
#endif
#ifdef SIGQUIT
      case SIGQUIT:
        return "Quit";
#endif
#ifdef SIGILL
      case SIGILL:
        return "Illegal instruction";
#endif
#ifdef SIGTRAP
      case SIGTRAP:
        return "Trace trap";
#endif
#ifdef SIGABRT
      case SIGABRT:
        return "Abort";
#endif
#ifdef SIGBUS
      case SIGBUS:
        return "BUS error";
#endif
#ifdef SIGFPE
      case SIGFPE:
        return "Floating-point exception";
#endif
#ifdef SIGUSR1
      case SIGUSR1:
        return "User-defined signal 1";
#endif
#ifdef SIGSEGV
      case SIGSEGV:
        return "Segmentation violation";
#endif
#ifdef SIGUSR2
      case SIGUSR2:
        return "User-defined signal 2";
#endif
#ifdef SIGPIPE
      case SIGPIPE:
        return "Broken pipe";
#endif
#ifdef SIGALRM
      case SIGALRM:
        return "Alarm clock";
#endif
#ifdef SIGTERM
      case SIGTERM:
        return "Termination";
#endif
#ifdef SIGSTKFLT
      case SIGSTKFLT:
        return "???";
#endif
#ifdef SIGCHLD
      case SIGCHLD:
        return "Child status has changed";
#endif
#ifdef SIGCONT
      case SIGCONT:
        return "Continue";
#endif
#ifdef SIGSTOP
      case SIGSTOP:
        return "Stop, unblockable";
#endif
#ifdef SIGTSTP
      case SIGTSTP:
        return "Keyboard stop";
#endif
#ifdef SIGTTIN
      case SIGTTIN:
        return "Background read from tty";
#endif
#ifdef SIGTTOU
      case SIGTTOU:
        return "Background write to tty";
#endif
#ifdef SIGURG
      case SIGURG:
        return "Urgent condition on socket ";
#endif
#ifdef SIGXCPU
      case SIGXCPU:
        return "CPU limit exceeded";
#endif
#ifdef SIGXFSZ
      case SIGXFSZ:
        return "File size limit exceeded ";
#endif
#ifdef SIGVTALRM
      case SIGVTALRM:
        return "Virtual alarm clock ";
#endif
#ifdef SIGPROF
      case SIGPROF:
        return "Profiling alarm clock";
#endif
#ifdef SIGWINCH
      case SIGWINCH:
        return "Window size change ";
#endif
#ifdef SIGIO
      case SIGIO:
        return "I/O now possible";
#endif
#ifdef SIGPWR
      case SIGPWR:
        return "Power failure restart";
#endif
      default:
        return "???";
      }
  }

  static void psignal(int sig, char *str)
  {
      fprintf(stderr, "%s: %s\n", str, signal_to_string(sig));
  }
#elif defined(__BEOS__)
  static void psignal(int sig, char *str)
  {
      fprintf(stderr, "%s: %s\n", str, strsignal(sig));
  }
#endif

  static void signal_handler(int sig)
  {
#if defined(SIGPIPE)
      if (sig == (int) SIGPIPE) {
        psignal(sig, "Csound ignoring SIGPIPE");
        return;
      }
#endif
      psignal(sig, "Csound tidy up");
      if ((sig == (int) SIGINT || sig == (int) SIGTERM) && !exitNow_) {
        exitNow_ = -1;
        return;
      }
      exit(1);
  }

  static const int sigs[] = {
#if defined(LINUX) || defined(SGI) || defined(sol) || defined(__MACH__)
    SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGIOT, SIGBUS,
    SIGFPE, SIGSEGV, SIGPIPE, SIGTERM, SIGXCPU, SIGXFSZ,
#elif defined(WIN32)
    SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGTERM,
#elif defined(__EMX__)
    SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGBUS, SIGFPE,
    SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGTERM, SIGCHLD,
#endif
    -1
  };

  static void install_signal_handler(void)
  {
      int i;
      for (i = 0; sigs[i] >= 0; i++) {
        signal(sigs[i], signal_handler);
      }
  }

  static int getTimeResolution(void);

  PUBLIC int csoundInitialize(int *argc, char ***argv, int flags)
  {
      int     n;

      (void) argc;
      (void) argv;
      do {
        csoundLock();
        n = init_done;
        switch (n) {
        case 2:
          csoundUnLock();
          csoundSleep(1);
        case 0:
          break;
        default:
          csoundUnLock();
          return n;
        }
      } while (n);
      init_done = 2;
      csoundUnLock();
      init_getstring();
      if (getTimeResolution() != 0) {
        csoundLock();
        init_done = -1;
        csoundUnLock();
        return -1;
      }
      if (!(flags & CSOUNDINIT_NO_SIGNAL_HANDLER)) {
        install_signal_handler();
      }
      if (!(flags & CSOUNDINIT_NO_ATEXIT))
#if !defined(WIN32)
        atexit(destroy_all_instances);
#endif
      /*aops_init_tables();*/
      csoundLock();
      init_done = 1;
      csoundUnLock();
      return 0;
  }

  PUBLIC CSOUND *csoundCreate(void *hostdata)
  {
      CSOUND        *csound;
      csInstance_t  *p;

      if (init_done != 1) {
        if (csoundInitialize(NULL, NULL, 0) < 0) return NULL;
      }
      csound = (CSOUND*) malloc(sizeof(CSOUND));
      if (UNLIKELY(csound == NULL)) return NULL;
      memcpy(csound, &cenviron_, sizeof(CSOUND));
      csound->oparms = &(csound->oparms_);
      csound->hostdata = hostdata;
      p = (csInstance_t*) malloc(sizeof(csInstance_t));
      if (UNLIKELY(p == NULL)) {
        free(csound);
        return NULL;
      }
      csoundLock();
      p->csound = csound;
      p->nxt = (csInstance_t*) instance_list;
      instance_list = p;
      csoundUnLock();
      csoundReset(csound);
      //csound_aops_init_tables(csound);
      return csound;
  }

  /* dummy real time MIDI functions */
  static int DummyMidiInOpen(CSOUND *csound, void **userData,
                             const char *devName);
  static int DummyMidiRead(CSOUND *csound, void *userData,
                           unsigned char *buf, int nbytes);
  static int DummyMidiOutOpen(CSOUND *csound, void **userData,
                              const char *devName);
  static int DummyMidiWrite(CSOUND *csound, void *userData,
                            const unsigned char *buf, int nbytes);
  /* random.c */
  extern void csound_init_rand(CSOUND *);

  /**
   * Reset and prepare an instance of Csound for compilation.
   * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
   * CSOUND_MEMORY if an error occured.
   */
  PUBLIC int csoundPreCompile(CSOUND *p)
  {
      char    *s;
      int     i, max_len;
      int     n;

      if ((n = setjmp(p->exitjmp)) != 0) {
        return ((n - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
      }
      /* reset instance */
      csoundReset(p);
      /* copy system environment variables */
      i = csoundInitEnv(p);
      if (UNLIKELY(i != CSOUND_SUCCESS)) {
        p->engineState |= CS_STATE_JMP;
        return i;
      }
      csound_init_rand(p);
      /* allow selecting real time audio module */
      max_len = 21;
      csoundCreateGlobalVariable(p, "_RTAUDIO", (size_t) max_len);
      s = csoundQueryGlobalVariable(p, "_RTAUDIO");
      strcpy(s, "PortAudio");
      csoundCreateConfigurationVariable(p, "rtaudio", s, CSOUNDCFG_STRING,
                                        0, NULL, &max_len,
                                        "Real time audio module name", NULL);

      /* initialise real time MIDI */
      p->midiGlobals = (MGLOBAL*) mcalloc(p, sizeof(MGLOBAL));
      p->midiGlobals->Midevtblk = (MEVENT*) NULL;
      p->midiGlobals->MidiInOpenCallback = DummyMidiInOpen;
      p->midiGlobals->MidiReadCallback = DummyMidiRead;
      p->midiGlobals->MidiInCloseCallback = (int (*)(CSOUND *, void *)) NULL;
      p->midiGlobals->MidiOutOpenCallback = DummyMidiOutOpen;
      p->midiGlobals->MidiWriteCallback = DummyMidiWrite;
      p->midiGlobals->MidiOutCloseCallback = (int (*)(CSOUND *, void *)) NULL;
      p->midiGlobals->MidiErrorStringCallback = (const char *(*)(int)) NULL;
      p->midiGlobals->midiInUserData = NULL;
      p->midiGlobals->midiOutUserData = NULL;
      p->midiGlobals->midiFileData = NULL;
      p->midiGlobals->midiOutFileData = NULL;
      p->midiGlobals->bufp = &(p->midiGlobals->mbuf[0]);
      p->midiGlobals->endatp = p->midiGlobals->bufp;
      csoundCreateGlobalVariable(p, "_RTMIDI", (size_t) max_len);
      s = csoundQueryGlobalVariable(p, "_RTMIDI");
      strcpy(s, "portmidi");
      csoundCreateConfigurationVariable(p, "rtmidi", s, CSOUNDCFG_STRING,
                                        0, NULL, &max_len,
                                        "Real time MIDI module name", NULL);
      max_len = 256;  /* should be the same as in csoundCore.h */
      csoundCreateConfigurationVariable(p, "mute_tracks",
                                        &(p->midiGlobals->muteTrackList[0]),
                                        CSOUNDCFG_STRING, 0, NULL, &max_len,
                                        "Ignore events (other than tempo changes)"
                                        " in tracks defined by pattern",
                                        NULL);
      csoundCreateConfigurationVariable(p, "raw_controller_mode",
                                        &(p->midiGlobals->rawControllerMode),
                                        CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                        "Do not handle special MIDI controllers"
                                        " (sustain pedal etc.)", NULL);
      /* sound file tag options */
      max_len = 201;
      i = (max_len + 7) & (~7);
      p->SF_id_title = (char*) mcalloc(p, (size_t) i * (size_t) 6);
      csoundCreateConfigurationVariable(p, "id_title", p->SF_id_title,
                                        CSOUNDCFG_STRING, 0, NULL, &max_len,
                                        "Title tag in output soundfile "
                                        "(no spaces)", NULL);
      p->SF_id_copyright = (char*) p->SF_id_title + (int) i;
      csoundCreateConfigurationVariable(p, "id_copyright", p->SF_id_copyright,
                                        CSOUNDCFG_STRING, 0, NULL, &max_len,
                                        "Copyright tag in output soundfile"
                                        " (no spaces)", NULL);
      p->SF_id_software = (char*) p->SF_id_copyright + (int) i;
      csoundCreateConfigurationVariable(p, "id_software", p->SF_id_software,
                                        CSOUNDCFG_STRING, 0, NULL, &max_len,
                                        "Software tag in output soundfile"
                                        " (no spaces)", NULL);
      p->SF_id_artist = (char*) p->SF_id_software + (int) i;
      csoundCreateConfigurationVariable(p, "id_artist", p->SF_id_artist,
                                        CSOUNDCFG_STRING, 0, NULL, &max_len,
                                        "Artist tag in output soundfile "
                                        "(no spaces)",
                                        NULL);
      p->SF_id_comment = (char*) p->SF_id_artist + (int) i;
      csoundCreateConfigurationVariable(p, "id_comment", p->SF_id_comment,
                                        CSOUNDCFG_STRING, 0, NULL, &max_len,
                                        "Comment tag in output soundfile"
                                        " (no spaces)", NULL);
      p->SF_id_date = (char*) p->SF_id_comment + (int) i;
      csoundCreateConfigurationVariable(p, "id_date", p->SF_id_date,
                                        CSOUNDCFG_STRING, 0, NULL, &max_len,
                                        "Date tag in output soundfile "
                                        "(no spaces)",
                                        NULL);
      {
        int   minVal = 10;
        int   maxVal = 10000;
        MYFLT minValF = FL(0.0);
        /* max. length of string variables */
        csoundCreateConfigurationVariable(p, "max_str_len", &(p->strVarMaxLen),
                                          CSOUNDCFG_INTEGER, 0, &minVal, &maxVal,
                                          "Maximum length of string variables + 1",
                                          NULL);
        csoundCreateConfigurationVariable(p, "msg_color", &(p->enableMsgAttr),
                                          CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                          "Enable message attributes (colors etc.)",
                                          NULL);
        csoundCreateConfigurationVariable(p, "skip_seconds",
                                          &(p->csoundScoreOffsetSeconds_),
                                          CSOUNDCFG_MYFLT, 0, &minValF, NULL,
                                          "Start score playback at the specified"
                                          " time, skipping earlier events",
                                          NULL);
      }
      csoundCreateConfigurationVariable(p, "ignore_csopts",
                                        &(p->disable_csd_options),
                                        CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                        "Ignore <CsOptions> in CSD files"
                                        " (default: no)", NULL);
      p->opcode_list = (int*) mcalloc(p, sizeof(int) * 256);
      p->engineState |= CS_STATE_PRE;
      csound_aops_init_tables(p);
      /* now load and pre-initialise external modules for this instance */
      /* this function returns an error value that may be worth checking */
      {
        int err = csoundInitStaticModules(p);
        if (p->delayederrormessages && p->printerrormessagesflag==NULL) {
          p->Warning(p, p->delayederrormessages);
          free(p->delayederrormessages);
          p->delayederrormessages = NULL;
        }
        if (UNLIKELY(err==CSOUND_ERROR)) return err;
        err = csoundLoadModules(p);
        if (p->delayederrormessages && p->printerrormessagesflag==NULL) {
          p->Warning(p, p->delayederrormessages);
          free(p->delayederrormessages);
          p->delayederrormessages = NULL;
        }
        return err;
      }
  }


  PUBLIC int csoundQueryInterface(const char *name, void **iface, int *version)
  {
      if (strcmp(name, "CSOUND") != 0)
        return 1;
      *iface = csoundCreate(NULL);
      *version = csoundGetAPIVersion();
      return 0;
  }

  typedef struct CsoundCallbackEntry_s CsoundCallbackEntry_t;

  struct CsoundCallbackEntry_s {
    unsigned int  typeMask;
    CsoundCallbackEntry_t *nxt;
    void    *userData;
    int     (*func)(void *, void *, unsigned int);
  };

  PUBLIC void csoundDestroy(CSOUND *csound)
  {
      csInstance_t  *p, *prv = NULL;

      csoundLock();
      p = (csInstance_t*) instance_list;
      while (p != NULL && p->csound != csound) {
        prv = p;
        p = p->nxt;
      }
      if (p == NULL) {
        csoundUnLock();
        return;
      }
      if (prv == NULL)
        instance_list = p->nxt;
      else
        prv->nxt = p->nxt;
      csoundUnLock();
      free(p);
      csoundReset(csound);
      if (csound->csoundCallbacks_ != NULL) {
        CsoundCallbackEntry_t *pp, *nxt;
        pp = (CsoundCallbackEntry_t*) csound->csoundCallbacks_;
        do {
          nxt = pp->nxt;
          free((void*) pp);
          pp = nxt;
        } while (pp != (CsoundCallbackEntry_t*) NULL);
      }
      free((void*) csound);
  }

  PUBLIC int csoundGetVersion(void)
  {
      return (int) (CS_VERSION * 1000 + CS_SUBVER * 10 + CS_PATCHLEVEL);
  }

  PUBLIC int csoundGetAPIVersion(void)
  {
      return CS_APIVERSION * 100 + CS_APISUBVER;
  }

  PUBLIC void *csoundGetHostData(CSOUND *csound)
  {
      return csound->hostdata;
  }

  PUBLIC void csoundSetHostData(CSOUND *csound, void *hostData)
  {
      csound->hostdata = hostData;
  }

  /*
   * PERFORMANCE
   */

  extern int sensevents(CSOUND *);

  /**
   * perform currently active instrs for one kperiod
   *      & send audio result to output buffer
   * returns non-zero if this kperiod was skipped
   */

  static int getThreadIndex(CSOUND *csound, void *threadId)
  {
#ifndef mac_classic
      int index = 0;
      THREADINFO *current = csound->multiThreadedThreadInfo;

      if (current == NULL) {
        return -1;
      }

      while(current != NULL) {
        if (pthread_equal(*(pthread_t *)threadId, *(pthread_t *)current->threadId))
          return index;
        index++;
        current = current->next;
      }
#endif
      return -1;
  }

#if 0
  static int getNumActive(INSDS *start, INSDS *end)
  {
      INSDS *current = start;
      int counter = 1;
      while(((current = current->nxtact) != NULL) && current != end) {
        counter++;
      }
      return counter;
  }
#endif

  inline void advanceINSDSPointer(INSDS ***start, int num)
  {
      int i;
      INSDS *s = **start;

      if (s == NULL) return;
      for (i = 0; i < num; i++) {
        s = s->nxtact;

        if (s == NULL) {
          **start = NULL;
          return;
        }
      }
      **start = s;
  }
#if defined(USE_OPENMP)
  inline void multiThreadedLayer(CSOUND *csound, INSDS *layerBegin, INSDS *layerEnd)
  {
      // A single thread iterates over all instrument instances
      // in the current layer...
      INSDS *currentInstance;
#pragma omp parallel
      {
#pragma omp single
        {
          for (currentInstance = layerBegin;
               currentInstance && (currentInstance != layerEnd);
               currentInstance = currentInstance->nxtact) {
            // ...but each instance is computed in its own thread.
#pragma omp task firstprivate(currentInstance)
            {
              OPDS *currentOpcode = (OPDS *)currentInstance;
              while ((currentOpcode = currentOpcode->nxtp)) {
                (*currentOpcode->opadr)(csound, currentOpcode);
              }
            }
          }
        }
      }
  }
#endif




#ifdef PARCS
  static int inline nodePerf(CSOUND *csound, int index)
  {
      struct instr_semantics_t *instr = NULL;
      INSDS *insds = NULL;
      OPDS  *opstart = NULL;
      int update_hdl = -1;
      int played_count = 0;
      DAG_NODE *node;

      do {
        TRACE_2("Consume DAG [%i]\n", index);
        csp_dag_consume(csound, csound->multiThreadedDag, &node, &update_hdl);

        if (UNLIKELY(node == NULL)) {
          return played_count;
        }

        if (node->hdr.type == DAG_NODE_INDV) {
          instr = node->instr;
          insds = node->insds;
          played_count++;

          TRACE_2("DAG_NODE_INDV [%i] Playing: %s [%p]\n", index, instr->name, insds);

          opstart = (OPDS *)insds;
          while ((opstart = opstart->nxtp) != NULL) {
            (*opstart->opadr)(csound, opstart); /* run each opcode */
          }

          TRACE_2("[%i] Played:  %s [%p]\n", index, instr->name, insds);
        }
        else if (node->hdr.type == DAG_NODE_LIST) {
          played_count += node->count;

          int node_ctr = 0;
          while (node_ctr < node->count) {
            DAG_NODE *play_node = node->nodes[node_ctr];
            instr = play_node->instr;
            insds = play_node->insds;
            TRACE_1("DAG_NODE_LIST: node->nodes=%p: play_node = %p, instr=%p, insds=%p\n",
                    node->nodes, play_node, instr, insds);

            TRACE_2("[%i] Playing: %s [%p]\n", index, instr->name, insds);

            opstart = (OPDS *)insds;
            while ((opstart = opstart->nxtp) != NULL) {
              /* csound->Message(csound, "**opstart=%p; opadr=%p (%s)\n", opstart, */
              /*                 opstart->opadr, opstart->optext->t.opcod); */
              (*opstart->opadr)(csound, opstart); /* run each opcode */
            }

            TRACE_2("[%i] Played:  %s [%p]\n", index, instr->name, insds);
            node_ctr++;
          }
        }
        else if (node->hdr.type == DAG_NODE_DAG) {
          csound->Die(csound, "Recursive DAGs not implemented");
        }
        else {
          csound->Die(csound, "Unknown DAG node type");
        }

        csp_dag_consume_update(csound, csound->multiThreadedDag, update_hdl);
      } while (!csp_dag_is_finished(csound, csound->multiThreadedDag));

      return played_count;
  }

  unsigned long kperfThread(void * cs)
  {
      INSDS *start;
      CSOUND *csound = (CSOUND *)cs;
      void *threadId;
      int index;
      int numThreads;

      csound->WaitBarrier(csound->barrier2);

      threadId = csound->GetCurrentThreadID();
      index = getThreadIndex(csound, threadId);
      numThreads = csound->oparms->numThreads;
      start = NULL;
      csound->Message(csound,
                      "Multithread performance: insno: %3d  thread %d of "
                      "%d starting.\n",
                      start ? start->insno : -1,
                      index,
                      numThreads);
      if (index < 0) {
        csound->Die(csound, "Bad ThreadId");
        return ULONG_MAX;
      }
      index++;

      while (1) {

        TRACE_1("[%i] Barrier1 Reached\n", index);
        SHARK_SIGNPOST(BARRIER_1_WAIT_SYM);
        csound->WaitBarrier(csound->barrier1);

        TRACE_1("[%i] Go\n", index);

        /* TIMER_INIT(mutex, "Mutex ");
           TIMER_T_START(mutex, index, "Mutex "); */

        csound_global_mutex_lock();
        if (csound->multiThreadedComplete == 1) {
          csound_global_mutex_unlock();
          free(threadId);
          /* csound->Message(csound,
             "Multithread performance: insno: %3d  thread "
             "%d of %d exiting.\n",
             start->insno,
             index,
             numThreads); */
          return 0UL;
        }
        csound_global_mutex_unlock();

        /* TIMER_T_END(mutex, index, "Mutex "); */

        TIMER_INIT(thread, "");
        TIMER_T_START(thread, index, "");

          nodePerf(csound, index);

          TIMER_T_END(thread, index, "");

          TRACE_1("[%i] Done\n", index);

        SHARK_SIGNPOST(BARRIER_2_WAIT_SYM);
        csound->WaitBarrier(csound->barrier2);
        TRACE_1("[%i] Barrier2 Done\n", index);
      }
  }
#endif /* ! PARCS */

  inline void singleThreadedLayer(CSOUND *csound,
                                  INSDS *layerBegin, INSDS *layerEnd)
  {
      INSDS *currentInstance;
      for (currentInstance = layerBegin;
           currentInstance && (currentInstance != layerEnd);
           currentInstance = currentInstance->nxtact) {
        csound->pds = (OPDS *)currentInstance;
        while ((csound->pds = csound->pds->nxtp)) {
          (*csound->pds->opadr)(csound, csound->pds);
        }
      }
  }

  int kperf(CSOUND *csound)
  {
#ifdef PARCS
      void *barrier1, *barrier2;
      INSDS *ip;
#endif /* PARCS */
      /* update orchestra time */
      csound->kcounter = ++(csound->global_kcounter);
      csound->icurTime += csound->ksmps;
      csound->curBeat += csound->curBeat_inc;
      /* if skipping time on request by 'a' score statement: */
      if (UNLIKELY(csound->advanceCnt)) {
        csound->advanceCnt--;
        return 1;
      }
      /* if i-time only, return now */
      if (UNLIKELY(csound->initonly))
        return 1;
      /* PC GUI needs attention, but avoid excessively frequent */
      /* calls of csoundYield() */
      if (--(csound->evt_poll_cnt) < 0) {
        csound->evt_poll_cnt = csound->evt_poll_maxcnt;
        if (!csoundYield(csound))
         csound->LongJmp(csound, 1);
       }
      /* for one kcnt: */
      if (csound->oparms_.sfread)         /*   if audio_infile open  */
        csound->spinrecv(csound);         /*      fill the spin buf  */
      csound->spoutactive = 0;            /*   make spout inactive   */
#ifndef PARCS
      if (csound->actanchor.nxtact) {
#if defined(USE_OPENMP)
        if (csound->oparms->numThreads > 1) {
          INSDS *layerBegin;
          INSDS *currentInstance;
          int layerInstances = 0;
          for (currentInstance = layerBegin = csound->actanchor.nxtact;
               currentInstance;
               currentInstance = currentInstance->nxtact) {
            if (!currentInstance->nxtact) {
              if (layerInstances > 1) {
                multiThreadedLayer(csound, layerBegin, 0);
              } else {
                singleThreadedLayer(csound, layerBegin, 0);
              }
            } else {
              layerInstances++;
              if (((int) layerBegin->insno) != ((int) currentInstance->insno)) {
                if (layerInstances > 1) {
                  multiThreadedLayer(csound, layerBegin, currentInstance);
                } else {
                  singleThreadedLayer(csound, layerBegin, currentInstance);
                }
                layerBegin = currentInstance;
                layerInstances = 0;
              }
            }
          }
        } else {
          singleThreadedLayer(csound, csound->actanchor.nxtact, 0);
        }
#else /* MPI */
         INSDS *ip = csound->actanchor.nxtact;
        while (ip != NULL) {                /* for each instr active:  */
          INSDS *nxt = ip->nxtact;
          csound->pds = (OPDS*) ip;
          while ((csound->pds = csound->pds->nxtp) != NULL) {
            (*csound->pds->opadr)(csound, csound->pds); /* run each opcode */
          }
          ip = nxt; /* but this does not allow for all deletions */
        }
#endif
      }
#else /* PARCS */
      barrier1 = csound->multiThreadedBarrier1;
      barrier2 = csound->multiThreadedBarrier2;
      ip = csound->actanchor.nxtact;

      if (ip != NULL) {
        TIMER_INIT(thread, "");
        TIMER_START(thread, "Clock Sync ");
        TIMER_END(thread, "Clock Sync ");

          SHARK_SIGNPOST(KPERF_SYM);
        TRACE_1("[%i] kperf\n", 0);

        /* There are 2 partitions of work: 1st by inso,
           2nd by inso count / thread count. */
        if (csound->multiThreadedThreadInfo != NULL) {
          struct dag_t *dag2 = NULL;

          TIMER_START(thread, "Dag ");
#if defined(LINEAR_CACHE) || defined(HASH_CACHE)
          csp_dag_cache_fetch(csound, &dag2, ip);
          csp_dag_build(csound, &dag2, ip);
#endif
          TIMER_END(thread, "Dag ");

          TRACE_1("{Time: %f}\n", csound->GetScoreTime(csound));
#if TRACE > 1
          csp_dag_print(csound, dag2);
#endif
          csound->multiThreadedDag = dag2;

          /* process this partition */
          TRACE_1("[%i] Barrier1 Reached\n", 0);
          SHARK_SIGNPOST(BARRIER_1_WAIT_SYM);
          csound->WaitBarrier(csound->barrier1);

          TIMER_START(thread, "[0] ");

          (void) nodePerf(csound, 0);

          TIMER_END(thread, "[0] ");

            SHARK_SIGNPOST(BARRIER_2_WAIT_SYM);
          /* wait until partition is complete */
          csound->WaitBarrier(csound->barrier2);
          TRACE_1("[%i] Barrier2 Done\n", 0);
          TIMER_END(thread, "");

#if !defined(LINEAR_CACHE) && !defined(HASH_CACHE)
            csp_dag_dealloc(csound, &dag2);
#else
          dag2 = NULL;
#endif
          csound->multiThreadedDag = NULL;
        }
        else {
          while (ip != NULL) {                /* for each instr active:  */
            INSDS *nxt = ip->nxtact;
            csound->pds = (OPDS*) ip;
            while ((csound->pds = csound->pds->nxtp) != NULL) {
              (*csound->pds->opadr)(csound, csound->pds); /* run each opcode */
            }
            ip = nxt; /* but this does not allow for all deletions */
          }
        }
      }
#endif /* PARCS */
      if (!csound->spoutactive) {             /*   results now in spout? */
        memset(csound->spout, 0, csound->nspout * sizeof(MYFLT));
      }
      csound->spoutran(csound);               /*      send to audio_out  */
      return 0;
  }

  PUBLIC int csoundPerformKsmps(CSOUND *csound)
  {
      int done;
      int returnValue;
      /* setup jmp for return after an exit() */
      if ((returnValue = setjmp(csound->exitjmp))) {
#ifndef MACOSX
        csoundMessage(csound, Str("Early return from csoundPerformKsmps().\n"));
#endif
        return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
      }
      do {
        if (UNLIKELY((done = sensevents(csound)))) {
          csoundMessage(csound, Str("Score finished in csoundPerformKsmps().\n"));
          return done;
        }
      } while (kperf(csound));
      return 0;
  }

  PUBLIC int csoundPerformKsmpsAbsolute(CSOUND *csound)
  {
      int done = 0;
      int returnValue;
      /* setup jmp for return after an exit() */
      if ((returnValue = setjmp(csound->exitjmp))) {
#ifndef MACOSX
        csoundMessage(csound, Str("Early return from csoundPerformKsmps().\n"));
#endif
        return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
      }
      do {
        done |= sensevents(csound);
      } while (kperf(csound));
      return done;
  }

  /* external host's outbuffer passed in csoundPerformBuffer() */

  PUBLIC int csoundPerformBuffer(CSOUND *csound)
  {
      int returnValue;
      int done;
      /* Setup jmp for return after an exit(). */
      if ((returnValue = setjmp(csound->exitjmp))) {
#ifndef MACOSX
        csoundMessage(csound, Str("Early return from csoundPerformBuffer().\n"));
#endif
        return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
      }
      csound->sampsNeeded += csound->oparms_.outbufsamps;
      while (csound->sampsNeeded > 0) {
        do {
          if (UNLIKELY((done = sensevents(csound))))
            return done;
        } while (kperf(csound));
        csound->sampsNeeded -= csound->nspout;
      }
      return 0;
  }

  /* perform an entire score */

  PUBLIC int csoundPerform(CSOUND *csound)
  {
      int done;
      int returnValue;
      csound->performState = 0;
      /* setup jmp for return after an exit() */
      if ((returnValue = setjmp(csound->exitjmp))) {
#ifndef MACOSX
        csoundMessage(csound, Str("Early return from csoundPerform().\n"));
#endif
        return ((returnValue - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
      }
      do {
        do {
          if ((done = sensevents(csound))) {
            csoundMessage(csound, Str("Score finished in csoundPerform().\n"));
#ifdef PARCS
            if (csound->oparms->numThreads > 1) {
#if   defined(LINEAR_CACHE) || defined(HASH_CACHE)
              csp_dag_cache_print(csound);
#endif
              csound->multiThreadedComplete = 1;

              csound->WaitBarrier(csound->barrier1);
            }
            if (csound->oparms->calculateWeights) {
              /* csp_weights_dump(csound); */
              csp_weights_dump_normalised(csound);
            }

#endif /* PARCS */
            return done;
          }
        } while (kperf(csound));
      } while ((unsigned char) csound->performState == (unsigned char) 0);
      csoundMessage(csound, Str("csoundPerform(): stopped.\n"));
      csound->performState = 0;
      return 0;
  }

  /* stop a csoundPerform() running in another thread */

  PUBLIC void *csoundGetNamedGens(CSOUND *csound)
  {
      return csound->namedgen;
  }

  PUBLIC void csoundStop(CSOUND *csound)
  {
      csound->performState = -1;
  }

  /*
   * ATTRIBUTES
   */

  PUBLIC MYFLT csoundGetSr(CSOUND *csound)
  {
      return csound->esr;
  }

  PUBLIC MYFLT csoundGetKr(CSOUND *csound)
  {
      return csound->ekr;
  }

  PUBLIC int csoundGetKsmps(CSOUND *csound)
  {
      return csound->ksmps;
  }

  PUBLIC int csoundGetNchnls(CSOUND *csound)
  {
      return csound->nchnls;
  }

  PUBLIC MYFLT csoundGet0dBFS(CSOUND *csound)
  {
      return csound->e0dbfs;
  }

  PUBLIC int csoundGetStrVarMaxLen(CSOUND *csound)
  {
      return csound->strVarMaxLen;
  }

  PUBLIC int csoundGetSampleFormat(CSOUND *csound)
  {
      /* should we assume input is same as output ? */
      return csound->oparms_.outformat;
  }

  PUBLIC int csoundGetSampleSize(CSOUND *csound)
  {
      /* should we assume input is same as output ? */
      return csound->oparms_.sfsampsize;
  }

  PUBLIC long csoundGetInputBufferSize(CSOUND *csound)
  {
      return csound->oparms_.inbufsamps;
  }

  PUBLIC long csoundGetOutputBufferSize(CSOUND *csound)
  {
      return csound->oparms_.outbufsamps;
  }

  PUBLIC MYFLT *csoundGetSpin(CSOUND *csound)
  {
      return csound->spin;
  }

  PUBLIC void csoundAddSpinSample(CSOUND *csound, int frame,
                                  int channel, MYFLT sample)
  {

      int index = (frame * csound->inchnls) + channel;
      csound->spin[index] += sample;
  }

  PUBLIC MYFLT *csoundGetSpout(CSOUND *csound)
  {
      return csound->spout;
  }

  PUBLIC MYFLT csoundGetSpoutSample(CSOUND *csound, int frame, int channel)
  {
      int index = (frame * csound->nchnls) + channel;
      return csound->spout[index];
  }

  PUBLIC const char *csoundGetOutputFileName(CSOUND *csound)
  {
      return (const char*) csound->oparms_.outfilename;
  }

  /**
   * Calling this function with a non-zero 'state' value between
   * csoundPreCompile() and csoundCompile() will disable all default
   * handling of sound I/O by the Csound library, allowing the host
   * application to use the spin/spout/input/output buffers directly.
   * If 'bufSize' is greater than zero, the buffer size (-b) will be
   * set to the integer multiple of ksmps that is nearest to the value
   * specified.
   */

  PUBLIC void csoundSetHostImplementedAudioIO(CSOUND *csound,
                                              int state, int bufSize)
  {
      csound->enableHostImplementedAudioIO = state;
      csound->hostRequestedBufferSize = (bufSize > 0 ? bufSize : 0);
  }

  PUBLIC double csoundGetScoreTime(CSOUND *csound)
  {
      return (double)csound->icurTime/csound->esr;
  }

  /*
   * SCORE HANDLING
   */

  PUBLIC int csoundIsScorePending(CSOUND *csound)
  {
      return csound->csoundIsScorePending_;
  }

  PUBLIC void csoundSetScorePending(CSOUND *csound, int pending)
  {
      csound->csoundIsScorePending_ = pending;
  }

  PUBLIC void csoundSetScoreOffsetSeconds(CSOUND *csound, MYFLT offset)
  {
      double  aTime;
      MYFLT   prv = (MYFLT) csound->csoundScoreOffsetSeconds_;

      csound->csoundScoreOffsetSeconds_ = offset;
      if (offset < FL(0.0))
        return;
      /* if csoundCompile() was not called yet, just store the offset */
      if (!(csound->engineState & CS_STATE_COMP))
        return;
      /* otherwise seek to the requested time now */
      aTime = (double) offset - (csound->icurTime/csound->esr);
      if (aTime < 0.0 || offset < prv) {
        csoundRewindScore(csound);    /* will call csoundSetScoreOffsetSeconds */
        return;
      }
      if (aTime > 0.0) {
        EVTBLK  evt;
        evt.strarg = NULL;
        evt.opcod = 'a';
        evt.pcnt = 3;
        evt.p[2] = evt.p[1] = FL(0.0);
        evt.p[3] = (MYFLT) aTime;
        insert_score_event_at_sample(csound, &evt, csound->icurTime);
      }
  }

  PUBLIC MYFLT csoundGetScoreOffsetSeconds(CSOUND *csound)
  {
      return csound->csoundScoreOffsetSeconds_;
  }

  extern void musmon_rewind_score(CSOUND *csound);      /* musmon.c */
  extern void midifile_rewind_score(CSOUND *csound);    /* midifile.c */

  PUBLIC void csoundRewindScore(CSOUND *csound)
  {
      musmon_rewind_score(csound);
      midifile_rewind_score(csound);
  }

  PUBLIC void csoundSetCscoreCallback(CSOUND *p,
                                      void (*cscoreCallback)(CSOUND *))
  {
      p->cscoreCallback_ = (cscoreCallback != NULL ? cscoreCallback : cscore_);
  }

  static void csoundDefaultMessageCallback(CSOUND *csound, int attr,
                                           const char *format, va_list args)
  {
#if defined(WIN32) || defined(MAC)
      switch (attr & CSOUNDMSG_TYPE_MASK) {
      case CSOUNDMSG_ERROR:
      case CSOUNDMSG_WARNING:
      case CSOUNDMSG_REALTIME:
        vfprintf(stderr, format, args);
        break;
      default:
        vfprintf(stdout, format, args);
      }
#else
      if (!attr || !csound->enableMsgAttr) {
        vfprintf(stderr, format, args);
        return;
      }
      if ((attr & CSOUNDMSG_TYPE_MASK) == CSOUNDMSG_ORCH)
      if (attr & CSOUNDMSG_BG_COLOR_MASK)
        fprintf(stderr, "\033[4%cm", ((attr & 0x70) >> 4) + '0');
      if (attr & CSOUNDMSG_FG_ATTR_MASK) {
        if (attr & CSOUNDMSG_FG_BOLD)
          fprintf(stderr, "\033[1m");
        if (attr & CSOUNDMSG_FG_UNDERLINE)
          fprintf(stderr, "\033[4m");
      }
      if (attr & CSOUNDMSG_FG_COLOR_MASK)
        fprintf(stderr, "\033[3%cm", (attr & 7) + '0');
      vfprintf(stderr, format, args);
      fprintf(stderr, "\033[m");
#endif
  }

  PUBLIC void csoundSetMessageCallback(CSOUND *csound,
                                       void (*csoundMessageCallback)(CSOUND *csound,
                                                                     int attr,
                                                                     const char *format,
                                                                     va_list args))
  {
      /* Protect against a null callback. */
      if (csoundMessageCallback) {
        csound->csoundMessageCallback_ = csoundMessageCallback;
      } else {
        csound->csoundMessageCallback_ = csoundDefaultMessageCallback;
      }
  }

  PUBLIC void csoundMessageV(CSOUND *csound,
                             int attr, const char *format, va_list args)
  {
      csound->csoundMessageCallback_(csound, attr, format, args);
  }

  PUBLIC void csoundMessage(CSOUND *csound, const char *format, ...)
  {
      va_list args;
      va_start(args, format);
      csound->csoundMessageCallback_(csound, 0, format, args);
      va_end(args);
  }

  PUBLIC void csoundMessageS(CSOUND *csound, int attr, const char *format, ...)
  {
      va_list args;
      va_start(args, format);
      csound->csoundMessageCallback_(csound, attr, format, args);
      va_end(args);
  }

  void csoundDie(CSOUND *csound, const char *msg, ...)
  {
      va_list args;
      va_start(args, msg);
      csound->ErrMsgV(csound, (char*) 0, msg, args);
      va_end(args);
      csound->perferrcnt++;
      csound->LongJmp(csound, 1);
  }

  void csoundWarning(CSOUND *csound, const char *msg, ...)
  {
      va_list args;
      if (!(csound->oparms_.msglevel & WARNMSG))
        return;
      csoundMessageS(csound, CSOUNDMSG_WARNING, Str("WARNING: "));
      va_start(args, msg);
      csound->csoundMessageCallback_(csound, CSOUNDMSG_WARNING, msg, args);
      va_end(args);
      csoundMessageS(csound, CSOUNDMSG_WARNING, "\n");
  }

  void csoundDebugMsg(CSOUND *csound, const char *msg, ...)
  {
      va_list args;
      if (!(csound->oparms_.odebug))
        return;
      va_start(args, msg);
      csound->csoundMessageCallback_(csound, 0, msg, args);
      va_end(args);
      csoundMessage(csound, "\n");
  }

  void csoundErrorMsg(CSOUND *csound, const char *msg, ...)
  {
      va_list args;
      va_start(args, msg);
      csound->csoundMessageCallback_(csound, CSOUNDMSG_ERROR, msg, args);
      va_end(args);
      csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
  }

  void csoundErrMsgV(CSOUND *csound,
                     const char *hdr, const char *msg, va_list args)
  {
      if (hdr != NULL)
        csound->MessageS(csound, CSOUNDMSG_ERROR, "%s", hdr);
      csound->csoundMessageCallback_(csound, CSOUNDMSG_ERROR, msg, args);
      csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
  }

  void csoundLongJmp(CSOUND *csound, int retval)
  {
      int   n = CSOUND_EXITJMP_SUCCESS;

      n = (retval < 0 ? n + retval : n - retval) & (CSOUND_EXITJMP_SUCCESS - 1);
      if (!n)
        n = CSOUND_EXITJMP_SUCCESS;

      csound->curip = NULL;
      csound->ids = NULL;
      csound->pds = NULL;
      csound->reinitflag = 0;
      csound->tieflag = 0;
      csound->perferrcnt += csound->inerrcnt;
      csound->inerrcnt = 0;
      csound->engineState |= CS_STATE_JMP;

      longjmp(csound->exitjmp, n);
  }

  PUBLIC void csoundSetMessageLevel(CSOUND *csound, int messageLevel)
  {
      csound->oparms_.msglevel = messageLevel;
  }

  PUBLIC int csoundGetMessageLevel(CSOUND *csound)
  {
      return csound->oparms_.msglevel;
  }

  PUBLIC void csoundKeyPress(CSOUND *csound, char c)
  {
      csound->inChar_ = (int) ((unsigned char) c);
  }

  /*
   * CONTROL AND EVENTS
   */

  PUBLIC void csoundSetInputValueCallback(CSOUND *csound,
                                          void (*inputValueCalback)(CSOUND *csound,
                                                                    const char *channelName,
                                                                    MYFLT *value))
  {
      csound->InputValueCallback_ = inputValueCalback;
  }

  PUBLIC void csoundSetOutputValueCallback(CSOUND *csound,
                                           void (*outputValueCalback)(CSOUND *csound,
                                                                      const char *channelName,
                                                                      MYFLT value))
  {
      csound->OutputValueCallback_ = outputValueCalback;
  }

  PUBLIC int csoundScoreEvent(CSOUND *csound, char type,
                              const MYFLT *pfields, long numFields)
  {
      EVTBLK  evt;
      int     i;

      evt.strarg = NULL;
      evt.opcod = type;
      evt.pcnt = (int16) numFields;
      for (i = 0; i < (int) numFields; i++) /* Could be memcpy */
        evt.p[i + 1] = pfields[i];
      //memcpy(&evt.p[1],pfields, numFields*sizeof(MYFLT));
      return insert_score_event_at_sample(csound, &evt, csound->icurTime);
      }


  /*
   *    REAL-TIME AUDIO
   */

  /* dummy functions for the case when no real-time audio module is available */

  static double *get_dummy_rtaudio_globals(CSOUND *csound)
  {
      double  *p;

      p = (double*) csound->QueryGlobalVariable(csound, "__rtaudio_null_state");
      if (p == NULL) {
        if (UNLIKELY(csound->CreateGlobalVariable(csound, "__rtaudio_null_state",
                                                  sizeof(double) * 4) != 0))
          csound->Die(csound, Str("rtdummy: failed to allocate globals"));
        csound->Message(csound, Str("rtaudio: dummy module enabled\n"));
        p = (double*) csound->QueryGlobalVariable(csound, "__rtaudio_null_state");
      }
      return p;
  }

  static void dummy_rtaudio_timer(CSOUND *csound, double *p)
  {
      double  timeWait;
      int     i;

      timeWait = p[0] - csoundGetRealTime(csound->csRtClock);
      i = (int) (timeWait * 1000.0 + 0.5);
      if (i > 0)
        csoundSleep((size_t) i);
  }

  static int playopen_dummy(CSOUND *csound, const csRtAudioParams *parm)
  {
      double  *p;
      char    *s;

      /* find out if the use of dummy real-time audio functions was requested, */
      /* or an unknown plugin name was specified; the latter case is an error */
      s = (char*) csoundQueryGlobalVariable(csound, "_RTAUDIO");
      if (s != NULL && !(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                         strcmp(s, "NULL") == 0)) {
        if (s[0] == '\0')
          csoundErrorMsg(csound,
                         Str(" *** error: rtaudio module set to empty string"));
        else {
          print_opcodedir_warning(csound);
          csoundErrorMsg(csound,
                         Str(" *** error: unknown rtaudio module: '%s'"), s);
        }
        return CSOUND_ERROR;
      }
      p = get_dummy_rtaudio_globals(csound);
      csound->rtPlay_userdata = (void*) p;
      p[0] = csound->GetRealTime(csound->csRtClock);
      p[1] = 1.0 / ((double) ((int) sizeof(MYFLT) * parm->nChannels)
                    * (double) parm->sampleRate);
      return CSOUND_SUCCESS;
  }

  static void rtplay_dummy(CSOUND *csound, const MYFLT *outBuf, int nbytes)
  {
      double  *p = (double*) csound->rtPlay_userdata;
      (void) outBuf;
      p[0] += ((double) nbytes * p[1]);
      dummy_rtaudio_timer(csound, p);
  }

  static int recopen_dummy(CSOUND *csound, const csRtAudioParams *parm)
  {
      double  *p;
      char    *s;

      /* find out if the use of dummy real-time audio functions was requested, */
      /* or an unknown plugin name was specified; the latter case is an error */
      s = (char*) csoundQueryGlobalVariable(csound, "_RTAUDIO");
      if (s != NULL && !(strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
                         strcmp(s, "NULL") == 0)) {
        if (s[0] == '\0')
          csoundErrorMsg(csound,
                         Str(" *** error: rtaudio module set to empty string"));
        else {
          print_opcodedir_warning(csound);
          csoundErrorMsg(csound,
                         Str(" *** error: unknown rtaudio module: '%s'"), s);
        }
        return CSOUND_ERROR;
      }
      p = (double*) get_dummy_rtaudio_globals(csound) + 2;
      csound->rtRecord_userdata = (void*) p;
      p[0] = csound->GetRealTime(csound->csRtClock);
      p[1] = 1.0 / ((double) ((int) sizeof(MYFLT) * parm->nChannels)
                    * (double) parm->sampleRate);
      return CSOUND_SUCCESS;
  }

  static int rtrecord_dummy(CSOUND *csound, MYFLT *inBuf, int nbytes)
  {
      double  *p = (double*) csound->rtRecord_userdata;

      /* for (i = 0; i < (nbytes / (int) sizeof(MYFLT)); i++) */
      /*   ((MYFLT*) inBuf)[i] = FL(0.0); */
      memset(inBuf, 0, nbytes);

      p[0] += ((double) nbytes * p[1]);
      dummy_rtaudio_timer(csound, p);

      return nbytes;
  }

  static void rtclose_dummy(CSOUND *csound)
  {
      csound->rtPlay_userdata = NULL;
      csound->rtRecord_userdata = NULL;
  }

  PUBLIC void csoundSetPlayopenCallback(CSOUND *csound,
                                        int (*playopen__)(CSOUND *,
                                                          const csRtAudioParams
                                                          *parm))
  {
      csound->playopen_callback = playopen__;
  }

  PUBLIC void csoundSetRtplayCallback(CSOUND *csound,
                                      void (*rtplay__)(CSOUND *,
                                                       const MYFLT *outBuf,
                                                       int nbytes))
  {
      csound->rtplay_callback = rtplay__;
  }

  PUBLIC void csoundSetRecopenCallback(CSOUND *csound,
                                       int (*recopen__)(CSOUND *,
                                                        const csRtAudioParams *parm))
  {
      csound->recopen_callback = recopen__;
  }

  PUBLIC void csoundSetRtrecordCallback(CSOUND *csound,
                                        int (*rtrecord__)(CSOUND *,
                                                          MYFLT *inBuf,
                                                          int nbytes))
  {
      csound->rtrecord_callback = rtrecord__;
  }

  PUBLIC void csoundSetRtcloseCallback(CSOUND *csound,
                                       void (*rtclose__)(CSOUND *))
  {
      csound->rtclose_callback = rtclose__;
  }

  /* dummy real time MIDI functions */

  static int DummyMidiInOpen(CSOUND *csound, void **userData,
                             const char *devName)
  {
      char *s;

      (void) devName;
      *userData = NULL;
      s = (char*) csoundQueryGlobalVariable(csound, "_RTMIDI");
      if (s == NULL ||
          (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
           strcmp(s, "NULL") == 0)) {
        csoundMessage(csound, Str("WARNING: real time midi input disabled, "
                                  "using dummy functions\n"));
        return 0;
      }
      if (s[0] == '\0')
        csoundErrorMsg(csound, Str("error: -+rtmidi set to empty string"));
      else {
        print_opcodedir_warning(csound);
        csoundErrorMsg(csound, Str("error: -+rtmidi='%s': unknown module"), s);
      }
      return -1;
  }

  static int DummyMidiRead(CSOUND *csound, void *userData,
                           unsigned char *buf, int nbytes)
  {
      (void) csound;
      (void) userData;
      (void) buf;
      (void) nbytes;
      return 0;
  }

  static int DummyMidiOutOpen(CSOUND *csound, void **userData,
                              const char *devName)
  {
      char *s;

      (void) devName;
      *userData = NULL;
      s = (char*) csoundQueryGlobalVariable(csound, "_RTMIDI");
      if (s == NULL ||
          (strcmp(s, "null") == 0 || strcmp(s, "Null") == 0 ||
           strcmp(s, "NULL") == 0)) {
        csoundMessage(csound, Str("WARNING: real time midi output disabled, "
                                  "using dummy functions\n"));
        return 0;
      }
      if (s[0] == '\0')
        csoundErrorMsg(csound, Str("error: -+rtmidi set to empty string"));
      else {
        print_opcodedir_warning(csound);
        csoundErrorMsg(csound, Str("error: -+rtmidi='%s': unknown module"), s);
      }
      return -1;
  }

  static int DummyMidiWrite(CSOUND *csound, void *userData,
                            const unsigned char *buf, int nbytes)
  {
      (void) csound;
      (void) userData;
      (void) buf;
      return nbytes;
  }

  static const char *midi_err_msg = Str_noop("Unknown MIDI error");

  /**
   * Returns pointer to a string constant storing an error massage
   * for error code 'errcode'.
   */
  const char *csoundExternalMidiErrorString(CSOUND *csound, int errcode)
  {
      if (csound->midiGlobals->MidiErrorStringCallback == NULL)
        return midi_err_msg;
      return (csound->midiGlobals->MidiErrorStringCallback(errcode));
  }

  /* Set real time MIDI function pointers. */

  PUBLIC void csoundSetExternalMidiInOpenCallback(CSOUND *csound,
                                                  int (*func)(CSOUND *,
                                                              void **,
                                                              const char *))
  {
      csound->midiGlobals->MidiInOpenCallback = func;
  }

  PUBLIC void csoundSetExternalMidiReadCallback(CSOUND *csound,
                                                int (*func)(CSOUND *,
                                                            void *,
                                                            unsigned char *, int))
  {
      csound->midiGlobals->MidiReadCallback = func;
  }

  PUBLIC void csoundSetExternalMidiInCloseCallback(CSOUND *csound,
                                                   int (*func)(CSOUND *, void *))
  {
      csound->midiGlobals->MidiInCloseCallback = func;
  }

  PUBLIC void csoundSetExternalMidiOutOpenCallback(CSOUND *csound,
                                                   int (*func)(CSOUND *,
                                                               void **,
                                                               const char *))
  {
      csound->midiGlobals->MidiOutOpenCallback = func;
  }

  PUBLIC void csoundSetExternalMidiWriteCallback(CSOUND *csound,
                                                 int (*func)(CSOUND *,
                                                             void *,
                                                             const unsigned char *,
                                                             int))
  {
      csound->midiGlobals->MidiWriteCallback = func;
  }

  PUBLIC void csoundSetExternalMidiOutCloseCallback(CSOUND *csound,
                                                    int (*func)(CSOUND *, void *))
  {
      csound->midiGlobals->MidiOutCloseCallback = func;
  }

  PUBLIC void csoundSetExternalMidiErrorStringCallback(CSOUND *csound,
                                                       const char *(*func)(int))
  {
      csound->midiGlobals->MidiErrorStringCallback = func;
  }

  /*
   *    FUNCTION TABLE DISPLAY.
   */

  PUBLIC int csoundSetIsGraphable(CSOUND *csound, int isGraphable)
  {
      int prv = csound->isGraphable_;
      csound->isGraphable_ = isGraphable;
      return prv;
  }

  PUBLIC void csoundSetMakeGraphCallback(CSOUND *csound,
                                         void (*makeGraphCB)(CSOUND *csound,
                                                             WINDAT *windat,
                                                             const char *name))
  {
      csound->csoundMakeGraphCallback_ = makeGraphCB;
  }

  PUBLIC void csoundSetDrawGraphCallback(CSOUND *csound,
                                         void (*drawGraphCallback)(CSOUND *csound,
                                                                   WINDAT *windat))
  {
      csound->csoundDrawGraphCallback_ = drawGraphCallback;
  }

  PUBLIC void csoundSetKillGraphCallback(CSOUND *csound,
                                         void (*killGraphCallback)(CSOUND *csound,
                                                                   WINDAT *windat))
  {
      csound->csoundKillGraphCallback_ = killGraphCallback;
  }

  static int defaultCsoundExitGraph(CSOUND *csound)
  {
      (void) csound;
      return CSOUND_SUCCESS;
  }

  PUBLIC void csoundSetExitGraphCallback(CSOUND *csound,
                                         int (*exitGraphCallback)(CSOUND *))
  {
      csound->csoundExitGraphCallback_ = exitGraphCallback;
  }

  static void defaultCsoundMakeXYin(CSOUND *csound,
                                    XYINDAT *xyindat, MYFLT x, MYFLT y)
  {
      (void) x;
      (void) y;
      memset(xyindat, 0, sizeof(XYINDAT));
      csoundWarning(csound,
                    Str("xyin not supported. use invalue opcode instead."));
  }

  static void defaultCsoundReadKillXYin(CSOUND *csound, XYINDAT *xyindat)
  {
      (void) csound;
      (void) xyindat;
  }

  PUBLIC void csoundSetMakeXYinCallback(CSOUND *csound,
                                        void (*makeXYinCallback)(CSOUND *,
                                                                 XYINDAT *xyindat,
                                                                 MYFLT x,
                                                                 MYFLT y))
  {
      csound->csoundMakeXYinCallback_ = makeXYinCallback;
  }

  PUBLIC void csoundSetReadXYinCallback(CSOUND *csound,
                                        void (*readXYinCallback)(CSOUND *,
                                                                 XYINDAT *xyindat))
  {
      csound->csoundReadXYinCallback_ = readXYinCallback;
  }

  PUBLIC void csoundSetKillXYinCallback(CSOUND *csound,
                                        void (*killXYinCallback)(CSOUND *,
                                                                 XYINDAT *xyindat))
  {
      csound->csoundKillXYinCallback_ = killXYinCallback;
  }

  /*
   * OPCODES
   */

  static CS_NOINLINE int opcode_list_new_oentry(CSOUND *csound,
                                                const OENTRY *ep)
  {
      int     oldCnt = 0;
      int     h = 0;

      if (ep->opname == NULL)
        return CSOUND_ERROR;
      if (ep->opname[0] != (char) 0)
        h = (int) name_hash_2(csound, ep->opname);
      else if (csound->opcodlst != NULL)
        return CSOUND_ERROR;
      if (csound->opcodlst != NULL) {
        int   n;
        oldCnt = (int) ((OENTRY*) csound->oplstend - (OENTRY*) csound->opcodlst);
        /* check if this opcode is already defined */
        n = csound->opcode_list[h];
        while (n) {
          if (!sCmp(csound->opcodlst[n].opname, ep->opname)) {
            int tmp = csound->opcodlst[n].prvnum;
            /* redefine existing opcode */
            memcpy(&(csound->opcodlst[n]), ep, sizeof(OENTRY));
            csound->opcodlst[n].useropinfo = NULL;
            csound->opcodlst[n].prvnum = tmp;
            return CSOUND_SUCCESS;
          }
          n = csound->opcodlst[n].prvnum;
        }
      }
      if (!(oldCnt & 0x7F)) {
        OENTRY  *newList;
        size_t  nBytes = (size_t) (oldCnt + 0x80) * sizeof(OENTRY);
        if (!oldCnt)
          newList = (OENTRY*) malloc(nBytes);
        else
          newList = (OENTRY*) realloc(csound->opcodlst, nBytes);
        if (newList == NULL)
          return CSOUND_MEMORY;
        csound->opcodlst = newList;
        csound->oplstend = ((OENTRY*) newList + (int) oldCnt);
        memset(&(csound->opcodlst[oldCnt]), 0, sizeof(OENTRY) * 0x80);
      }
      memcpy(&(csound->opcodlst[oldCnt]), ep, sizeof(OENTRY));
      csound->opcodlst[oldCnt].useropinfo = NULL;
      csound->opcodlst[oldCnt].prvnum = csound->opcode_list[h];
      csound->opcode_list[h] = oldCnt;
      csound->oplstend = (OENTRY*) csound->oplstend + (int) 1;

      return 0;
  }

  PUBLIC int csoundAppendOpcode(CSOUND *csound,
                                const char *opname, int dsblksiz, int thread,
                                const char *outypes, const char *intypes,
                                int (*iopadr)(CSOUND *, void *),
                                int (*kopadr)(CSOUND *, void *),
                                int (*aopadr)(CSOUND *, void *))
  {
      OENTRY  tmpEntry;
      int     err;

      tmpEntry.opname     = (char*) opname;
      tmpEntry.dsblksiz   = (uint16) dsblksiz;
      tmpEntry.thread     = (uint16) thread;
      tmpEntry.outypes    = (char*) outypes;
      tmpEntry.intypes    = (char*) intypes;
      tmpEntry.iopadr     = (SUBR) iopadr;
      tmpEntry.kopadr     = (SUBR) kopadr;
      tmpEntry.aopadr     = (SUBR) aopadr;
      err = opcode_list_new_oentry(csound, &tmpEntry);
      if (UNLIKELY(err))
        csoundErrorMsg(csound, Str("Failed to allocate new opcode entry."));

      return err;
  }

  /**
   * Appends a list of opcodes implemented by external software to Csound's
   * internal opcode list. The list should either be terminated with an entry
   * that has a NULL opname, or the number of entries (> 0) should be specified
   * in 'n'. Returns zero on success.
   */

  int csoundAppendOpcodes(CSOUND *csound, const OENTRY *opcodeList, int n)
  {
      OENTRY  *ep = (OENTRY*) opcodeList;
      int     err, retval = 0;

      if (UNLIKELY(opcodeList == NULL))
        return -1;
      if (UNLIKELY(n <= 0))
        n = 0x7FFFFFFF;
      while (n && ep->opname != NULL) {
        if (UNLIKELY((err = opcode_list_new_oentry(csound, ep)) != 0)) {
          csoundErrorMsg(csound, Str("Failed to allocate opcode entry for %s."),
                         ep->opname);
          retval = err;
        }
        n--, ep++;
      }
      return retval;
  }

  /*
   * MISC FUNCTIONS
   */

  int defaultCsoundYield(CSOUND *csound)
  {
      (void) csound;
      return 1;
  }

  PUBLIC void csoundSetYieldCallback(CSOUND *csound,
                                     int (*yieldCallback)(CSOUND *))
  {
      csound->csoundYieldCallback_ = yieldCallback;
  }

  void SetInternalYieldCallback(CSOUND *csound,
                                int (*yieldCallback)(CSOUND *))
  {
      csound->csoundInternalYieldCallback_ = yieldCallback;
  }

  int csoundYield(CSOUND *csound)
  {
      if (exitNow_)
        csound->LongJmp(csound, CSOUND_SIGNAL);
      csound->csoundInternalYieldCallback_(csound);
      return csound->csoundYieldCallback_(csound);
  }

  extern void csoundDeleteAllGlobalVariables(CSOUND *csound);

  typedef struct resetCallback_s {
    void    *userData;
    int     (*func)(CSOUND *, void *);
    struct resetCallback_s  *nxt;
  } resetCallback_t;

  extern void cscoreRESET(CSOUND *);
  extern void tranRESET(CSOUND *);
  extern void memRESET(CSOUND *);

  PUBLIC void csoundReset(CSOUND *csound)
  {
      CSOUND    *saved_env;
      void      *p1, *p2;
      uintptr_t length;
     
      csoundCleanup(csound);
 
      /* call registered reset callbacks */
      while (csound->reset_list != NULL) {
        resetCallback_t *p = (resetCallback_t*) csound->reset_list;
        p->func(csound, p->userData);
        csound->reset_list = (void*) p->nxt;
        free(p);
      }
      /* call local destructor routines of external modules */
      /* should check return value... */
      csoundDestroyModules(csound);
 
      /* IV - Feb 01 2005: clean up configuration variables and */
      /* named dynamic "global" variables of Csound instance */
      csoundDeleteAllConfigurationVariables(csound);
      csoundDeleteAllGlobalVariables(csound);
 
#ifdef CSCORE
      cscoreRESET(csound);
#endif
      tranRESET(csound);

      csound->oparms_.odebug = 0;
      /* RWD 9:2000 not terribly vital, but good to do this somewhere... */
      pvsys_release(csound);
      close_all_files(csound);
      /* delete temporary files created by this Csound instance */
      remove_tmpfiles(csound);
      rlsmemfiles(csound);
      memRESET(csound);
      /**
       * Copy everything EXCEPT the function pointers.
       * We do it by saving them and copying them back again...
       */
      /* hope that this does not fail... */
      saved_env = (CSOUND*) malloc(sizeof(CSOUND));
      memcpy(saved_env, csound, sizeof(CSOUND));
      memcpy(csound, &cenviron_, sizeof(CSOUND));
      length = (uintptr_t) &(csound->ids) - (uintptr_t) csound;
      memcpy((void*) csound, (void*) saved_env, (size_t) length);
      csound->oparms = &(csound->oparms_);
      csound->hostdata = saved_env->hostdata;
      p1 = (void*) &(csound->first_callback_);
      p2 = (void*) &(csound->last_callback_);
      length = (uintptr_t) p2 - (uintptr_t) p1;
      memcpy(p1, (void*) &(saved_env->first_callback_), (size_t) length);
      csound->csoundCallbacks_ = saved_env->csoundCallbacks_;
      memcpy(&(csound->exitjmp), &(saved_env->exitjmp), sizeof(jmp_buf));
      csound->memalloc_db = saved_env->memalloc_db;
      free(saved_env);
   
  }

  PUBLIC int csoundGetDebug(CSOUND *csound)
  {
      return csound->oparms_.odebug;
  }

  PUBLIC void csoundSetDebug(CSOUND *csound, int debug)
  {
      csound->oparms_.odebug = debug;
  }

  PUBLIC int csoundTableLength(CSOUND *csound, int table)
  {
      MYFLT *tablePtr;
      return csound->GetTable(csound, &tablePtr, table);
  }

  PUBLIC MYFLT csoundTableGet(CSOUND *csound, int table, int index)
  {
      return csound->flist[table]->ftable[index];
  }

  PUBLIC void csoundTableSet(CSOUND *csound, int table, int index, MYFLT value)
  {
      csound->flist[table]->ftable[index] = value;
  }

  static int csoundDoCallback_(CSOUND *csound, void *p, unsigned int type)
  {
      if (csound->csoundCallbacks_ != NULL) {
        CsoundCallbackEntry_t *pp;
        pp = (CsoundCallbackEntry_t*) csound->csoundCallbacks_;
        do {
          if (pp->typeMask & type) {
            int   retval = pp->func(pp->userData, p, type);
            if (retval <= 0)
              return retval;
          }
          pp = pp->nxt;
        } while (pp != (CsoundCallbackEntry_t*) NULL);
      }
      return 1;
  }

  /**
   * Sets general purpose callback function that will be called on various
   * events. The callback is preserved on csoundReset(), and multiple
   * callbacks may be set and will be called in reverse order of
   * registration. If the same function is set again, it is only moved
   * in the list of callbacks so that it will be called first, and the
   * user data and type mask parameters are updated. 'typeMask' can be the
   * bitwise OR of callback types for which the function should be called,
   * or zero for all types.
   * Returns zero on success, CSOUND_ERROR if the specified function
   * pointer or type mask is invalid, and CSOUND_MEMORY if there is not
   * enough memory.
   *
   * The callback function takes the following arguments:
   *   void *userData
   *     the "user data" pointer, as specified when setting the callback
   *   void *p
   *     data pointer, depending on the callback type
   *   unsigned int type
   *     callback type, can be one of the following (more may be added in
   *     future versions of Csound):
   *       CSOUND_CALLBACK_KBD_EVENT
   *       CSOUND_CALLBACK_KBD_TEXT
   *         called by the sensekey opcode to fetch key codes. The data
   *         pointer is a pointer to a single value of type 'int', for
   *         returning the key code, which can be in the range 1 to 65535,
   *         or 0 if there is no keyboard event.
   *         For CSOUND_CALLBACK_KBD_EVENT, both key press and release
   *         events should be returned (with 65536 (0x10000) added to the
   *         key code in the latter case) as unshifted ASCII codes.
   *         CSOUND_CALLBACK_KBD_TEXT expects key press events only as the
   *         actual text that is typed.
   * The return value should be zero on success, negative on error, and
   * positive if the callback was ignored (for example because the type is
   * not known).
   */

  PUBLIC int csoundSetCallback(CSOUND *csound,
                               int (*func)(void *userData, void *p,
                                           unsigned int type),
                               void *userData, unsigned int typeMask)
  {
      CsoundCallbackEntry_t *pp;

      if (UNLIKELY(func == (int (*)(void *, void *, unsigned int)) NULL ||
                   (typeMask
#ifndef PARCS
                    & (~(CSOUND_CALLBACK_KBD_EVENT | CSOUND_CALLBACK_KBD_TEXT)))
                   != 0U)) {
#else /* PARCS */
                    & (~(CSOUND_CALLBACK_KBD_EVENT | CSOUND_CALLBACK_KBD_TEXT))) != 0U))
#endif /* PARCS */
        return CSOUND_ERROR;
#ifndef PARCS
      }
#endif /* ! PARCS */
      csoundRemoveCallback(csound, func);
      pp = (CsoundCallbackEntry_t*) malloc(sizeof(CsoundCallbackEntry_t));
#ifndef PARCS
      if (UNLIKELY(pp == (CsoundCallbackEntry_t*) NULL)) {
#else /* PARCS */
      if (UNLIKELY(pp == (CsoundCallbackEntry_t*) NULL))
#endif /* PARCS */
        return CSOUND_MEMORY;
#ifndef PARCS
      }
#endif /* ! PARCS */
      pp->typeMask = (typeMask ? typeMask : 0xFFFFFFFFU);
      pp->nxt = (CsoundCallbackEntry_t*) csound->csoundCallbacks_;
      pp->userData = userData;
      pp->func = func;
      csound->csoundCallbacks_ = (void*) pp;

      return CSOUND_SUCCESS;
  }

  /**
   * Removes a callback previously set with csoundSetCallback().
   */

  PUBLIC void csoundRemoveCallback(CSOUND *csound,
                                   int (*func)(void *, void *, unsigned int))
  {
      CsoundCallbackEntry_t *pp, *prv;

      pp = (CsoundCallbackEntry_t*) csound->csoundCallbacks_;
      prv = (CsoundCallbackEntry_t*) NULL;
      while (pp != (CsoundCallbackEntry_t*) NULL) {
        if (pp->func == func) {
#ifndef PARCS
          if (prv != (CsoundCallbackEntry_t*) NULL) {
#else /* PARCS */
          if (prv != (CsoundCallbackEntry_t*) NULL)
#endif /* PARCS */
            prv->nxt = pp->nxt;
#ifndef PARCS
          } else {
#else /* PARCS */
          else
#endif /* PARCS */
            csound->csoundCallbacks_ = (void*) pp->nxt;
#ifndef PARCS
          }
#endif /* ! PARCS */
          free((void*) pp);
          return;
        }
        prv = pp;
        pp = pp->nxt;
      }
  }

  PUBLIC void csoundSetFileOpenCallback(CSOUND *p,
#ifndef PARCS
                                        void (*fileOpenCallback)(CSOUND*,
                                                                 const char*,
                                                                 int, int, int))
#else /* PARCS */
                                        void (*fileOpenCallback)(CSOUND*, const char*, int, int, int))
#endif /* PARCS */
  {
      p->FileOpenCallback_ = fileOpenCallback;
  }

  /* csoundNotifyFileOpened() should be called by plugins via
     csound->NotifyFileOpened() to let Csound know that they opened a file
     without using one of the standard mechanisms (csound->FileOpen2() or
     ldmemfile2()).  The notification is passed on to the host if it has set
     the FileOpen callback. */
  void csoundNotifyFileOpened(CSOUND* csound, const char* pathname,
                              int csFileType, int writing, int temporary)
  {
      if (csound->FileOpenCallback_ != NULL)
        csound->FileOpenCallback_(csound, pathname, csFileType, writing,
                                  temporary);
      return;

  }

  /* -------- IV - Jan 27 2005: timer functions -------- */

#ifdef HAVE_GETTIMEOFDAY
#undef HAVE_GETTIMEOFDAY
#endif
#if defined(LINUX) || defined(__unix) || defined(__unix__) || defined(__MACH__)
#define HAVE_GETTIMEOFDAY 1
#include <sys/time.h>
#endif

  /* enable use of high resolution timer (Linux/i586/GCC only) */
  /* could in fact work under any x86/GCC system, but do not   */
  /* know how to query the actual CPU frequency ...            */

#define HAVE_RDTSC  1

  /* ------------------------------------ */

#if defined(HAVE_RDTSC)
#if !(defined(LINUX) && defined(__GNUC__) && defined(__i386__))
#undef HAVE_RDTSC
#endif
#endif

  /* hopefully cannot change during performance */
  static double timeResolutionSeconds = -1.0;

  /* find out CPU frequency based on /proc/cpuinfo */

  static int getTimeResolution(void)
  {
#if defined(HAVE_RDTSC)
      FILE    *f;
      char    buf[256];

      /* if frequency is not known yet */
      f = fopen("/proc/cpuinfo", "r");
      if (UNLIKELY(f == NULL)) {
        fprintf(stderr, Str("Cannot open /proc/cpuinfo. "
                            "Support for RDTSC is not available.\n"));
        return -1;
      }
      /* find CPU frequency */
      while (fgets(buf, 256, f) != NULL) {
        int     i;
        char    *s = (char*) buf - 1;

        buf[255] = '\0';          /* safety */
#ifndef PARCS
        if (strlen(buf) < 9) {
          continue;    /* too short, skip */
        }
#else /* PARCS */
        if (strlen(buf) < 9)
          continue;                       /* too short, skip */
#endif /* PARCS */
        while (*++s != '\0')
#ifndef PARCS
          if (isupper(*s)) {
            *s = tolower(*s);    /* convert to lower case */
          }
        if (strncmp(buf, "cpu mhz", 7) != 0) {
          continue;    /* check key name */
        }
#else /* PARCS */
          if (isupper(*s))
            *s = tolower(*s);             /* convert to lower case */
        if (strncmp(buf, "cpu mhz", 7) != 0)
          continue;                       /* check key name */
#endif /* PARCS */
        s = strchr(buf, ':');             /* find frequency value */
#ifndef PARCS
        if (s == NULL) {
          continue;    /* invalid entry */
        }
#else /* PARCS */
        if (s == NULL) continue;              /* invalid entry */
#endif /* PARCS */
        do {
          s++;
        } while (*s == ' ' || *s == '\t');    /* skip white space */
        i = sscanf(s, "%lf", &timeResolutionSeconds);
        if (i < 1 || timeResolutionSeconds < 1.0) {
          timeResolutionSeconds = -1.0;       /* invalid entry */
          continue;
        }
      }
      fclose(f);
      if (UNLIKELY(timeResolutionSeconds <= 0.0)) {
        fprintf(stderr, Str("No valid CPU frequency entry "
                            "was found in /proc/cpuinfo.\n"));
        return -1;
      }
      /* MHz -> seconds */
      timeResolutionSeconds = 0.000001 / timeResolutionSeconds;
#elif defined(WIN32)
      LARGE_INTEGER tmp1;
      int_least64_t tmp2;
      QueryPerformanceFrequency(&tmp1);
      tmp2 = (int_least64_t) tmp1.LowPart + ((int_least64_t) tmp1.HighPart << 32);
      timeResolutionSeconds = 1.0 / (double) tmp2;
#elif defined(HAVE_GETTIMEOFDAY)
      timeResolutionSeconds = 0.000001;
#else
      timeResolutionSeconds = 1.0;
#endif
#ifdef BETA
      fprintf(stderr, "time resolution is %.3f ns\n",
              1.0e9 * timeResolutionSeconds);
#endif
      return 0;
  }

  /* function for getting real time */

  static inline int_least64_t get_real_time(void)
  {
#if defined(HAVE_RDTSC)
      /* optimised high resolution timer for Linux/i586/GCC only */
      uint32_t  l, h;
#ifndef __STRICT_ANSI__
      asm volatile ("rdtsc" : "=a" (l), "=d" (h));
#else
      __asm__ volatile ("rdtsc" : "=a" (l), "=d" (h));
#endif
      return ((int_least64_t) l + ((int_least64_t) h << 32));
#elif defined(WIN32)
      /* Win32: use QueryPerformanceCounter - resolution depends on system, */
      /* but is expected to be better than 1 us. GetSystemTimeAsFileTime    */
      /* seems to have much worse resolution under Win95.                   */
      LARGE_INTEGER tmp;
      QueryPerformanceCounter(&tmp);
#ifndef PARCS
      return ((int_least64_t) tmp.LowPart + ((int_least64_t) tmp.HighPart << 32));
#else /* PARCS */
      return ((int_least64_t) tmp.LowPart + ((int_least64_t) tmp.HighPart <<32));
#endif /* PARCS */
#elif defined(HAVE_GETTIMEOFDAY)
      /* UNIX: use gettimeofday() - allows 1 us resolution */
      struct timeval tv;
      gettimeofday(&tv, NULL);
      return ((int_least64_t) tv.tv_usec
              + (int_least64_t) ((uint32_t) tv.tv_sec * (uint64_t) 1000000));
#else
      /* other systems: use time() - allows 1 second resolution */
      return ((int_least64_t) time(NULL));
#endif
  }

  /* function for getting CPU time */

  static inline int_least64_t get_CPU_time(void)
  {
      return ((int_least64_t) ((uint32_t) clock()));
  }

  /* initialise a timer structure */

  PUBLIC void csoundInitTimerStruct(RTCLOCK *p)
  {
      p->starttime_real = get_real_time();
      p->starttime_CPU = get_CPU_time();
  }

  /**
   * return the elapsed real time (in seconds) since the specified timer
   * structure was initialised
   */
  PUBLIC double csoundGetRealTime(RTCLOCK *p)
  {
      return ((double) (get_real_time() - p->starttime_real)
              * (double) timeResolutionSeconds);
  }

  /**
   * return the elapsed CPU time (in seconds) since the specified timer
   * structure was initialised
   */
  PUBLIC double csoundGetCPUTime(RTCLOCK *p)
  {
      return ((double) ((uint32_t) get_CPU_time() - (uint32_t) p->starttime_CPU)
              * (1.0 / (double) CLOCKS_PER_SEC));
  }

  /* return a 32-bit unsigned integer to be used as seed from current time */

  PUBLIC uint32_t csoundGetRandomSeedFromTime(void)
  {
      return (uint32_t) get_real_time();
  }

  /**
   * Return the size of MYFLT in bytes.
   */
  PUBLIC int csoundGetSizeOfMYFLT(void)
  {
      return (int) sizeof(MYFLT);
  }

  /**
   * Return pointer to user data pointer for real time audio input.
   */
  PUBLIC void **csoundGetRtRecordUserData(CSOUND *csound)
  {
      return &(csound->rtRecord_userdata);
  }

  /**
   * Return pointer to user data pointer for real time audio output.
   */
  PUBLIC void **csoundGetRtPlayUserData(CSOUND *csound)
  {
      return &(csound->rtPlay_userdata);
  }

  typedef struct opcodeDeinit_s {
    void    *p;
    int     (*func)(CSOUND *, void *);
    void    *nxt;
  } opcodeDeinit_t;

  /**
   * Register a function to be called at note deactivation.
   * Should be called from the initialisation routine of an opcode.
   * 'p' is a pointer to the OPDS structure of the opcode, and 'func'
   * is the function to be called, with the same arguments and return
   * value as in the case of opcode init/perf functions.
   * The functions are called in reverse order of registration.
   * Returns zero on success.
   */

  int csoundRegisterDeinitCallback(CSOUND *csound, void *p,
                                   int (*func)(CSOUND *, void *))
  {
      INSDS           *ip = ((OPDS*) p)->insdshead;
      opcodeDeinit_t  *dp = (opcodeDeinit_t*) malloc(sizeof(opcodeDeinit_t));

      (void) csound;
#ifndef PARCS
      if (UNLIKELY(dp == NULL)) {
#else /* PARCS */
      if (UNLIKELY(dp == NULL))
#endif /* PARCS */
        return CSOUND_MEMORY;
#ifndef PARCS
      }
#endif /* ! PARCS */
      dp->p = p;
      dp->func = func;
      dp->nxt = ip->nxtd;
      ip->nxtd = dp;
      return CSOUND_SUCCESS;
  }

  /**
   * Register a function to be called by csoundReset(), in reverse order
   * of registration, before unloading external modules. The function takes
   * the Csound instance pointer as the first argument, and the pointer
   * passed here as 'userData' as the second, and is expected to return zero
   * on success.
   * The return value of csoundRegisterResetCallback() is zero on success.
   */

  int csoundRegisterResetCallback(CSOUND *csound, void *userData,
                                  int (*func)(CSOUND *, void *))
  {
      resetCallback_t *dp = (resetCallback_t*) malloc(sizeof(resetCallback_t));

#ifndef PARCS
      if (UNLIKELY(dp == NULL)) {
#else /* PARCS */
      if (UNLIKELY(dp == NULL))
#endif /* PARCS */
        return CSOUND_MEMORY;
#ifndef PARCS
      }
#endif /* ! PARCS */
      dp->userData = userData;
      dp->func = func;
      dp->nxt = csound->reset_list;
      csound->reset_list = (void*) dp;
      return CSOUND_SUCCESS;
  }

  /* call the opcode deinitialisation routines of an instrument instance */
  /* called from deact() in insert.c */

  int csoundDeinitialiseOpcodes(CSOUND *csound, INSDS *ip)
  {
      int err = 0;

      while (ip->nxtd != NULL) {
        opcodeDeinit_t  *dp = (opcodeDeinit_t*) ip->nxtd;
        err |= dp->func(csound, dp->p);
        ip->nxtd = (void*) dp->nxt;
        free(dp);
      }
      return err;
  }

  /**
   * Returns the name of the opcode of which the data structure
   * is pointed to by 'p'.
   */
  char *csoundGetOpcodeName(void *p)
  {
      CSOUND *csound = (CSOUND*) ((OPDS*) p)->insdshead->csound;
      return (char*) csound->opcodlst[((OPDS*) p)->optext->t.opnum].opname;
  }

  /**
   * Returns the number of input arguments for opcode 'p'.
   */
  int csoundGetInputArgCnt(void *p)
  {
      return (int) ((OPDS*) p)->optext->t.inoffs->count;
  }

  /**
   * Returns a binary value of which bit 0 is set if the first input
   * argument is a-rate, bit 1 is set if the second input argument is
   * a-rate, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetInputArgAMask(void *p)
  {
      return (unsigned long) ((unsigned int) ((OPDS*) p)->optext->t.xincod);
  }

  /**
   * Returns a binary value of which bit 0 is set if the first input
   * argument is a string, bit 1 is set if the second input argument is
   * a string, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetInputArgSMask(void *p)
  {
      return (unsigned long) ((unsigned int) ((OPDS*) p)->optext->t.xincod_str);
  }

  /**
   * Returns the name of input argument 'n' (counting from 0) for opcode 'p'.
   */
  char *csoundGetInputArgName(void *p, int n)
  {
#ifndef PARCS
      if ((unsigned int) n >=
          (unsigned int) ((OPDS*) p)->optext->t.inoffs->count) {
#else /* PARCS */
      if ((unsigned int) n >= (unsigned int) ((OPDS*) p)->optext->t.inoffs->count)
#endif /* PARCS */
        return (char*) NULL;
#ifndef PARCS
      }
#endif /* ! PARCS */
      return (char*) ((OPDS*) p)->optext->t.inlist->arg[n];
  }

  /**
   * Returns the number of output arguments for opcode 'p'.
   */
  int csoundGetOutputArgCnt(void *p)
  {
      return (int) ((OPDS*) p)->optext->t.outoffs->count;
  }

  /**
   * Returns a binary value of which bit 0 is set if the first output
   * argument is a-rate, bit 1 is set if the second output argument is
   * a-rate, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetOutputArgAMask(void *p)
  {
      return (unsigned long) ((unsigned int) ((OPDS*) p)->optext->t.xoutcod);
  }

  /**
   * Returns a binary value of which bit 0 is set if the first output
   * argument is a string, bit 1 is set if the second output argument is
   * a string, and so on.
   * Only the first 31 arguments are guaranteed to be reported correctly.
   */
  unsigned long csoundGetOutputArgSMask(void *p)
  {
      return (unsigned long) ((unsigned int) ((OPDS*) p)->optext->t.xoutcod_str);
  }

  /**
   * Returns the name of output argument 'n' (counting from 0) for opcode 'p'.
   */
  char *csoundGetOutputArgName(void *p, int n)
  {
      if ((unsigned int) n
#ifndef PARCS
          >= (unsigned int) ((OPDS*) p)->optext->t.outoffs->count) {
#else /* PARCS */
          >= (unsigned int) ((OPDS*) p)->optext->t.outoffs->count)
#endif /* PARCS */
        return (char*) NULL;
#ifndef PARCS
      }
#endif /* ! PARCS */
      return (char*) ((OPDS*) p)->optext->t.outlist->arg[n];
  }

  /**
   * Set release time in control periods (1 / csound->ekr second units)
   * for opcode 'p' to 'n'. If the current release time is longer than
   * the specified value, it is not changed.
   * Returns the new release time.
   */
  int csoundSetReleaseLength(void *p, int n)
  {
#ifndef PARCS
      if (n > (int) ((OPDS*) p)->insdshead->xtratim) {
#else /* PARCS */
      if (n > (int) ((OPDS*) p)->insdshead->xtratim)
#endif /* PARCS */
        ((OPDS*) p)->insdshead->xtratim = n;
#ifndef PARCS
      }
#endif /* ! PARCS */
      return (int) ((OPDS*) p)->insdshead->xtratim;
  }

  /**
   * Set release time in seconds for opcode 'p' to 'n'.
   * If the current release time is longer than the specified value,
   * it is not changed.
   * Returns the new release time in seconds.
   */
  MYFLT csoundSetReleaseLengthSeconds(void *p, MYFLT n)
  {
      int kcnt = (int) (n * ((OPDS*) p)->insdshead->csound->ekr + FL(0.5));
#ifndef PARCS
      if (kcnt > (int) ((OPDS*) p)->insdshead->xtratim) {
#else /* PARCS */
      if (kcnt > (int) ((OPDS*) p)->insdshead->xtratim)
#endif /* PARCS */
        ((OPDS*) p)->insdshead->xtratim = kcnt;
#ifndef PARCS
      }
#endif /* ! PARCS */
      return ((MYFLT) ((OPDS*) p)->insdshead->xtratim
              * ((OPDS*) p)->insdshead->csound->onedkr);
  }

  /**
   * Returns MIDI channel number (0 to 15) for the instrument instance
   * that called opcode 'p'.
   * In the case of score notes, -1 is returned.
   */
  int csoundGetMidiChannelNumber(void *p)
  {
      MCHNBLK *chn = ((OPDS*) p)->insdshead->m_chnbp;
      int     i;
#ifndef PARCS
      if (chn == NULL) {
#else /* PARCS */
      if (chn == NULL)
#endif /* PARCS */
        return -1;
#ifndef PARCS
      }
#endif /* ! PARCS */
      for (i = 0; i < 16; i++) {
#ifndef PARCS
        if (chn == ((OPDS*) p)->insdshead->csound->m_chnbp[i]) {
#else /* PARCS */
        if (chn == ((OPDS*) p)->insdshead->csound->m_chnbp[i])
#endif /* PARCS */
          return i;
#ifndef PARCS
        }
#endif /* ! PARCS */
      }
      return -1;
  }

  /**
   * Returns a pointer to the MIDI channel structure for the instrument
   * instance that called opcode 'p'.
   * In the case of score notes, NULL is returned.
   */
  MCHNBLK *csoundGetMidiChannel(void *p)
  {
      return ((OPDS*) p)->insdshead->m_chnbp;
  }

  /**
   * Returns MIDI note number (in the range 0 to 127) for opcode 'p'.
   * If the opcode was not called from a MIDI activated instrument
   * instance, the return value is undefined.
   */
  int csoundGetMidiNoteNumber(void *p)
  {
      return (int) ((OPDS*) p)->insdshead->m_pitch;
  }

  /**
   * Returns MIDI velocity (in the range 0 to 127) for opcode 'p'.
   * If the opcode was not called from a MIDI activated instrument
   * instance, the return value is undefined.
   */
  int csoundGetMidiVelocity(void *p)
  {
      return (int) ((OPDS*) p)->insdshead->m_veloc;
  }

  /**
   * Returns non-zero if the current note (owning opcode 'p') is releasing.
   */
  int csoundGetReleaseFlag(void *p)
  {
      return (int) ((OPDS*) p)->insdshead->relesing;
  }

  /**
   * Returns the note-off time in seconds (measured from the beginning of
   * performance) of the current instrument instance, from which opcode 'p'
   * was called. The return value may be negative if the note has indefinite
   * duration.
   */
  double csoundGetOffTime(void *p)
  {
      return (double) ((OPDS*) p)->insdshead->offtim;
  }

  /**
   * Returns the array of p-fields passed to the instrument instance
   * that owns opcode 'p', starting from p0. Only p1, p2, and p3 are
   * guaranteed to be available. p2 is measured in seconds from the
   * beginning of the current section.
   */
  MYFLT *csoundGetPFields(void *p)
  {
      return (MYFLT*) &(((OPDS*) p)->insdshead->p0);
  }

  /**
   * Returns the instrument number (p1) for opcode 'p'.
   */
  int csoundGetInstrumentNumber(void *p)
  {
      return (int) ((OPDS*) p)->insdshead->p1;
  }

  typedef struct csMsgStruct_ {
    struct csMsgStruct_  *nxt;
    int         attr;
    char        s[1];
  } csMsgStruct;

  typedef struct csMsgBuffer_ {
    void        *mutex_;
    csMsgStruct *firstMsg;
    csMsgStruct *lastMsg;
    int         msgCnt;
    char        *buf;
  } csMsgBuffer;

  // callback for storing messages in the buffer only
  static void csoundMessageBufferCallback_1_(CSOUND *csound, int attr,
                                             const char *fmt, va_list args);

  // callback for writing messages to the buffer, and also stdout/stderr
  static void csoundMessageBufferCallback_2_(CSOUND *csound, int attr,
                                             const char *fmt, va_list args);

  /**
   * Creates a buffer for storing messages printed by Csound.
   * Should be called after creating a Csound instance; note that
   * the message buffer uses the host data pointer, and the buffer
   * should be freed by calling csoundDestroyMessageBuffer() before
   * deleting the Csound instance.
   * If 'toStdOut' is non-zero, the messages are also printed to
   * stdout and stderr (depending on the type of the message),
   * in addition to being stored in the buffer.
   */

  void PUBLIC csoundEnableMessageBuffer(CSOUND *csound, int toStdOut)
  {
      csMsgBuffer *pp;
      size_t      nBytes;

      csoundDestroyMessageBuffer(csound);
      nBytes = sizeof(csMsgBuffer);
#ifndef PARCS
      if (!toStdOut) {
#else /* PARCS */
      if (!toStdOut)
#endif /* PARCS */
        nBytes += (size_t) 16384;
#ifndef PARCS
      }
#endif /* ! PARCS */
      pp = (csMsgBuffer*) malloc(nBytes);
      pp->mutex_ = csoundCreateMutex(0);
      pp->firstMsg = (csMsgStruct*) 0;
      pp->lastMsg = (csMsgStruct*) 0;
      pp->msgCnt = 0;
      if (!toStdOut) {
        pp->buf = (char*) pp + (int) sizeof(csMsgBuffer);
        pp->buf[0] = (char) '\0';
#ifndef PARCS
      } else {
        pp->buf = (char*) 0;
#endif /* ! PARCS */
      }
#ifdef PARCS
      else
        pp->buf = (char*) 0;
#endif /* PARCS */
      csoundSetHostData(csound, (void*) pp);
#ifndef PARCS
      if (!toStdOut) {
#else /* PARCS */
      if (!toStdOut)
#endif /* PARCS */
        csoundSetMessageCallback(csound, csoundMessageBufferCallback_1_);
#ifndef PARCS
      } else {
#else /* PARCS */
      else
#endif /* PARCS */
        csoundSetMessageCallback(csound, csoundMessageBufferCallback_2_);
#ifndef PARCS
      }
#endif /* ! PARCS */
  }

  /**
   * Returns the first message from the buffer.
   */
#ifdef MSVC
  const char PUBLIC *csoundGetFirstMessage(CSOUND *csound)
#else
#ifndef PARCS
    const char * PUBLIC csoundGetFirstMessage(CSOUND *csound)
#else /* PARCS */
    const char *PUBLIC csoundGetFirstMessage(CSOUND *csound)
#endif /* PARCS */
#endif
  {
      csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
      char        *msg = NULL;

      if (pp && pp->msgCnt) {
        csoundLockMutex(pp->mutex_);
#ifndef PARCS
        if (pp->firstMsg) {
#else /* PARCS */
        if (pp->firstMsg)
#endif /* PARCS */
          msg = &(pp->firstMsg->s[0]);
#ifndef PARCS
        }
#endif /* ! PARCS */
        csoundUnlockMutex(pp->mutex_);
      }
      return msg;
  }

  /**
   * Returns the attribute parameter (see msg_attr.h) of the first message
   * in the buffer.
   */

  int PUBLIC csoundGetFirstMessageAttr(CSOUND *csound)
  {
      csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
      int         attr = 0;

      if (pp && pp->msgCnt) {
        csoundLockMutex(pp->mutex_);
#ifndef PARCS
        if (pp->firstMsg) {
#else /* PARCS */
        if (pp->firstMsg)
#endif /* PARCS */
          attr = pp->firstMsg->attr;
#ifndef PARCS
        }
#endif /* ! PARCS */
        csoundUnlockMutex(pp->mutex_);
      }
      return attr;
  }

  /**
   * Removes the first message from the buffer.
   */

  void PUBLIC csoundPopFirstMessage(CSOUND *csound)
  {
      csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);

      if (pp) {
        csMsgStruct *tmp;
        csoundLockMutex(pp->mutex_);
        tmp = pp->firstMsg;
        if (tmp) {
          pp->firstMsg = tmp->nxt;
          pp->msgCnt--;
#ifndef PARCS
          if (!pp->firstMsg) {
#else /* PARCS */
          if (!pp->firstMsg)
#endif /* PARCS */
            pp->lastMsg = (csMsgStruct*) 0;
#ifndef PARCS
          }
#endif /* ! PARCS */
        }
        csoundUnlockMutex(pp->mutex_);
#ifndef PARCS
        if (tmp) {
#else /* PARCS */
        if (tmp)
#endif /* PARCS */
          free((void*) tmp);
#ifndef PARCS
        }
#endif /* ! PARCS */
      }
  }

  /**
   * Returns the number of pending messages in the buffer.
   */

  int PUBLIC csoundGetMessageCnt(CSOUND *csound)
  {
      csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
      int         cnt = 0;

      if (pp) {
        csoundLockMutex(pp->mutex_);
        cnt = pp->msgCnt;
        csoundUnlockMutex(pp->mutex_);
      }
      return cnt;
  }

  /**
   * Releases all memory used by the message buffer.
   */

  void PUBLIC csoundDestroyMessageBuffer(CSOUND *csound)
  {
      csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);

#ifndef PARCS
      if (!pp) {
#else /* PARCS */
      if (!pp)
#endif /* PARCS */
        return;
#ifndef PARCS
      }
      while (csoundGetMessageCnt(csound) > 0) {
#else /* PARCS */
      while (csoundGetMessageCnt(csound) > 0)
#endif /* PARCS */
        csoundPopFirstMessage(csound);
#ifndef PARCS
      }
#endif /* ! PARCS */
      csoundSetHostData(csound, (void*) 0);
      csoundDestroyMutex(pp->mutex_);
      free((void*) pp);
  }

  static void csoundMessageBufferCallback_1_(CSOUND *csound, int attr,
                                             const char *fmt, va_list args)
  {
      csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
      csMsgStruct *p;
      int         len;

      csoundLockMutex(pp->mutex_);
      len = vsprintf(pp->buf, fmt, args);         // FIXME: this can overflow
      if (UNLIKELY((unsigned int) len >= (unsigned int) 16384)) {
        csoundUnlockMutex(pp->mutex_);
        fprintf(stderr, "csound: internal error: message buffer overflow\n");
        exit(-1);
      }
      p = (csMsgStruct*) malloc(sizeof(csMsgStruct) + (size_t) len);
      p->nxt = (csMsgStruct*) 0;
      p->attr = attr;
      strcpy(&(p->s[0]), pp->buf);
#ifndef PARCS
      if (pp->firstMsg == (csMsgStruct*) 0) {
#else /* PARCS */
      if (pp->firstMsg == (csMsgStruct*) 0)
#endif /* PARCS */
        pp->firstMsg = p;
#ifndef PARCS
      } else {
#else /* PARCS */
      else
#endif /* PARCS */
        pp->lastMsg->nxt = p;
#ifndef PARCS
      }
#endif /* ! PARCS */
      pp->lastMsg = p;
      pp->msgCnt++;
      csoundUnlockMutex(pp->mutex_);
  }

  static void csoundMessageBufferCallback_2_(CSOUND *csound, int attr,
                                             const char *fmt, va_list args)
  {
      csMsgBuffer *pp = (csMsgBuffer*) csoundGetHostData(csound);
      csMsgStruct *p;
      int         len = 0;

      switch (attr & CSOUNDMSG_TYPE_MASK) {
      case CSOUNDMSG_ERROR:
      case CSOUNDMSG_REALTIME:
      case CSOUNDMSG_WARNING:
        len = vfprintf(stderr, fmt, args);
        break;
      default:
        len = vfprintf(stdout, fmt, args);
      }
      p = (csMsgStruct*) malloc(sizeof(csMsgStruct) + (size_t) len);
      p->nxt = (csMsgStruct*) 0;
      p->attr = attr;
      vsprintf(&(p->s[0]), fmt, args);
      csoundLockMutex(pp->mutex_);
      if (pp->firstMsg == (csMsgStruct*) 0)
        pp->firstMsg = p;
      else
        pp->lastMsg->nxt = p;
      pp->lastMsg = p;
      pp->msgCnt++;
      csoundUnlockMutex(pp->mutex_);
  }

  void PUBLIC sigcpy(MYFLT *dest, MYFLT *src, int size)
  {                           /* Surely a memcpy*/
      memcpy(dest, src, size*sizeof(MYFLT));
  }

#ifdef __cplusplus
}
#endif

