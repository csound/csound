/*
 * Csound.c: csound engine initialisation and setup
 *
 *
 * Copyright (C) 2001-2006 Michael Gogins, Matt Ingalls, John D. Ramsdell,
 *                         John P. ffitch, Istvan Varga, Victor Lazzarini,
 *                         Steven Yi
 *
 * License
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
 * Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 *
 */

#if defined(HAVE_UNISTD_H) || defined(__unix) || defined(__unix__)
#include <unistd.h>
#endif

#include "csoundCore.h"
#include "csmodule.h"
#include "corfile.h"
#include "csound_standard_types.h"
#include "csGblMtx.h"
#include "fftlib.h"
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <windows.h>
#endif
#include <math.h>
#include "oload.h"
#include "fgens.h"
#include "namedins.h"
#include "pvfileio.h"
#include "fftlib.h"
#include "lpred.h"
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#include "namedins.h"
#include "find_opcode.h"

#if defined(linux) || defined(BSD) || defined(__HAIKU__) || defined(__EMSCRIPTEN__) ||         \
    defined(__CYGWIN__)
#define PTHREAD_SPINLOCK_INITIALIZER 0
#endif

#include "csound_standard_types.h"

#include "csdebug.h"
#include <time.h>

int32_t init0(CSOUND *csound);
int32_t kperf(CSOUND *csound);
int32_t csound_cleanup(CSOUND *);
int32_t get_time_resolution(void);

void allocate_message_queue(CSOUND *csound);
int32_t playopen_dummy(CSOUND *, const csRtAudioParams *parm);
void rtplay_dummy(CSOUND *, const MYFLT *outBuf, int32_t nbytes);
int32_t recopen_dummy(CSOUND *, const csRtAudioParams *parm);
int32_t rtrecord_dummy(CSOUND *, MYFLT *inBuf, int32_t nbytes);
void rtclose_dummy(CSOUND *);
int32_t audio_dev_list_dummy(CSOUND *, CS_AUDIODEVICE *, int32_t);
int32_t midi_dev_list_dummy(CSOUND *, CS_MIDIDEVICE *, int32_t);

/* dummy real time MIDI functions */
int32_t DummyMidiInOpen(CSOUND *csound, void **userData, const char *devName);
int32_t DummyMidiRead(CSOUND *csound, void *userData, unsigned char *buf,
                      int32_t nbytes);
int32_t DummyMidiOutOpen(CSOUND *csound, void **userData, const char *devName);
int32_t DummyMidiWrite(CSOUND *csound, void *userData, const unsigned char *buf,
                       int32_t nbytes);
void csound_init_rand(CSOUND *);

static void set_util_sr(CSOUND *csound, MYFLT sr);
static void set_util_nchnls(CSOUND *csound, int32_t nchnls);
static int32_t csoundDeprecate(CSOUND *csound, char *name,
                               char *o, char *i, int32_t deprec);
void memreset(CSOUND *);
MYFLT csoundPow2(CSOUND *csound, MYFLT a);
int32_t csoundInitStaticModules(CSOUND *);
void close_all_files(CSOUND *);
void csoundInputMessage(CSOUND *csound, const char *message);
int32_t isstrcod(MYFLT);
int32_t csoundFtError(const FGDATA *ff, const char *s, ...);
void csound_aops_init_tables(CSOUND *cs);
void csoundDeleteAllGlobalVariables(CSOUND *csound);
void (*msgcallback_)(CSOUND *, int32_t, const char *, va_list) = NULL;
void *csoundDCTSetup(CSOUND *csound, int32_t FFTsize, int32_t d);
void csoundDCT(CSOUND *csound, void *p, MYFLT *sig);
void message_dequeue(CSOUND *csound);
int32_t csound_compile_tree(CSOUND *csound, TREE *root, int32_t async);
int32_t csound_compile_orc(CSOUND *csound, const char *str,
                                 int32_t async);
int32_t csoundReadScore(CSOUND *csound, const char *message);
void csoundDefaultMessageCallback(CSOUND *, int32_t, const char *,
                                         va_list);
static INSTRTXT **csoundGetInstrumentList(CSOUND *csound);
static int32_t csoundDoCallback_(CSOUND *, void *, uint32_t);
static void reset(CSOUND *);
extern const OENTRY opcodlst_1[];

#define STRING_HASH(arg) STRSH(arg)
#define STRSH(arg) #arg

static void *csoundGetNamedGens(CSOUND *csound) {
  return csound->namedgen;
}

static long csoundGetInputBufferSize(CSOUND *csound) {
  return csound->oparms_.inbufsamps;
}

static long csoundGetOutputBufferSize(CSOUND *csound) {
  return csound->oparms_.outbufsamps;
}

static uint32_t csoundGetNchnls(CSOUND *csound) {
  return csound->nchnls;
}

static uint32_t csoundGetNchnlsInput(CSOUND *csound) {
  if (csound->inchnls >= 0)
    return (uint32_t)csound->inchnls;
  else
    return csound->nchnls;
}

static INSTRTXT **csoundGetInstrumentList(CSOUND *csound) {
  return csound->engineState.instrtxtp;
}
static void set_util_sr(CSOUND *csound, MYFLT sr) {
  csound->esr = sr;
}
static void set_util_nchnls(CSOUND *csound, int32_t nchnls) {
  csound->nchnls = nchnls;
}

static const char *csoundGetStrsets(CSOUND *csound, int32_t n) {
  if (csound->strsets == NULL)
    return NULL;
  else
    return csound->strsets[n];
}

static int32_t csoundGetStrsetsMax(CSOUND *csound) {
  return csound->strsmax;
}

static const char *csoundFileError(CSOUND *csound, void *ff) {
  CSFILE *f = (CSFILE *)ff;
  switch (f->type) {
  case CSFILE_SND_W:
  case CSFILE_SND_R:
    return csound->SndfileStrError(csound, ff);
    break;
  default:
    return "";
  }
}

void print_csound_version(CSOUND *csound) {
#ifdef USE_DOUBLE
#ifdef BETA
  csoundErrorMsg(csound,
                 Str("--Csound version %s beta (double samples) %s\n"
                     "[commit: %s]\n"),
                 CS_PACKAGE_VERSION, CS_PACKAGE_DATE,
                 STRING_HASH(GIT_HASH_VALUE));
#else
  csoundErrorMsg(csound,
                 Str("--Csound version %s (double samples) %s\n"
                     "[commit: %s]\n"),
                 CS_PACKAGE_VERSION, CS_PACKAGE_DATE,
                 STRING_HASH(GIT_HASH_VALUE));
#endif
#else
#ifdef BETA
  csoundErrorMsg(csound,
                 Str("--Csound version %s beta (float samples) %s\n"
                     "[commit: %s]\n"),
                 CS_PACKAGE_VERSION, CS_PACKAGE_DATE,
                 STRING_HASH(GIT_HASH_VALUE));
#else
  csoundErrorMsg(csound,
                 Str("--Csound version %s (float samples) %s\n"
                     "[commit: %s]\n"),
                 CS_PACKAGE_VERSION, CS_PACKAGE_DATE,
                 STRING_HASH(GIT_HASH_VALUE));
#endif
#endif
}

void print_sndfile_version(CSOUND *csound) {
#ifdef USE_LIBSNDFILE
  char buffer[128];
  csound->SndfileCommand(csound, NULL, SFC_GET_LIB_VERSION, buffer, 128);
  csoundErrorMsg(csound, "using %s\n", buffer);
#else
  csoundErrorMsg(csound, "%s\n", "not using libsndfile");
#endif
}

void print_engine_parameters(CSOUND *csound) {
  csoundErrorMsg(csound, Str("sr = %.1f,"), csound->esr);
  csoundErrorMsg(csound, Str(" kr = %.3f,"), csound->ekr);
  csoundErrorMsg(csound, Str(" ksmps = %d\n"), csound->ksmps);
  csoundErrorMsg(csound, Str("0dBFS level = %.1f,"), csound->e0dbfs);
  csoundErrorMsg(csound, Str(" A4 tuning = %.1f\n"), csound->A4);
}

static void free_opcode_table(CSOUND *csound) {
  int32_t i;
  CS_HASH_TABLE_ITEM *bucket;
  CONS_CELL *head;

  for (i = 0; i < csound->opcodes->table_size; i++) {
    bucket = csound->opcodes->buckets[i];

    while (bucket != NULL) {
      head = bucket->value;
      cs_cons_free_complete(csound, head);
      bucket = bucket->next;
    }
  }

  cs_hash_table_free(csound, csound->opcodes);
}

/** Module API csound->Event() calls are always synchronous
    as in csound->ReadScore() and csound->InputMessage()
 */
static void csoundEvent_(CSOUND *csound, int32_t type,
                         const MYFLT *pfields, int32_t pnum) {
  csoundEvent(csound, type, pfields, pnum, 0);
}


static void create_opcode_table(CSOUND *csound) {

  int32_t err;

  if (csound->opcodes != NULL) {
    free_opcode_table(csound);
  }
  csound->opcodes = cs_hash_table_create(csound);

  /* Basic Entry1 stuff */
  err = csoundAppendOpcodes(csound, &(opcodlst_1[0]), -1);

  if (UNLIKELY(err))
    csoundDie(csound, Str("Error allocating opcode list"));
}

#define MAX_MODULES 64

static void csoundModuleListAdd(CSOUND *csound, char *drv, char *type) {
  MODULE_INFO **modules =
      (MODULE_INFO **)csoundQueryGlobalVariable(csound, "_MODULES");
  if (modules != NULL) {
    int32_t i = 0;
    while (modules[i] != NULL && i < MAX_MODULES) {
      if (!strcmp(modules[i]->module, drv))
        return;
      i++;
    }
    modules[i] = (MODULE_INFO *)csound->Malloc(csound, sizeof(MODULE_INFO));
    strNcpy(modules[i]->module, drv, 11);
    strNcpy(modules[i]->type, type, 11);
  }
}

static int32_t csoundGetRandSeed(CSOUND *csound, int32_t which) {
  if (which > 1)
    return csound->randSeed1;
  else
    return csound->randSeed2;
}

static int32_t *csoundRandSeed31(CSOUND *csound) {
  return &(csound->randSeed1);
}

static int32_t csoundGetDitherMode(CSOUND *csound) {
  return csound->dither_output;
}

static int32_t csoundGetReinitFlag(CSOUND *csound) {
  return csound->reinitflag;
}

static int32_t csoundGetTieFlag(CSOUND *csound) {
  return csound->tieflag;
}

MYFLT csoundSystemSr(CSOUND *csound, MYFLT val) {
  if (val > 0)
    csound->_system_sr = val;
  return csound->_system_sr;
}

/* get type from name */
 const CS_TYPE *csoundGetType(CSOUND *csound, const char *type) {
  return csoundGetTypeWithVarTypeName(csound->typePool, type);
}

const CSOUND_UTIL *csoundGetCsoundUtility(CSOUND *csound) {
  return &csound->csound_util;
}

/*
  exact selects the type of search
 */
static const OENTRY *csoundFindOpcode(CSOUND *csound, int32_t exact,
                                      char *opname, char *outargs,
                                      char *inargs) {
  if (exact)
    return find_opcode_exact(csound, opname, outargs, inargs);
  else
    return find_opcode_new(csound, opname, outargs, inargs);
}

// NB: function naming convention
// all API functions (host/module)
// csound***(), corresponding to csound->***()
// but not all marked  (only for host API)

static const CSOUND cenviron_ = {
    /* attributes  */
    csoundGetNchnls,
    csoundGetNchnlsInput,
    csoundGet0dBFS,
    csoundGetA4,
    csoundGetTieFlag,
    csoundGetReinitFlag,
    csoundGetInstrumentList,
    csoundGetStrsetsMax,
    csoundGetStrsets,
    csoundGetHostData,
    csoundGetCurrentTimeSamples,
    csoundGetInputBufferSize,
    csoundGetOutputBufferSize,
    csoundGetDebug,
    csoundGetSizeOfMYFLT,
    csoundGetParams,
    csoundGetEnv,
    csoundSystemSr,
    /* channels */
    csoundGetChannelPtr,
    csoundListChannels,
    /* events and performance */
    csoundEvent_,   /* csoundEvent_ is csoundEvent arity - 1*/
    csoundGetScoreOffsetSeconds,
    csoundSetScoreOffsetSeconds,
    csoundRewindScore,
    csoundInputMessage,  
    csoundReadScore,     
    /* message printout */
    csoundMessage,
    csoundMessageS,
    csoundMessageV,
    csoundGetMessageLevel,
    csoundSetMessageLevel,
    csoundSetMessageCallback,
    /* arguments to opcodes and types*/
    csoundGetString,   
    csoundStringArg2Insno, 
    csoundStringArg2Name,   
    csoundGetType,              
    csoundGetTypePool,
    csoundAddVariableType,
    /* memory allocation */
    csoundAuxalloc,            
    csoundAuxalloc_async,       
    csoundMalloc,              
    csoundCalloc,              
    csoundRealloc,              
    csoundStrdup,             
    csoundFree,                 
    /* function tables */
    csoundFTCreate,
    csoundFTAlloc,   
    csoundFTFree,   
    csoundFTFind,    
    csoundGetNamedGens,         
    /* global and config variable manipulation */
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
    /* FFT support */
    csoundRealFFT2Setup,
    csoundRealFFT2,
    csoundGetInverseRealFFTScale,
    csoundComplexFFT,
    csoundInverseComplexFFT,
    csoundGetInverseComplexFFTScale,
    csoundRealFFTMult,
    csoundDCTSetup,
    csoundDCT,
    /* LPC support */
    csoundAutoCorrelation,
    csoundLPsetup,
    csoundLPfree,
    csoundLPred,
    csoundLPCeps,
    csoundCepsLP,
    csoundLPrms,
    /* PVOC-EX system */
    csoundPVOC_CreateFile, 
    csoundPVOC_OpenFile,   
    csoundPVOC_Closefile,  
    csoundPVOC_PutFrames,  
    csoundPVOC_GetFrames,  
    csoundPVOC_FrameCount, 
    csoundPVOC_fseek,     
    csoundPVOC_ErrorStr,   
    csoundPVOCEX_LoadFile,  
    /* error messages */
    csoundDie,
    csoundInitError,
    csoundPerfError,
    csoundFtError,         
    csoundWarning,  
    csoundDebugMsg,
    csoundLongJmp,
    csoundErrorMsg,
    csoundErrMsgV,
    /* random numbers */
    csoundGetRandomSeedFromTime,
    csoundSeedRandMT,
    csoundRandMT,
    csoundRand31,
    csoundRandSeed31,    
    csoundGetRandSeed,
    /* threads and locks */
    csoundCreateThread,
    csoundJoinThread,
    csoundCreateThreadLock,
    csoundDestroyThreadLock,
    csoundWaitThreadLock,
    csoundNotifyThreadLock,
    csoundWaitThreadLockNoTimeout,
    csoundCreateMutex,
    csoundLockMutexNoWait,
    csoundLockMutex,
    csoundUnlockMutex,
    csoundDestroyMutex,
    csoundCreateBarrier,
    csoundDestroyBarrier,
    csoundWaitBarrier,
    csoundGetCurrentThreadId,
    csoundSleep,
    csoundInitTimerStruct,
    csoundGetRealTime,
    csoundGetCPUTime,
    /* circular buffer */
    csoundCreateCircularBuffer,
    csoundReadCircularBuffer,
    csoundWriteCircularBuffer,
    csoundPeekCircularBuffer,
    csoundFlushCircularBuffer,
    csoundDestroyCircularBuffer,
    /* File access */
    csoundFindInputFile,
    csoundFindOutputFile,
    csoundFileOpenWithType,
    csoundNotifyFileOpened,
    csoundFileClose,
    csoundFileError,
    csoundFileOpenWithType_Async,
    csoundReadAsync,
    csoundWriteAsync,
    csoundFSeekAsync,
    csoundRewriteHeader, 
    csoundLoadSoundFile,   
    csoundLoadMemoryfile,  
    csoundFDRecord,           
    csoundFDClose,     
    csoundCreateFileHandle,
    csoundGetFileName,
    csoundType2CsfileType,   
    csoundSndfileType2CsfileType, 
    csoundType2String,       
    csoundGetStrFormat,    
    csoundSndfileSampleSize,       
    /* sndfile interface */
    csoundSndfileOpen,    
    csoundSndfileOpenFd, 
    csoundSndfileClose,
    csoundSndfileWrite,
    csoundSndfileRead,
    csoundSndfileWriteSamples,
    csoundSndfileReadSamples,
    csoundSndfileSeek,
    csoundSndfileSetString,
    csoundSndfileStrError,
    csoundSndfileCommand,
    /* generic callbacks */
    csoundRegisterKeyboardCallback,
    csoundRemoveKeyboardCallback,
    csoundRegisterResetCallback,
    /* hash table funcs */
    cs_hash_table_create,  // csoundCreateHashTable
    cs_hash_table_get,   //   csoundGetHashTable
    cs_hash_table_put,    // etc
    cs_hash_table_remove,
    cs_hash_table_free,
    cs_hash_table_get_key,
    cs_hash_table_keys,
    cs_hash_table_values,
    /* opcodes and instruments  */
    csoundAppendOpcode,
    csoundAppendOpcodes,
    csoundFindOpcode,
    /* RT audio IO and callbacks */
    csoundSetPlayopenCallback,
    csoundSetRtplayCallback,
    csoundSetRecopenCallback,
    csoundSetRtrecordCallback,
    csoundSetRtcloseCallback,
    csoundSetAudioDeviceListCallback,
    csoundGetRtRecordUserData,
    csoundGetRtPlayUserData,
    csoundGetDitherMode,
    /* MIDI and callbacks */
    csoundSetExternalMidiInOpenCallback,
    csoundSetExternalMidiReadCallback,
    csoundSetExternalMidiInCloseCallback,
    csoundSetExternalMidiOutOpenCallback,
    csoundSetExternalMidiWriteCallback,
    csoundSetExternalMidiOutCloseCallback,
    csoundSetExternalMidiErrorStringCallback,
    csoundSetMIDIDeviceListCallback,
    csoundModuleListAdd,  // csoundModuleListAdd
    /* displays & graphs */
    csoundSetDisplay,    // csoundSetDisplay
    csoundDisplay,    // csoundDisplay
    csoundDeinitDisplay,   // csoundDeinitDisplay
    csoundInitDisplay,   // csoundInitDisplay
    csoundSetIsGraphable,
    csoundSetMakeGraphCallback,
    csoundSetDrawGraphCallback,
    csoundSetKillGraphCallback,
    csoundSetExitGraphCallback,
    /* miscellaneous */
    csoundGetCsoundUtility,
    csoundPow2,
    csoundLocalizeString,
    csoundStrtod, // csoundStrtod
    csoundSprintf, // csoundSprintf
    csoundSscanf,  // csoundSscanf
    csoundDeprecate, 
    /* space for API expansion: 50 slots */
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    /* ------- private data (not to be used by hosts or externals) ------- */
    /* callback function pointers */
  (SUBR) NULL,    /*  first_callback_     */
  (channelCallback_t) NULL,
  (channelCallback_t) NULL,
  csoundDefaultMessageCallback,
  (int32_t (*)(CSOUND *)) NULL,
  (void (*)(CSOUND *, WINDAT *, const char *)) NULL, /* was: MakeAscii,*/
  (void (*)(CSOUND *, WINDAT *windat)) NULL, /* was: DrawAscii,*/
  (void (*)(CSOUND *, WINDAT *windat)) NULL, /* was: KillAscii,*/
  (int32_t (*)(CSOUND *)) NULL, /* was: defaultCsoundExitGraph, */
  (void*(*)(CSOUND*, const char*, int32_t,  void*)) NULL,/* OpenSoundFileCallback_ */
  (FILE*(*)(CSOUND*, const char*, const char*)) NULL, /* OpenFileCallback_ */
  (void(*)(CSOUND*, const char*, int32_t,  int32_t,  int32_t)) NULL, /* FileOpenCallback_ */
  (SUBR) NULL,    /*  last_callback_      */
  /* these are not saved on RESET */
  playopen_dummy,
  rtplay_dummy,
  recopen_dummy,
  rtrecord_dummy,
  rtclose_dummy,
  audio_dev_list_dummy,
  midi_dev_list_dummy,
  csoundDoCallback_,  /*  doCsoundCallback    */
  kperf,    /* current kperf function - not debug by default */
  (void (*)(CSOUND *csound, int32_t attr, const char *str)) NULL,/* message string callback */
  (void (*)(CSOUND *)) NULL,                      /*  spinrecv    */
  (void (*)(CSOUND *)) NULL,                      /*  spoutran    */
  (int32_t (*)(CSOUND *, MYFLT *, int32_t)) NULL,         /*  audrecv     */
  (void (*)(CSOUND *, const MYFLT *, int32_t)) NULL,  /*  audtran     */
  NULL,           /*  hostdata            */
  NULL, NULL,     /*  orchname, scorename */
  NULL, NULL,     /*  orchstr, *scorestr  */
  (OPDS*) NULL,   /*  ids                 */
  { (CS_VAR_POOL*)NULL,
    (CS_HASH_TABLE *) NULL,
    (CS_HASH_TABLE *) NULL,
    -1,
    (INSTRTXT**)NULL,
    { NULL,
      {
        0,0,
        NULL, NULL, NULL, NULL,
        0,0,
        NULL,
        0},
      0,0,0,
      //0,
      NULL,
      0,
      0,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      0,
      0,
      FL(0.0),
      NULL,
      NULL,
      0,
      0,
      0
    },
    NULL,
    MAXINSNO,     /* engineState          */
  },
  (INSTRTXT *) NULL, /* instr0  */
  (INSTRTXT**)NULL,  /* dead_instr_pool */
  0,                /* dead_instr_no */
  (TYPE_POOL*)NULL,
  DFLT_KSMPS,     /*  ksmps               */
  DFLT_NCHNLS,    /*  nchnls              */
  -1,             /*  inchns              */
  0L,             /*  kcounter            */
  0L,             /*  global_kcounter     */
  DFLT_SR,        /*  esr                 */
  DFLT_KR,        /*  ekr                 */
  0l,             /*  curTime             */
  0l,             /*  curTime_inc         */
  0.0,            /*  timeOffs            */
  0.0,            /*  beatOffs            */
  0.0,            /*  curBeat             */
  0.0,            /*  curBeat_inc         */
  0L,             /*  beatTime            */
  (EVTBLK*) NULL, /*  currevent           */
  (INSDS*) NULL,  /*  curip               */
  FL(0.0),        /*  cpu_power_busy      */
  (char*) NULL,   /*  xfilename           */
  1,              /*  peakchunks          */
  0,              /*  keep_tmp            */
  (CS_HASH_TABLE*)NULL, /* Opcode hash table */
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
  NULL,           /*  argoffspace         */
  NULL,           /*  frstoff             */
  0,              /*  randSeed1           */
  0,              /*  randSeed2           */
  NULL,           /*  csRandState         */
  NULL,           /*  csRtClock           */
  // 16384,            /*  strVarMaxLen        */
  0,              /*  strsmax             */
  (char**) NULL,  /*  strsets             */
  NULL,           /*  spin                */
  NULL,           /*  spout               */
  NULL,           /*  spout_tmp               */
  0,              /*  nspin               */
  0,              /*  nspout              */
  NULL,           /*  auxspin             */
  (OPARMS*) NULL, /*  oparms              */
  { NULL },       /*  m_chnbp             */
  0,              /*   dither_output      */
  FL(0.0),        /*  onedsr              */
  FL(0.0),        /*  sicvt               */
  FL(-1.0),       /*  tpidsr              */
  FL(-1.0),       /*  pidsr               */
  FL(-1.0),       /*  mpidsr              */
  FL(-1.0),       /*  mtpdsr              */
  FL(0.0),        /*  onedksmps           */
  FL(0.0),        /*  onedkr              */
  FL(0.0),        /*  kicvt               */
  0,              /*  reinitflag          */
  0,              /*  tieflag             */
  DFLT_DBFS,      /*  e0dbfs              */
  FL(1.0) / DFLT_DBFS, /* dbfs_to_float ( = 1.0 / e0dbfs) */
  440.0,               /* A4 base frequency */
  NULL,           /*  rtRecord_userdata   */
  NULL,           /*  rtPlay_userdata     */
#if defined(MSVC) ||defined(__POWERPC__) || defined(MACOSX)
  {0},
#elif defined(LINUX)
  {{{0}}},        /*  exitjmp of type jmp_buf */
#else
  {0},
#endif
  NULL,           /*  frstbp              */
  0,              /*  sectcnt             */
  0, 0, 0, 0,     /*  inerrcnt, synterrcnt, perferrcnt, total_assert_cnt */
  /* {NULL}, */   /*  instxtanchor  in engineState */
  {   /*  actanchor           */
    NULL,
    NULL,
    NULL,
    NULL, /*nxtdd*/
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    NULL,
    NULL,
    0,
    NULL,
    0,
    0,
    0,
    0,
    0,
    0.0,
    0.0,
    //    NULL,
    NULL,
    0,
    FL(0.0),  /* esr */
    FL(0.0),
    FL(0.0),
    FL(0.0),
    0,       /* in_cvt */
    0,       /* out_cvt */
    0,
    FL(0.0),
    FL(0.0), FL(0.0), FL(0.0),
    NULL,
    {FL(0.0), FL(0.0), FL(0.0), FL(0.0)},
    NULL,NULL,
    NULL,
    0,
    0,
    0,
    NULL,
    NULL,
    0,
    0,
    0,
    FL(0.0),
    NULL,
    NULL,
    0,  /* link flag */
    0,  /* instance id */
    {NULL, FL(0.0)},
    {NULL, FL(0.0)},
    {NULL, FL(0.0)},
    {NULL, FL(0.0)}
  },
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
  NULL,           /*  opcodeInfo  */
  NULL,           /*  flist               */
  0,              /*  maxfnum             */
  NULL,           /*  gensub              */
  GENMAX+1,       /*  genmax              */
  NULL,           /*  namedGlobals        */
  NULL,           /*  cfgVariableDB       */
  FL(0.0), FL(0.0), FL(0.0),  /*  prvbt, curbt, nxtbt */
  FL(0.0), FL(0.0),       /*  curp2, nxtim        */
  0,              /*  cyclesRemaining     */
  { 0, NULL, NULL, 0, '\0', 0, FL(0.0),
    FL(0.0), NULL},   /*  evt */
  NULL,           /*  memalloc_db         */
  (MGLOBAL*) NULL, /* midiGlobals         */
  NULL,           /*  envVarDB            */
  (MEMFIL*) NULL, /*  memfiles            */
  NULL,           /*  pvx_memfiles        */
  0,              /*  FFT_max_size        */
  NULL,           /*  FFT_table_1         */
  NULL,           /*  FFT_table_2         */
  NULL, NULL, NULL, /* tseg, tpsave, unused */
  (MYFLT*) NULL,  /*  gbloffbas           */
  NULL,           /* file_io_thread    */
  0,              /* file_io_start   */
  NULL,           /* file_io_threadlock */
  0,              /* realtime_audio_flag */
  NULL,           /* init pass thread */
  0,              /* init pass loop  */
  NULL,           /* init pass threadlock */
  NULL,           /* API_lock */
  SPINLOCK_INIT, SPINLOCK_INIT, /* spinlocks */
  SPINLOCK_INIT, SPINLOCK_INIT, /* spinlocks */
  NULL, NULL,             /* Delayed messages */
  {
    NULL, NULL, NULL, NULL, /* bp, prvibp, sp, nx */
    0, 0, 0, 0,   /*  op warpin linpos lincnt */
    -FL(1.0), FL(0.0), FL(1.0), /* prvp2 clock_base warp_factor */
    NULL,         /*  curmem              */
    NULL,         /*  memend              */
    NULL,         /*  macros              */
    -1,           /*  next_name           */
    NULL, NULL,   /*  inputs, str         */
    0,0,0,        /*  input_size, input_cnt, pop */
    1,            /*  ingappop            */
    -1,           /*  linepos             */
    {{NULL, 0, 0}}, /* names        */
    {""},         /*  repeat_name_n[RPTDEPTH][NAMELEN] */
    {0},          /*  repeat_cnt_n[RPTDEPTH] */
    {0},          /*  repeat_point_n[RPTDEPTH] */
    1, {NULL}, 0, /*  repeat_inc_n,repeat_mm_n repeat_index */
    "",          /*  repeat_name[NAMELEN] */
    0,0,1,        /*  repeat_cnt, repeat_point32_t,  repeat_inc */
    NULL,         /*  repeat_mm */
    0
  },
  {
    NULL,
    NULL, NULL, NULL, /* orcname, sconame, midname */
    0, 0           /* midiSet, csdlinecount */
  },
  {
    NULL, NULL,   /* Linep, Linebufend    */
    0,            /* stdmode              */
    {
      0, NULL, NULL, 0, 0, 0, FL(0.0), FL(0.0), NULL
    },            /* EVTBLK  prve         */
    NULL,        /* Linebuf              */
    0,            /* linebufsiz */
    NULL, NULL,
    0, NULL, 0
  },
  {
    {0,0}, {0,0},  /* srngcnt, orngcnt    */
    0, 0, 0, 0, 0, /* srngflg, sectno, lplayed, segamps, sormsg */
  },
  //NULL,           /*  musmonGlobals       */
  {
    NULL,         /*  outfile             */
    NULL,         /*  infile              */
    NULL,         /*  sfoutname;          */
    NULL,         /*  inbuf               */
    NULL,         /*  outbuf              */
    NULL,         /*  outbufp             */
    0,            /*  inbufrem            */
    0,            /*  outbufrem           */
    0,0,          /*  inbufsiz,  outbufsiz */
    0,            /*  isfopen             */
    0,            /*  osfopen             */
    0,0,          /*  pipdevin, pipdevout */
    1U,           /*  nframes             */
    NULL, NULL,   /*  pin, pout           */
    0,            /*dither                */
  },
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
  //    NULL,           /*  pluginOpcodeFiles   */
  0,              /*  enableHostImplementedAudioIO  */
  0,              /* MIDI IO */
  0,              /*  hostRequestedBufferSize       */
  0,              /*  engineStatus         */
  0,              /*  stdin_assign_flg    */
  0,              /*  stdout_assign_flg   */
  0,              /*  orcname_mode        */
  0,              /*  use_only_orchfile   */
  NULL,           /*  csmodule_db         */
  (char*) NULL,   /*  dl_opcodes_oplibs   */
  (char*) NULL,   /*  SF_csd_licence      */
  (char*) NULL,   /*  SF_id_title         */
  (char*) NULL,   /*  SF_id_copyright     */
  -1,             /*  SF_id_scopyright    */
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
  (MYFLT*) NULL,  /*  disprep_fftcoefs    */
  NULL,           /*  winEPS_globals      */
  {               /*  oparms_             */
    0,            /*    odebug            */
    0, 1, 0,   /*    sfread, ...       */
    0, 0, 0, 0,   /*    inbufsamps, ...   */
    0,            /*    csoundSndfileSampleSize        */
    1,            /*    displays          */
    1, 0, 135,    /*    graphsoff ...     */
    0, 0,         /*    Beatmode, ...     */
    0,
    0, 0, 0, 0,   /*    RTevents, ...     */
    0, 0,         /*    ringbell, ...     */
    0, 0, 0,      /*    rewrt_hdr, ...    */
    0.0,          /*    cmdTempo          */
    0.0f, 0.0f,   /*    sr_override ...   */
    0, 0,     /*    nchnls_override ...   */
    (char*) NULL, (char*) NULL,   /* filenames */
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
    0,            /*    runUnitTests      */
    1,            /*    useCsdLineCounts  */
    0,            /*    samp acc   */
    0,            /*    realtime  */
    0.0,          /*    0dbfs override */
    0,            /*    no exit on compile error */
    0.4,          /*    vbr quality  */
    0,            /*    ksmps_override */
    0,             /*    fft_lib */
    0,             /*    echo */
    0.0,           /*   limiter */
    DFLT_SR, DFLT_KR,  /* defaults */
    0,             /* mp3 mode */
    0              /* instr redefinition flag */
  },
  {0, 0, {0}}, /* REMOT_BUF */
  NULL,           /* remoteGlobals        */
  0, 0,           /* nchanof, nchanif     */
  NULL, NULL,     /* chanif, chanof       */
  0,              /* multiThreadedComplete */
  NULL,           /* multiThreadedThreadInfo */
  NULL,           /* multiThreadedDag */
  NULL,           /* barrier1 */
  NULL,           /* barrier2 */
  /* statics from cs_par_orc_semantic_analysis */
  NULL,           /* instCurr */
  NULL,           /* instRoot */
  0,              /* inInstr */
  /* new dag model statics */
  1,              /* dag_changed */
  0,              /* dag_num_active */
  NULL,           /* dag_task_map */
  NULL,           /* dag_task_status */
  NULL,           /* dag_task_watch */
  NULL,           /* dag_wlmm */
  NULL,           /* dag_task_dep */
  100,            /* dag_task_max_size */
  0,              /* tempStatus */
  1,              /* orcLineOffset */
  0,              /* scoLineOffset */
  NULL,           /* csdname */
  0,              /*  parserNamedInstrFlag */
  0,              /*  tran_nchnlsi */
  0,              /* Count of score strings */
  0,              /* length of current strings space */
  NULL,           /* sinetable */
  16384,          /* sinesize */
  NULL,           /* unused *** pow2 table */
  NULL,           /* cps conv table */
  NULL,           /* output of preprocessor */
  NULL,           /* output of preprocessor */
  {NULL},         /* filedir */
  NULL,           /* message buffer struct */
  0,              /* jumpset */
  0,              /* info_message_request */
  0,              /* modules loaded */
  -1,             /* audio system sr */
  0,              /* csdebug_data */
  0,              /* which score parser */
  0,              /* print_version */
  1,              /* inZero */
  NULL,           /* msg_queue */
  0,              /* msg_queue_wget */
  0,              /* msg_queue_wput */
  0,              /* msg_queue_rstart */
  0,              /* msg_queue_items */
  127,            /* aftouch */
  NULL,           /* directory for corfiles */
  NULL,           /* alloc_queue */
  0,              /* alloc_queue_items */
  0,              /* alloc_queue_wp */
  SPINLOCK_INIT,  /* alloc_spinlock */
  NULL,           /* init_event */
  NULL,           /* message_string */
  0,              /* message_string_queue_items */
  0,              /* message_string_queue_wp */
  NULL,           /* message_string_queue */
  0,              /* io_initialised */
  0,              /* options_checked */
  NULL,           /* op */
  0,              /* mode */
  NULL,           /* opcodedir */
  NULL,           /* score_srt */
  {NULL, NULL, NULL, 0, 0, NULL}, /* osc_message_anchor */
  NULL,
  SPINLOCK_INIT,
  {                /* csound_util */
  csoundAddUtility,
  csoundRunUtility,
  csoundListUtilities,
  csoundSetUtilityDescription,
  csoundGetUtilityDescription,
  set_util_sr,
  set_util_nchnls,
  SAsndgetset,
  sndgetset,
  getsndin
  },
    0, /* instance count */
    300, /* genLabs */
    0, 0, 0, 0, /* midi RT messages */
    0, /* struct_array_temp_counter */
    0, NULL  /* PARCS thread sync */
};

void csoundLongJmp(CSOUND *csound, int32_t retval) {
  int32_t n = CSOUND_EXITJMP_SUCCESS;

  n = (retval < 0 ? n + retval : n - retval) & (CSOUND_EXITJMP_SUCCESS - 1);
  // printf("**** n = %d\n", n);
  if (!n)
    n = CSOUND_EXITJMP_SUCCESS;

  csound->curip = NULL;
  csound->ids = NULL;
  csound->reinitflag = 0;
  csound->tieflag = 0;
  csound->perferrcnt += csound->inerrcnt;
  csound->inerrcnt = 0;
  csound->engineStatus |= CS_STATE_JMP;
  // printf("**** longjmp with %d\n", n);

  longjmp(csound->exitjmp, n);
}



typedef struct csInstance_s {
  CSOUND *csound;
  struct csInstance_s *nxt;
} csInstance_t;

/* initialisation state: */
/* 0: not done yet, 1: complete, 2: in progress, -1: failed */
static volatile int32_t init_done = 0;
/* chain of allocated Csound instances */
static volatile csInstance_t *instance_list = NULL;
/* non-zero if performance should be terminated now */
static volatile int32_t exitNow_ = 0;

#if !defined(WIN32)
static void destroy_all_instances(void) {
  volatile csInstance_t *p;

  csoundLock();
  init_done = -1; /* prevent the creation of any new instances */
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
#endif

#if defined(ANDROID) ||                                                        \
    (!defined(LINUX) && !defined(SGI) && !defined(__HAIKU__) &&                \
     !defined(__BEOS__) && !defined(__MACH__) && !defined(__EMSCRIPTEN__))

static char *signal_to_string(int32_t sig) {
  switch (sig) {
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

#ifdef ANDROID
static void psignal_(int sig, char *str) {
  fprintf(stderr, "%s: %s\n", str, signal_to_string(sig));
}
#else
#if !defined(__CYGWIN__) && !defined(BSD)
void psignal(int sig, const char *str) {
  fprintf(stderr, "%s: %s\n", str, signal_to_string(sig));
}
#endif
#endif
#elif defined(__BEOS__)
static void psignal_(int sig, char *str) {
  fprintf(stderr, "%s: %s\n", str, strsignal(sig));
}
#endif

static void signal_handler(int sig) {
#if defined(HAVE_EXECINFO_H) && !defined(ANDROID)
#include <execinfo.h>

  {
    int32_t j, nptrs;
#define SIZE 100
    void *buffer[SIZE];
    char **strings;

    nptrs = backtrace(buffer, SIZE);
    printf("backtrace() returned %d addresses\n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (UNLIKELY(strings == NULL)) {
      perror("backtrace_symbols");
      exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
      printf("%s\n", strings[j]);

    free(strings);
  }
#endif

#if defined(SIGPIPE)
  if (sig == (int32_t)SIGPIPE) {
#ifdef ANDROID
    psignal_(sig, "Csound ignoring SIGPIPE");
#elif !defined(__wasm__)
    psignal(sig, "Csound ignoring SIGPIPE");
#endif
    return;
  }
#endif
#ifdef ANDROID
  psignal_(sig, "Csound tidy up");
#elif !defined(__wasm__)
  psignal(sig, "Csound tidy up");
#endif
  if ((sig == (int32_t)SIGINT || sig == (int32_t)SIGTERM) && !exitNow_) {
    exitNow_ = -1;
    return;
  }
  exit(1);
}

static const int32_t sigs[] = {
#if defined(LINUX) || defined(SGI) || defined(sol) || defined(__MACH__)
    SIGHUP, SIGINT, SIGQUIT, SIGILL,  SIGTRAP, SIGABRT, SIGIOT,
    SIGBUS, SIGFPE, SIGSEGV, SIGPIPE, SIGTERM, SIGXCPU, SIGXFSZ,
#elif defined(WIN32)
    SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGTERM,
#elif defined(__EMX__)
    SIGHUP, SIGINT,  SIGQUIT, SIGILL,  SIGTRAP, SIGABRT, SIGBUS,
    SIGFPE, SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGTERM, SIGCHLD,
#endif
    -1};

static void install_signal_handler(void) {
  uint32_t i;
  for (i = 0; sigs[i] >= 0; i++) {
    signal(sigs[i], signal_handler);
  }
}



 int32_t csoundInitialize(int32_t flags) {
  int32_t n;

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
  if (get_time_resolution() != 0) {
    csoundLock();
    init_done = -1;
    csoundUnLock();
    return -1;
  }
  if (!(flags & CSOUNDINIT_NO_SIGNAL_HANDLER)) {
    install_signal_handler();
  }
#if !defined(WIN32)
  if (!(flags & CSOUNDINIT_NO_ATEXIT))
    atexit(destroy_all_instances);
#endif
  csoundLock();
  init_done = 1;
  csoundUnLock();
  return 0;
}

 CSOUND *csoundCreate(void *hostdata, const char *opcodedir) {
  CSOUND *csound;
  csInstance_t *p;
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

  if (init_done != 1) {
    if (csoundInitialize(0) < 0)
      return NULL;
  }
  csound = (CSOUND *)malloc(sizeof(CSOUND));
  if (UNLIKELY(csound == NULL))
    return NULL;
  memcpy(csound, &cenviron_, sizeof(CSOUND));
  init_getstring(csound);
  csound->oparms = &(csound->oparms_);
  csound->hostdata = hostdata;
  p = (csInstance_t *)malloc(sizeof(csInstance_t));
  if (UNLIKELY(p == NULL)) {
    free(csound);
    return NULL;
  }
  csoundLock();
  p->csound = csound;
  p->nxt = (csInstance_t *)instance_list;
  instance_list = p;
  csoundUnLock();
  // no csoundStrdup yet so we use strdup and
  // free it later in csoundDestroy
  if (opcodedir != NULL)
    csound->opcodedir = strdup(opcodedir);
  csoundReset(csound);
  csound->API_lock = csoundCreateMutex(1);
  allocate_message_queue(csound);
  // version is csoundDisplayed by default, can be suppressed via --suppress-version
  csound->print_version = 1;
  return csound;
}

typedef struct CsoundCallbackEntry_s CsoundCallbackEntry_t;
struct CsoundCallbackEntry_s {
  uint32_t typeMask;
  CsoundCallbackEntry_t *nxt;
  void *userData;
  int32_t (*func)(void *, void *, uint32_t);
};

 void csoundDestroy(CSOUND *csound) {
  csInstance_t *p, *prv = NULL;

  csoundLock();
  p = (csInstance_t *)instance_list;
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

  reset(csound);

  if (csound->csoundCallbacks_ != NULL) {
    CsoundCallbackEntry_t *pp, *nxt;
    pp = (CsoundCallbackEntry_t *)csound->csoundCallbacks_;
    do {
      nxt = pp->nxt;
      free((void *)pp);
      pp = nxt;
    } while (pp != (CsoundCallbackEntry_t *)NULL);
  }
  if (csound->API_lock != NULL) {
    // csoundLockMutex(csound->API_lock);
    csoundDestroyMutex(csound->API_lock);
  }
  // free opcodedir
  free(csound->opcodedir);
  /* clear the pointer */
  // *(csound->self) = NULL;
  free((void *)csound);
}

 int32_t csoundGetVersion(void) {
  return (int32_t)(CS_VERSION * 1000 + CS_SUBVER * 10 + CS_PATCHLEVEL);
}

 void *csoundGetHostData(CSOUND *csound) {
  return csound->hostdata;
}

 void csoundSetHostData(CSOUND *csound, void *hostData) {
  csound->hostdata = hostData;
}

void csoundStop(CSOUND *csound) {
  csound->performState = -1;
}

 int32_t csoundCompileTree(CSOUND *csound, TREE *root, int32_t async) {
  return csound_compile_tree(csound, root, async);
}

 int32_t csoundCompileOrc(CSOUND *csound, const char *str,
                                int32_t async) {
  return csound_compile_orc(csound, str, async);
}

MYFLT csoundEvalCode(CSOUND *csound, const char *str)
{
  int32_t async = 0;
  if (str && csound_compile_orc(csound,str,async)
      == CSOUND_SUCCESS){
    if(!(csound->engineStatus & CS_STATE_COMP)) {
      init0(csound);
    }
      return csound->instr0->instance[0].retval;
    }
  else return 0; // always return 0 on failure
}


 uint32_t csoundGetChannels(CSOUND *csound, int32_t isInput) {
  if (isInput)
    return csoundGetNchnlsInput(csound);
  else
    return csoundGetNchnls(csound);
}

 int64_t csoundGetCurrentTimeSamples(CSOUND *csound) {
  // icurTimeSamples is frames updated in ksmps-blocks
  return csound->icurTimeSamples;
}

 MYFLT csoundGetSr(CSOUND *csound) {
  return csound->esr;
}

 MYFLT csoundGetKr(CSOUND *csound) {
  return csound->ekr;
}

 uint32_t csoundGetKsmps(CSOUND *csound) {
  return csound->ksmps;
}

 MYFLT csoundGet0dBFS(CSOUND *csound) {
  return csound->e0dbfs;
}

 uint64_t csoundGetKcounter(CSOUND *csound) {
  return csound->kcounter;
}

 MYFLT csoundGetA4(CSOUND *csound) {
  return (MYFLT)csound->A4;
}

 int32_t csoundErrCnt(CSOUND *csound) {
  return csound->perferrcnt;
}

 MYFLT *csoundGetSpin(CSOUND *csound) {
  return csound->spin;
}

 const MYFLT *csoundGetSpout(CSOUND *csound) {
  return csound->spout;
}

 const char *csoundGetOutputName(CSOUND *csound) {
  return (const char *)csound->oparms_.outfilename;
}

 const char *csoundGetInputName(CSOUND *csound) {
  return (const char *)csound->oparms_.infilename;
}


 void csoundKeyPress(CSOUND *csound, char c) {
  csound->inChar_ = (int32_t)((unsigned char)c);
}

 void
csoundSetInputChannelCallback(CSOUND *csound,
                              channelCallback_t inputChannelCalback) {
  csound->InputChannelCallback_ = inputChannelCalback;
}

 void
csoundSetOutputChannelCallback(CSOUND *csound,
                               channelCallback_t outputChannelCalback) {
  csound->OutputChannelCallback_ = outputChannelCalback;
}


 int32_t csoundSetIsGraphable(CSOUND *csound, int32_t isGraphable) {
  int32_t prv = csound->isGraphable_;
  csound->isGraphable_ = isGraphable;
  return prv;
}

 void csoundSetMakeGraphCallback(CSOUND *csound,
                                       void (*makeGraphCB)(CSOUND *csound,
                                                           WINDAT *windat,
                                                           const char *name)) {
  csound->csoundMakeGraphCallback_ = makeGraphCB;
}

 void csoundSetDrawGraphCallback(
    CSOUND *csound, void (*drawGraphCallback)(CSOUND *csound, WINDAT *windat)) {
  csound->csoundDrawGraphCallback_ = drawGraphCallback;
}

 void csoundSetKillGraphCallback(
    CSOUND *csound, void (*killGraphCallback)(CSOUND *csound, WINDAT *windat)) {
  csound->csoundKillGraphCallback_ = killGraphCallback;
}

 void csoundSetExitGraphCallback(CSOUND *csound,
                                       int32_t (*exitGraphCallback)(CSOUND *)) {
  csound->csoundExitGraphCallback_ = exitGraphCallback;
}

/*
 * OPCODES
 */
static CS_NOINLINE int32_t opcode_list_new_oentry(CSOUND *csound,
                                                  const OENTRY *ep) {
  CONS_CELL *head;
  OENTRY *entryCopy;
  char *shortName;
  int32_t ret = 0;

  if (UNLIKELY(ep->opname == NULL || csound->opcodes == NULL))
    return CSOUND_ERROR;

  shortName = get_opcode_short_name(csound, ep->opname);
  head = cs_hash_table_get(csound, csound->opcodes, shortName);
  entryCopy = csound->Malloc(csound, sizeof(OENTRY));
  memcpy(entryCopy, ep, sizeof(OENTRY));
  entryCopy->useropinfo = NULL;

  if (head != NULL) {
    cs_cons_append(head, cs_cons(csound, entryCopy, NULL));
  } else {
    head = cs_cons(csound, entryCopy, NULL);
    cs_hash_table_put(csound, csound->opcodes, shortName, head);
  }

  if (shortName != ep->opname) {
    csound->Free(csound, shortName);
  }

  return ret;
}

 int32_t csoundAppendOpcode(CSOUND *csound,
                              const char *opname, size_t dsblksiz, int32_t flags,
                              const char *outypes, const char *intypes,
                              int32_t (*init)(CSOUND *, void *),
                              int32_t (*perf)(CSOUND *, void *),
                              int32_t (*deinit)(CSOUND *, void *))
{
  OENTRY  tmpEntry = {0};
  int32_t     err;
  tmpEntry.opname     = (char*) opname;
  tmpEntry.dsblksiz   =  dsblksiz;
  tmpEntry.flags      =  flags;
  tmpEntry.outypes    = (char*) outypes;
  tmpEntry.intypes    = (char*) intypes;
  tmpEntry.init     = init;
  tmpEntry.perf     = perf;
  tmpEntry.deinit     = deinit;
  err = opcode_list_new_oentry(csound, &tmpEntry);
  // add_to_symbtab(csound, &tmpEntry);
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

int32_t csoundAppendOpcodes(CSOUND *csound, const OENTRY *opcodeList,
                            int32_t n) {
  OENTRY *ep = (OENTRY *)opcodeList;
  int32_t err, retval = 0;

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

static int32_t csoundDeprecate(CSOUND *csound, char *name,
                        char *o, char *i, int32_t deprec) {
  OENTRY *e = (OENTRY *)
    csound->FindOpcode(csound, 1, name, o, i);
  if(e) {
    e->deprecated = deprec;
    return CSOUND_SUCCESS;
  } else return CSOUND_ERROR;
}

 int32_t csoundGetModule(CSOUND *csound, int32_t no, char **module,
                               char **type) {
  MODULE_INFO **modules =
      (MODULE_INFO **)csoundQueryGlobalVariable(csound, "_MODULES");
  if (UNLIKELY(modules[no] == NULL || no >= MAX_MODULES))
    return CSOUND_ERROR;
  *module = modules[no]->module;
  *type = modules[no]->type;
  return CSOUND_SUCCESS;
}

 int32_t csoundLoadPlugins(CSOUND *csound, const char *dir) {
  if (dir != NULL) {
    csound->Message(csound, "loading plugins from %s\n", dir);
    int32_t err = csoundLoadAndInitModules(csound, dir);
    if (!err) {
      return CSOUND_SUCCESS;
    } else
      return err;
  } else
    return CSOUND_ERROR;
}

/**
 * Register a function to be called by csoundReset(), in reverse order
 * of registration, before unloading external modules. The function takes
 * the Csound instance pointer as the first argument, and the pointer
 * passed here as 'userData' as the second, and is expected to return zero
 * on success.
 * The return value of csoundRegisterResetCallback() is zero on success.
 */
typedef struct resetCallback_s {
  void *userData;
  int32_t (*func)(CSOUND *, void *);
  struct resetCallback_s *nxt;
} resetCallback_t;

int32_t csoundRegisterResetCallback(CSOUND *csound, void *userData,
                                    int32_t (*func)(CSOUND *, void *)) {
  resetCallback_t *dp = (resetCallback_t *)malloc(sizeof(resetCallback_t));

  if (UNLIKELY(dp == NULL))
    return CSOUND_MEMORY;
  dp->userData = userData;
  dp->func = func;
  dp->nxt = csound->reset_list;
  csound->reset_list = (void *)dp;
  return CSOUND_SUCCESS;
}

static void reset(CSOUND *csound) {
  CSOUND *saved_env;
  void *p1, *p2;
  uintptr_t length;
  uintptr_t end, start;
  int32_t n = 0;

  // stop multicore threads if still running
  if (csound->oparms->numThreads > 1) {
    ATOMIC_SET(csound->multiThreadedComplete, 1);
#ifdef PARCS_USE_LOCK_BARRIER
    csound->WaitBarrier(csound->barrier1);
#else
    ATOMIC_SET(csound->parflag,!csound->parflag);
#endif
  }
  csound_cleanup(csound);
  /* call registered reset callbacks */
  while (csound->reset_list != NULL) {
    resetCallback_t *p = (resetCallback_t *)csound->reset_list;
    p->func(csound, p->userData);
    csound->reset_list = (void *)p->nxt;
    free(p);
  }
  /* call local destructor routines of external modules */
  /* should check return value... */
  csoundDestroyModules(csound);

  /* IV - Feb 01 2005: clean up configuration variables and */
  /* named dynamic "global" variables of Csound instance */
  csoundDeleteAllConfigurationVariables(csound);
  csoundDeleteAllGlobalVariables(csound);

  if (csound->opcodes != NULL) {
    free_opcode_table(csound);
    csound->opcodes = NULL;
  }

  csound->oparms_.odebug = 0;
  /* RWD 9:2000 not terribly vital, but good to do this somewhere... */
  pvsys_release(csound);
  close_all_files(csound);
  /* delete temporary files created by this Csound instance */
  remove_tmpfiles(csound);
  free_memfiles(csound);

  while (csound->filedir[n]) /* Clear source directory */
    csound->Free(csound, csound->filedir[n++]);

  memreset(csound);

  /**
   * Copy everything EXCEPT the function pointers.
   * We do it by saving them and copying them back again...
   * hope that this does not fail...
   */
  /* VL 07.06.2013 - check if the status is COMP before
     resetting.
  */
  // CSOUND **self = csound->self;
  saved_env = (CSOUND *)malloc(sizeof(CSOUND));
  memcpy(saved_env, csound, sizeof(CSOUND));
  memcpy(csound, &cenviron_, sizeof(CSOUND));
  end = (uintptr_t) & (csound->first_callback_); /* used to be &(csound->ids) */
  start = (uintptr_t)csound;
  length = end - start;
  memcpy((void *)csound, (void *)saved_env, (size_t)length);
  csound->oparms = &(csound->oparms_);
  csound->hostdata = saved_env->hostdata;
  csound->opcodedir = saved_env->opcodedir;
  p1 = (void *)&(csound->first_callback_);
  p2 = (void *)&(csound->last_callback_);
  length = (uintptr_t)p2 - (uintptr_t)p1;
  memcpy(p1, (void *)&(saved_env->first_callback_), (size_t)length);
  csound->csoundCallbacks_ = saved_env->csoundCallbacks_;
  csound->API_lock = saved_env->API_lock;
#ifdef HAVE_PTHREAD_SPIN_LOCK
  csound->memlock = saved_env->memlock;
  csound->spinlock = saved_env->spinlock;
  csound->spoutlock = saved_env->spoutlock;
  csound->spinlock1 = saved_env->spinlock1;
#endif
  csound->enableHostImplementedMIDIIO = saved_env->enableHostImplementedMIDIIO;
  memcpy(&(csound->exitjmp), &(saved_env->exitjmp), sizeof(jmp_buf));
  csound->memalloc_db = saved_env->memalloc_db;
  csound->message_buffer =
      saved_env->message_buffer; /*VL 19.06.21 keep msg buffer */
  // csound->self = self;
  free(saved_env);
}

 void csoundReset(CSOUND *csound) {
  int32_t i;
  OPARMS *O = csound->oparms;

  if (csound->engineStatus & CS_STATE_COMP ||
      csound->engineStatus & CS_STATE_PRE) {
    /* and reset */
    csound->Message(csound, "resetting Csound instance\n");
    reset(csound);
    /* clear compiled flag */
    csound->engineStatus |= ~(CS_STATE_COMP);
  } else {
    csoundSpinLockInit(&csound->spoutlock);
    csoundSpinLockInit(&csound->spinlock);
    csoundSpinLockInit(&csound->memlock);
    csoundSpinLockInit(&csound->spinlock1);
    if (UNLIKELY(O->odebug))
      csound->Message(csound, "init spinlocks\n");
  }

  if (msgcallback_ != NULL) {
    csoundSetMessageCallback(csound, msgcallback_);
  } else {
    csoundSetMessageCallback(csound, csoundDefaultMessageCallback);
  }
  csound->printerrormessagesflag = (void *)1234;
  /* copysystem environment variables */
  i = csoundInitEnv(csound);
  if (UNLIKELY(i != CSOUND_SUCCESS)) {
    csound->engineStatus |= CS_STATE_JMP;
    csound->Die(csound, Str("Failed during csoundInitEnv"));
  }
  csound_init_rand(csound);
  csound->engineState.stringPool = cs_hash_table_create(csound);
  csound->engineState.constantsPool = cs_hash_table_create(csound);
  csound->engineStatus |= CS_STATE_PRE;
  csound_aops_init_tables(csound);
  create_opcode_table(csound);

  /* now load and pre-initialise external modules for this instance */
  /* this function returns an error value that may be worth checking */
  {
    int32_t err;
#ifndef BUILD_PLUGINS
    err = csoundInitStaticModules(csound);
    if (csound->delayederrormessages &&
        csound->printerrormessagesflag == NULL) {
      csound->Warning(csound, "%s", csound->delayederrormessages);
      csound->Free(csound, csound->delayederrormessages);
      csound->delayederrormessages = NULL;
    }
    if (UNLIKELY(err == CSOUND_ERROR))
      csound->Die(csound, Str("Failed during csoundInitStaticModules"));
#endif
#ifndef BARE_METAL
    csoundCreateGlobalVariable(csound, "_MODULES",
                               (size_t)MAX_MODULES * sizeof(MODULE_INFO *));
    char *modules = (char *)csoundQueryGlobalVariable(csound, "_MODULES");
    memset(modules, 0, sizeof(MODULE_INFO *) * MAX_MODULES);

    err = csoundLoadModules(csound);
    if (csound->delayederrormessages &&
        csound->printerrormessagesflag == NULL) {
      csound->Warning(csound, "%s", csound->delayederrormessages);
      csound->Free(csound, csound->delayederrormessages);
      csound->delayederrormessages = NULL;
    }
    if (UNLIKELY(err != CSOUND_SUCCESS))
      csound->Die(csound, Str("Failed during csoundLoadModules"));

    if (csoundInitModules(csound) != 0)
      csound->LongJmp(csound, 1);

#endif // BARE_METAL
    init_pvsys(csound);
    /* utilities depend on this as well as orchs; may get changed by an orch */
    dbfs_init(csound, DFLT_DBFS);
    csound->csRtClock = (RTCLOCK *)csound->Calloc(csound, sizeof(RTCLOCK));
    csoundInitTimerStruct(csound->csRtClock);
    csound->engineStatus |= /*CS_STATE_COMP |*/ CS_STATE_CLN;

    /* do not know file type yet */
    O->filetyp = -1;
    csound->peakchunks = 1;
    csound->typePool = csound->Calloc(csound, sizeof(TYPE_POOL));
    csound->engineState.varPool = csoundCreateVarPool(csound);
    add_standard_types(csound, csound->typePool);
    /* csoundLoadExternals(csound); */
  }
  int32_t max_len = 21;
  char *s;

#ifndef BARE_METAL
  /* allow selecting real time audio module */
  csoundCreateGlobalVariable(csound, "_RTAUDIO", (size_t)max_len);
  s = csoundQueryGlobalVariable(csound, "_RTAUDIO");
#ifndef LINUX
#ifdef __HAIKU__
  strcpy(s, "haiku");
#else
#ifdef __MACH__
  strcpy(s, "auhal");
#else
  strcpy(s, "PortAudio");
#endif
#endif
#else
  strcpy(s, "alsa");
#endif

  csoundCreateConfigurationVariable(csound, "rtaudio", s, CSOUNDCFG_STRING, 0,
                                    NULL, &max_len,
                                    Str("Real time audio module name"), NULL);
#endif
  /* initialise real time MIDI */
  csound->midiGlobals = (MGLOBAL *)csound->Calloc(csound, sizeof(MGLOBAL));
  csound->midiGlobals->bufp = &(csound->midiGlobals->mbuf[0]);
  csound->midiGlobals->endatp = csound->midiGlobals->bufp;
  csoundCreateGlobalVariable(csound, "_RTMIDI", (size_t)max_len);
  csound->SetMIDIDeviceListCallback(csound, midi_dev_list_dummy);
  csound->SetExternalMidiInOpenCallback(csound, DummyMidiInOpen);
  csound->SetExternalMidiReadCallback(csound, DummyMidiRead);
  csound->SetExternalMidiOutOpenCallback(csound, DummyMidiOutOpen);
  csound->SetExternalMidiWriteCallback(csound, DummyMidiWrite);

  s = csoundQueryGlobalVariable(csound, "_RTMIDI");
  strcpy(s, "null");
  if (csound->enableHostImplementedMIDIIO == 0)
#ifndef LINUX
#ifdef __HAIKU__
    strcpy(s, "haiku");
#else
    strcpy(s, "portmidi");
#endif
#else
    strcpy(s, "alsa");
#endif
  else
    strcpy(s, "hostbased");

  csoundCreateConfigurationVariable(csound, "rtmidi", s, CSOUNDCFG_STRING, 0,
                                    NULL, &max_len,
                                    Str("Real time MIDI module name"), NULL);
  max_len = 256; /* should be the same as in csoundCore.h */
  csoundCreateConfigurationVariable(
      csound, "mute_tracks", &(csound->midiGlobals->muteTrackList[0]),
      CSOUNDCFG_STRING, 0, NULL, &max_len,
      Str("Ignore events (other than tempo "
          "changes) in tracks defined by pattern"),
      NULL);
  csoundCreateConfigurationVariable(csound, "raw_controller_mode",
                                    &(csound->midiGlobals->rawControllerMode),
                                    CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                    Str("Do not handle special MIDI controllers"
                                        " (sustain pedal etc.)"),
                                    NULL);
#ifndef BARE_METAL
  /* sound file tag options */
  max_len = 201;
  i = (max_len + 7) & (~7);
  csound->SF_id_title = (char *)csound->Calloc(csound, (size_t)i * (size_t)6);
  csoundCreateConfigurationVariable(csound, "id_title", csound->SF_id_title,
                                    CSOUNDCFG_STRING, 0, NULL, &max_len,
                                    Str("Title tag in output soundfile "
                                        "(no spaces)"),
                                    NULL);
  csound->SF_id_copyright = (char *)csound->SF_id_title + (int32_t)i;
  csoundCreateConfigurationVariable(csound, "id_copyright",
                                    csound->SF_id_copyright, CSOUNDCFG_STRING,
                                    0, NULL, &max_len,
                                    Str("Copyright tag in output soundfile"
                                        " (no spaces)"),
                                    NULL);
  csoundCreateConfigurationVariable(csound, "id_scopyright",
                                    &csound->SF_id_scopyright,
                                    CSOUNDCFG_INTEGER, 0, NULL, &max_len,
                                    Str("Short Copyright tag in"
                                        " output soundfile"),
                                    NULL);
  csound->SF_id_software = (char *)csound->SF_id_copyright + (int32_t)i;
  csoundCreateConfigurationVariable(csound, "id_software",
                                    csound->SF_id_software, CSOUNDCFG_STRING, 0,
                                    NULL, &max_len,
                                    Str("Software tag in output soundfile"
                                        " (no spaces)"),
                                    NULL);
  csound->SF_id_artist = (char *)csound->SF_id_software + (int32_t)i;
  csoundCreateConfigurationVariable(csound, "id_artist", csound->SF_id_artist,
                                    CSOUNDCFG_STRING, 0, NULL, &max_len,
                                    Str("Artist tag in output soundfile "
                                        "(no spaces)"),
                                    NULL);
  csound->SF_id_comment = (char *)csound->SF_id_artist + (int32_t)i;
  csoundCreateConfigurationVariable(csound, "id_comment", csound->SF_id_comment,
                                    CSOUNDCFG_STRING, 0, NULL, &max_len,
                                    Str("Comment tag in output soundfile"
                                        " (no spaces)"),
                                    NULL);
  csound->SF_id_date = (char *)csound->SF_id_comment + (int32_t)i;
  csoundCreateConfigurationVariable(csound, "id_date", csound->SF_id_date,
                                    CSOUNDCFG_STRING, 0, NULL, &max_len,
                                    Str("Date tag in output soundfile "
                                        "(no spaces)"),
                                    NULL);
  {
    MYFLT minValF = FL(0.0);

    csoundCreateConfigurationVariable(csound, "msg_color",
                                      &(csound->enableMsgAttr),
                                      CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                      Str("Enable message attributes "
                                          "(colors etc.)"),
                                      NULL);
    csoundCreateConfigurationVariable(
        csound, "skip_seconds", &(csound->csoundScoreOffsetSeconds_),
        CSOUNDCFG_MYFLT, 0, &minValF, NULL,
        Str("Start score playback at the specified"
            " time, skipping earlier events"),
        NULL);
  }
  csoundCreateConfigurationVariable(csound, "ignore_csopts",
                                    &(csound->disable_csd_options),
                                    CSOUNDCFG_BOOLEAN, 0, NULL, NULL,
                                    Str("Ignore <CsOptions> in CSD files"
                                        " (default: no)"),
                                    NULL);
#endif
}

 int32_t csoundGetDebug(CSOUND *csound) {
  return csound->oparms_.odebug;
}

 void csoundSetDebug(CSOUND *csound, int32_t debug) {
  csound->oparms_.odebug = debug;
}

 int32_t csoundTableLength(CSOUND *csound, int32_t table) {
  MYFLT *tablePtr;
  return csoundGetTable(csound, &tablePtr, table);
}


static int32_t csoundDoCallback_(CSOUND *csound, void *p, uint32_t type) {
  if (csound->csoundCallbacks_ != NULL) {
    CsoundCallbackEntry_t *pp;
    pp = (CsoundCallbackEntry_t *)csound->csoundCallbacks_;
    do {
      if (pp->typeMask & type) {
        int32_t retval = pp->func(pp->userData, p, type);
        if (retval != CSOUND_SUCCESS)
          return retval;
      }
      pp = pp->nxt;
    } while (pp != (CsoundCallbackEntry_t *)NULL);
  }
  return 1;
}

/**
 * Sets a callback function that will be called on keyboard
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
 *   uint32_t type
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

 int32_t csoundRegisterKeyboardCallback(
    CSOUND *csound, int32_t (*func)(void *userData, void *p, uint32_t type),
    void *userData, uint32_t typeMask) {
  CsoundCallbackEntry_t *pp;

  if (UNLIKELY(func == (int32_t(*)(void *, void *, uint32_t))NULL ||
               (typeMask & (~(CSOUND_CALLBACK_KBD_EVENT |
                              CSOUND_CALLBACK_KBD_TEXT))) != 0U))
    return CSOUND_ERROR;
  csoundRemoveKeyboardCallback(csound, func);
  pp = (CsoundCallbackEntry_t *)malloc(sizeof(CsoundCallbackEntry_t));
  if (UNLIKELY(pp == (CsoundCallbackEntry_t *)NULL))
    return CSOUND_MEMORY;
  pp->typeMask = (typeMask ? typeMask : 0xFFFFFFFFU);
  pp->nxt = (CsoundCallbackEntry_t *)csound->csoundCallbacks_;
  pp->userData = userData;
  pp->func = func;
  csound->csoundCallbacks_ = (void *)pp;

  return CSOUND_SUCCESS;
}

/**
 * Removes a callback previously set with csoundSetCallback().
 */

 void csoundRemoveKeyboardCallback(CSOUND *csound,
                                         int32_t (*func)(void *, void *,
                                                         uint32_t)) {
  CsoundCallbackEntry_t *pp, *prv;

  pp = (CsoundCallbackEntry_t *)csound->csoundCallbacks_;
  prv = (CsoundCallbackEntry_t *)NULL;
  while (pp != (CsoundCallbackEntry_t *)NULL) {
    if (pp->func == func) {
      if (prv != (CsoundCallbackEntry_t *)NULL)
        prv->nxt = pp->nxt;
      else
        csound->csoundCallbacks_ = (void *)pp->nxt;
      free((void *)pp);
      return;
    }
    prv = pp;
    pp = pp->nxt;
  }
}

 void csoundSetOpenSoundFileCallback(
    CSOUND *p,
    void *(*openSoundFileCallback)(CSOUND *, const char *, int32_t, void *)) {
  p->OpenSoundFileCallback_ = openSoundFileCallback;
}

 void csoundSetOpenFileCallback(CSOUND *p,
                                      FILE *(*openFileCallback)(CSOUND *,
                                                                const char *,
                                                                const char *)) {
  p->OpenFileCallback_ = openFileCallback;
}

void csoundSetFileOpenCallback(CSOUND *p,
                               void (*fileOpenCallback)(CSOUND *, const char *,
                                                        int32_t, int32_t,
                                                        int32_t)) {
  p->FileOpenCallback_ = fileOpenCallback;
}

/* csoundNotifyFileOpened() should be called by plugins via
   csound->NotifyFileOpened() to let Csound know that they opened a file
   without using one of the standard mechanisms (csound->FileOpen() or
   csoundLoadMemoryfile()).  The notification is passed on to the host if it
   has set the FileOpen callback. */
void csoundNotifyFileOpened(CSOUND *csound, const char *pathname,
                            int32_t csFileType, int32_t writing,
                            int32_t temporary) {
  if (csound->FileOpenCallback_ != NULL)
    csound->FileOpenCallback_(csound, pathname, csFileType, writing, temporary);
  return;
}

/**
 * Return the size of MYFLT in bytes.
 */
int32_t csoundGetSizeOfMYFLT(void) {
  return (int32_t)sizeof(MYFLT);
}

/**
 * Returns the name of the opcode of which the data structure
 * is pointed to by 'p'.
 */
char *csoundGetOpcodeName(void *p) {
  return ((OPDS *)p)->optext->t.oentry->opname;
}

/** Returns the CS_TYPE for an opcode's arg pointer */

CS_TYPE *csoundGetTypeForArg(void *argPtr) {
  char *ptr = (char *)argPtr;
  CS_TYPE *varType = *(CS_TYPE **)(ptr - CS_VAR_TYPE_OFFSET);
  return varType;
}

/**
 * Returns the number of input arguments for opcode 'p'.
 */
int32_t csoundGetInputArgCnt(void *p) {
  return (int32_t)((OPDS *)p)->optext->t.inArgCount;
}

/**
 * Returns the name of input argument 'n' (counting from 0) for opcode 'p'.
 */
char *csoundGetInputArgName(void *p, int32_t n) {
  if ((uint32_t)n >= (uint32_t)((OPDS *)p)->optext->t.inArgCount)
    return (char *)NULL;
  return (char *)((OPDS *)p)->optext->t.inlist->arg[n];
}

/**
 * Returns the number of output arguments for opcode 'p'.
 */
int32_t csoundGetOutputArgCnt(void *p) {
  return (int32_t)((OPDS *)p)->optext->t.outArgCount;
}

/**
 * Returns the name of output argument 'n' (counting from 0) for opcode 'p'.
 */
char *csoundGetOutputArgName(void *p, int32_t n) {
  if ((uint32_t)n >= (uint32_t)((OPDS *)p)->optext->t.outArgCount)
    return (char *)NULL;
  return (char *)((OPDS *)p)->optext->t.outlist->arg[n];
}

/**
 * Set release time in control periods (1 / csound->ekr second units)
 * for opcode 'p' to 'n'. If the current release time is longer than
 * the specified value, it is not changed.
 * Returns the new release time.
 */
int32_t csoundSetReleaseLength(void *p, int32_t n) {
  if (n > (int32_t)((OPDS *)p)->insdshead->xtratim)
    ((OPDS *)p)->insdshead->xtratim = n;
  return (int32_t)((OPDS *)p)->insdshead->xtratim;
}

/**
 * Set release time in seconds for opcode 'p' to 'n'.
 * If the current release time is longer than the specified value,
 * it is not changed.
 * Returns the new release time in seconds.
 */
MYFLT csoundSetReleaseLengthSeconds(void *p, MYFLT n) {
  int32_t kcnt = (int32_t)(n * ((OPDS *)p)->insdshead->csound->ekr + FL(0.5));
  if (kcnt > (int32_t)((OPDS *)p)->insdshead->xtratim)
    ((OPDS *)p)->insdshead->xtratim = kcnt;
  return ((MYFLT)((OPDS *)p)->insdshead->xtratim *
          ((OPDS *)p)->insdshead->csound->onedkr);
}


