/*
    csoundCore.h:

    Copyright (C) 1991-2006 Barry Vercoe, John ffitch, Istvan Varga

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

/*
    csoundCore.h:

    Copyright (C) 1991-2006 Barry Vercoe, John ffitch, Istvan Varga

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

#pragma once

#if defined(__EMSCRIPTEN__) && !defined(EMSCRIPTEN)
#define EMSCRIPTEN
#endif

#include <setjmp.h>                  // for jmp_buf
#include <stdarg.h>                  // for va_list
#include <stdint.h>                  // for uint32_t, int64_t, uint64_t
#include <stdio.h>                   // for FILE

#include "cs_par_structs.h"          // for watchList, stateWithPadding
#include "cscore.h"                  // for EVENT
#include "csound.h"                  // for CSOUND, WINDAT, CsoundRandMTState
#include "csoundCore_common.h"       // for MAXCHNLS, CORFIL, EVTBLK, INSDS
#include "csound_data_structures.h"  // for CS_HASH_TABLE
#include "csound_type_system.h"      // for TYPE_POOL
#include "remote.h"                  // for REMOT_BUF
#include "sort.h"                    // for SRTBLK
#include "sysdep.h"                  // for MYFLT, int32, uint32, int16, spi...

// not needed TODO: remove
#include "csound_standard_types.h"
#include "envvar.h"
#include "prototyp.h"
#include "soundfile.h"

/* VL not sure if we need to check for SSE */
#if defined(__SSE__) && !defined(EMSCRIPTEN)
#include <xmmintrin.h>
#ifndef _MM_DENORMALS_ZERO_ON
#define _MM_DENORMALS_ZERO_MASK 0x0040
#define _MM_DENORMALS_ZERO_ON 0x0040
#define _MM_DENORMALS_ZERO_OFF 0x0000
#define _MM_SET_DENORMALS_ZERO_MODE(mode)                                      \
  _mm_setcsr((_mm_getcsr() & ~_MM_DENORMALS_ZERO_MASK) | (mode))
#define _MM_GET_DENORMALS_ZERO_MODE() (_mm_getcsr() & _MM_DENORMALS_ZERO_MASK)
#endif
#else
#ifndef _MM_DENORMALS_ZERO_ON
#define _MM_DENORMALS_ZERO_MASK 0
#define _MM_DENORMALS_ZERO_ON 0
#define _MM_DENORMALS_ZERO_OFF 0
#define _MM_SET_DENORMALS_ZERO_MODE(mode)
#endif
#endif

#define INSTR 1
#define ENDIN 2
#define OPCODE 3
#define ENDOP 4
#define LABEL 5
#define SETBEG 6
#define PSET 6
#define USEROPCODE 7
#define SETEND 8

#define TOKMAX 50L /* Should be 50 but bust */

/* max number of input/output args for user defined opcodes */
#define OPCODENUMOUTS_LOW 16
#define OPCODENUMOUTS_HIGH 64
#define OPCODENUMOUTS_MAX 256

#define MBUFSIZ (4096)
#define MIDIINBUFMAX (1024)
#define MIDIINBUFMSK (MIDIINBUFMAX - 1)

#define MIDIMAXPORTS (64)

typedef union {
  uint32 dwData;
  unsigned char bData[4];
} MIDIMESSAGE;

/* MIDI globals */

typedef struct midiglobals {
  MEVENT *Midevtblk;
  int sexp;
  int MIDIoutDONE;
  int MIDIINbufIndex;
  MIDIMESSAGE MIDIINbuffer2[MIDIINBUFMAX];
  int (*MidiInOpenCallback)(CSOUND *, void **, const char *);
  int (*MidiReadCallback)(CSOUND *, void *, unsigned char *, int);
  int (*MidiInCloseCallback)(CSOUND *, void *);
  int (*MidiOutOpenCallback)(CSOUND *, void **, const char *);
  int (*MidiWriteCallback)(CSOUND *, void *, const unsigned char *, int);
  int (*MidiOutCloseCallback)(CSOUND *, void *);
  const char *(*MidiErrorStringCallback)(int);
  void *midiInUserData;
  void *midiOutUserData;
  void *midiFileData;
  void *midiOutFileData;
  int rawControllerMode;
  char muteTrackList[256];
  unsigned char mbuf[MBUFSIZ];
  unsigned char *bufp, *endatp;
  int16 datreq, datcnt;
} MGLOBAL;

typedef struct eventnode {
  struct eventnode *nxt;
  uint32 start_kcnt;
  EVTBLK evt;
} EVTNODE;

typedef struct {
  OPDS h;
  MYFLT *ktempo, *istartempo;
  MYFLT prvtempo;
} TEMPO;

/* typedef struct token { */
/*   char    *str; */
/*   int16   prec; */
/* } TOKEN; */

typedef struct names {
  char *mac;
  struct names *next;
} NAMES;

typedef struct threadInfo {
  struct threadInfo *next;
  void *threadId;
} THREADINFO;

#define CS_STATE_PRE (1)
#define CS_STATE_COMP (2)
#define CS_STATE_UTIL (4)
#define CS_STATE_CLN (8)
#define CS_STATE_JMP (16)

/* These are used to set/clear bits in csound->tempStatus.
   If the bit is set, it indicates that the given file is
   a temporary. */
extern const uint32_t csOrcMask;
extern const uint32_t csScoInMask;
extern const uint32_t csScoSortMask;
extern const uint32_t csMidiScoMask;
extern const uint32_t csPlayScoMask;

#ifdef PARCS
int nodePerf(CSOUND *csound, int index, int numThreads);
#endif

/* kperf function protoypes. Used by the debugger to switch between debug
 * and nodebug kperf functions */
int kperf_nodebug(CSOUND *csound);
int kperf_debug(CSOUND *csound);

/*
  check if code is running at init time.
  result may not be valid in realtime mode
 */
int csoundIsInitThread(CSOUND *csound);

#define NAMELEN 40  /* array size of repeat macro names */
#define RPTDEPTH 40 /* size of repeat_n arrays (39 loop levels) */
#define LBUFSIZ 32768

/* ------- private data (not to be used by hosts or externals) ------- */
/** @name Private Data
  Private Data in the CSOUND struct to be used internally by the Csound
  library and should be hidden from plugins.
  If a new variable member is needed by the library, add it below, as a
  private data member. If access is required solely by plugins (and not
  internally by the library), use the CreateGlobalVariable() etc. interface,
  instead of adding to CSOUND.

  If you find that a plugin needs to access existing private data,
  first check above for an existing interface; if none is available,
  add one. Please avoid giving full access, or allowing plugins to
  change the values of private members, by using one of the two methods
  below:

  1) To get the data member value:
  \code
     returnType (*GetVar)(CSOUND *)
  \endcode
  2) in case of pointers, data should be copied out to a supplied memory
     slot, rather than the pointer being obtained:
  \code
     void (*GetData)(CSOUND *, dataType *)

     dataType var;
     csound->GetData(csound, &var);
  \endcode
*/
/**@{ */

/**
 * Contains all function pointers, data, and data pointers required
 * to run one instance of Csound.
 *
 * \b PUBLIC functions in CSOUND_
 * These are used by plugins to access the
 * Csound library functionality without the requirement
 * of compile-time linkage to the csound library
 * New functions only need to be added here if
 * they are required by plugins.
 */
struct CSOUND_ {
  CSOUND_PUBLIC_FIELDS
  SUBR first_callback_;
  channelCallback_t InputChannelCallback_;
  channelCallback_t OutputChannelCallback_;
  void (*csoundMessageCallback_)(CSOUND *, int attr, const char *format,
                                 va_list args);
  int (*csoundConfigureCallback_)(CSOUND *);
  void (*csoundMakeGraphCallback_)(CSOUND *, WINDAT *windat, const char *name);
  void (*csoundDrawGraphCallback_)(CSOUND *, WINDAT *windat);
  void (*csoundKillGraphCallback_)(CSOUND *, WINDAT *windat);
  int (*csoundExitGraphCallback_)(CSOUND *);
  int (*csoundYieldCallback_)(CSOUND *);
  void (*cscoreCallback_)(CSOUND *);
  void (*FileOpenCallback_)(CSOUND *, const char *, int, int, int);
  SUBR last_callback_;
  /* these are not saved on RESET */
  int (*playopen_callback)(CSOUND *, const csRtAudioParams *parm);
  void (*rtplay_callback)(CSOUND *, const MYFLT *outBuf, int nbytes);
  int (*recopen_callback)(CSOUND *, const csRtAudioParams *parm);
  int (*rtrecord_callback)(CSOUND *, MYFLT *inBuf, int nbytes);
  void (*rtclose_callback)(CSOUND *);
  int (*audio_dev_list_callback)(CSOUND *, CS_AUDIODEVICE *, int);
  int (*midi_dev_list_callback)(CSOUND *, CS_MIDIDEVICE *, int);
  int (*doCsoundCallback)(CSOUND *, void *, unsigned int);
  int (*csoundInternalYieldCallback_)(CSOUND *);
  /* end of callbacks */
  void (*spinrecv)(CSOUND *);
  void (*spoutran)(CSOUND *);
  int (*audrecv)(CSOUND *, MYFLT *, int);
  void (*audtran)(CSOUND *, const MYFLT *, int);
  void *hostdata;
  char *orchname, *scorename;
  CORFIL *orchstr, *scorestr;
  OPDS *ids;                /* used by init loops */
  ENGINE_STATE engineState; /* current Engine State merged after
                               compilation */
  INSTRTXT *instr0;         /* instr0     */
  INSTRTXT **dead_instr_pool;
  int dead_instr_no;
  TYPE_POOL *typePool;
  unsigned int ksmps;
  uint32_t nchnls;
  int inchnls;
  uint64_t kcounter, global_kcounter;
  MYFLT esr;
  MYFLT ekr;
  /** current time in seconds, inc. per kprd */
  int64_t icurTime; /* Current time in samples */
  double curTime_inc;
  /** start time of current section    */
  double timeOffs, beatOffs;
  /** current time in beats, inc per kprd */
  double curBeat, curBeat_inc;
  /** beat time = 60 / tempo           */
  int64_t ibeatTime; /* Beat time in samples */
  EVTBLK *currevent;
  INSDS *curip;
  MYFLT cpu_power_busy;
  char *xfilename;
  int peakchunks;
  int keep_tmp;
  CS_HASH_TABLE *opcodes;
  int32 nrecs;
  FILE *Linepipe;
  int Linefd;
  void *csoundCallbacks_;
  FILE *scfp;
  CORFIL *scstr;
  FILE *oscfp;
  MYFLT maxamp[MAXCHNLS];
  MYFLT smaxamp[MAXCHNLS];
  MYFLT omaxamp[MAXCHNLS];
  uint32 maxpos[MAXCHNLS], smaxpos[MAXCHNLS], omaxpos[MAXCHNLS];
  FILE *scorein;
  FILE *scoreout;
  int *argoffspace;
  INSDS *frstoff;
  /** reserved for std opcode library  */
  void *stdOp_Env;
  int holdrand;
  int randSeed1;
  int randSeed2;
  CsoundRandMTState *csRandState;
  RTCLOCK *csRtClock;
  int strsmax;
  char **strsets;
  MYFLT *spin;
  MYFLT *spout;
  MYFLT *spout_tmp;
  int nspin;
  int nspout;
  MYFLT *auxspin;
  OPARMS *oparms;
  /** reserve space for up to MIDIMAXPORTS MIDI devices */
  MCHNBLK *m_chnbp[MIDIMAXPORTS * 16];
  int dither_output;
  MYFLT onedsr, sicvt;
  MYFLT tpidsr, pidsr, mpidsr, mtpdsr;
  MYFLT onedksmps;
  MYFLT onedkr;
  MYFLT kicvt;
  int reinitflag;
  int tieflag;
  MYFLT e0dbfs, dbfs_to_float;
  double A4;
  void *rtRecord_userdata;
  void *rtPlay_userdata;
  jmp_buf exitjmp;
  SRTBLK *frstbp;
  int sectcnt;
  int inerrcnt, synterrcnt, perferrcnt;
  INSDS actanchor;
  int32 rngcnt[MAXCHNLS];
  int16 rngflg, multichan;
  void *evtFuncChain;
  EVTNODE *OrcTrigEvts; /* List of events to be started */
  EVTNODE *freeEvtNodes;
  int csoundIsScorePending_;
  int64_t advanceCnt;
  int initonly;
  int evt_poll_cnt;
  int evt_poll_maxcnt;
  int Mforcdecs, Mxtroffs, MTrkend;
  OPCODINFO *opcodeInfo;
  FUNC **flist;
  int maxfnum;
  GEN *gensub;
  int genmax;
  CS_HASH_TABLE *namedGlobals;
  CS_HASH_TABLE *cfgVariableDB;
  double prvbt, curbt, nxtbt;
  double curp2, nxtim;
  int64_t cyclesRemaining;
  EVTBLK evt;
  void *memalloc_db;
  MGLOBAL *midiGlobals;
  CS_HASH_TABLE *envVarDB;
  MEMFIL *memfiles;
  PVOCEX_MEMFILE *pvx_memfiles;
  int FFT_max_size;
  void *FFT_table_1;
  void *FFT_table_2;
  /* statics from twarp.c should be TSEG* */
  void *tseg, *tpsave;
  /* persistent macros */
  MACRO *orc_macros;
  /* Statics from express.c */
  MYFLT *gbloffbas; /* was static in oload.c */
  void *file_io_thread;
  int file_io_start;
  void *file_io_threadlock;
  int realtime_audio_flag;
  void *event_insert_thread;
  int event_insert_loop;
  void *init_pass_threadlock;
  void *API_lock;
  spin_lock_t spoutlock, spinlock;
  spin_lock_t memlock, spinlock1;
  char *delayederrormessages;
  void *printerrormessagesflag;
  struct sread__ {
    SRTBLK *bp, *prvibp;          /* current srtblk,  prev w/same int(p1) */
    char *sp, *nxp;               /* string pntrs into srtblk text        */
    int op;                       /* opcode of current event              */
    int warpin;                   /* input format sensor                  */
    int linpos;                   /* line position sensor                 */
    int lincnt;                   /* count of lines/section in scorefile  */
    MYFLT prvp2 /* = -FL(1.0) */; /* Last event time                  */
    MYFLT clock_base /* = FL(0.0) */;
    MYFLT warp_factor /* = FL(1.0) */;
    char *curmem;
    char *memend; /* end of cur memblk                    */
    MACRO *unused_ptr2;
    int last_name /* = -1 */;
    IN_STACK *inputs, *str;
    int input_size, input_cnt;
    int unused_int3;
    int unused_int2;
    int linepos /* = -1 */;
    MARKED_SECTIONS names[30];
    char unused_char0[RPTDEPTH][NAMELEN];
    int unused_int4[RPTDEPTH];
    int32 unused_int7[RPTDEPTH];
    int unused_int5;
    MACRO *unused_ptr0[RPTDEPTH];
    int unused_int6;
    /* Variable for repeat sections */
    char unused_char1[NAMELEN];
    int unused_int8;
    int32 unused_int9;
    int unused_intA;
    MACRO *unused_ptr1;
    int nocarry;
  } sread;
  struct onefileStatics__ {
    NAMELST *toremove;
    char *orcname;
    char *sconame;
    char *midname;
    int midiSet;
    int csdlinecount;
  } onefileStatics;
  struct lineventStatics__ {
    char *Linep, *Linebufend;
    int stdmode;
    EVTBLK prve;
    char *Linebuf;
    int linebufsiz;
    char *orchestra, *orchestrab;
    int oflag;
  } lineventStatics;
  struct musmonStatics__ {
    int32 srngcnt[MAXCHNLS], orngcnt[MAXCHNLS];
    int16 srngflg;
    int16 sectno;
    int lplayed;
    int segamps, sormsg;
    EVENT **ep, **epend; /* pointers for stepping through lplay list */
    EVENT *lsect;
  } musmonStatics;
  struct libsndStatics__ {
    void *outfile;
    void *infile;
    char *sfoutname; /* soundout filename            */
    MYFLT *inbuf;
    MYFLT *outbuf;  /* contin sndio buffers         */
    MYFLT *outbufp; /* MYFLT pntr                   */
    uint32 inbufrem;
    uint32 outbufrem;                 /* in monosamps                 */
                                      /* (see openin, iotranset)      */
    unsigned int inbufsiz, outbufsiz; /* alloc in sfopenin/out        */
    int isfopen;                      /* (real set in sfopenin)       */
    int osfopen;                      /* (real set in sfopenout)      */
    int pipdevin, pipdevout;          /* 0: file, 1: pipe, 2: rtaudio */
    uint32 nframes /* = 1UL */;
    FILE *pin, *pout;
    int dither;
  } libsndStatics;

  int warped; /* rdscor.c */
  int sstrlen;
  char *sstrbuf;
  int enableMsgAttr; /* csound.c */
  int sampsNeeded;
  MYFLT csoundScoreOffsetSeconds_;
  int inChar_;
  int isGraphable_;
  int delayr_stack_depth; /* ugens6.c */
  void *first_delayr;
  void *last_delayr;
  int32 revlpsiz[6];
  int32 revlpsum;
  double rndfrac; /* aops.c */
  MYFLT *logbase2;
  NAMES *omacros, *smacros;
  void *namedgen;   /* fgens.c */
  void *open_files; /* fileopen.c */
  void *searchPathCache;
  CS_HASH_TABLE *sndmemfiles;
  void *reset_list;
  void *pvFileTable; /* pvfileio.c */
  int pvNumFiles;
  int pvErrorCode;
  /* database for deferred loading of opcode plugin libraries */
  /*    void          *pluginOpcodeFiles; */
  int enableHostImplementedAudioIO;
  int enableHostImplementedMIDIIO;
  int hostRequestedBufferSize;
  /* engineStatus is sum of:
   *   1 (CS_STATE_PRE):  csoundPreCompile was called
   *   2 (CS_STATE_COMP): csoundCompile was called
   *   4 (CS_STATE_UTIL): csoundRunUtility was called
   *   8 (CS_STATE_CLN):  csoundCleanup needs to be called
   *  16 (CS_STATE_JMP):  csoundLongJmp was called
   */
  char engineStatus;
  /* stdXX_assign_flags  can be {1,2,4,8} */
  char stdin_assign_flg;
  char stdout_assign_flg;
  char orcname_mode; /* 0: normal, 1: ignore, 2: fail */
  int use_only_orchfile;
  void *csmodule_db;
  char *dl_opcodes_oplibs;
  char *SF_csd_licence;
  char *SF_id_title;
  char *SF_id_copyright;
  int SF_id_scopyright;
  char *SF_id_software;
  char *SF_id_artist;
  char *SF_id_comment;
  char *SF_id_date;
  void *utility_db;
  int16 *isintab; /* ugens3.c */
  void *lprdaddr; /* ugens5.c */
  int currentLPCSlot;
  int max_lpc_slot;
  CS_HASH_TABLE *chn_db;
  int opcodedirWasOK;
  int disable_csd_options;
  CsoundRandMTState randState_;
  int performState;
  int ugens4_rand_16;
  int ugens4_rand_15;
  void *schedule_kicked;
  MYFLT *disprep_fftcoefs;
  void *winEPS_globals;
  OPARMS oparms_;
  REMOT_BUF SVrecvbuf; /* RM: rt_evt input Communications buffer */
  void *remoteGlobals;
  /* VL: pvs bus */
  int nchanif, nchanof;
  char *chanif, *chanof;
  /* VL: internal yield callback */
  int multiThreadedComplete;
  THREADINFO *multiThreadedThreadInfo;
  struct dag_t *multiThreadedDag;
  void *barrier1;
  void *barrier2;
  /* Statics from cs_par_dispatch; */
  /* ********These are no longer used******** */
  void *pointer1; /*struct global_var_lock_t *global_var_lock_root; */
  void *pointer2; /*struct global_var_lock_t **global_var_lock_cache; */
  int int1;       /* global_var_lock_count; */
  /* statics from cs_par_orc_semantic_analysis */
  struct instr_semantics_t *instCurr;
  struct instr_semantics_t *instRoot;
  int inInstr;
  int dag_changed;
  int dag_num_active;
  INSDS **dag_task_map;
  volatile stateWithPadding *dag_task_status;
  watchList *volatile *dag_task_watch;
  watchList *dag_wlmm;
  char **dag_task_dep;
  int dag_task_max_size;
  uint32_t tempStatus; /* keeps track of which files are temps */
  int orcLineOffset;   /* 1 less than 1st orch line in the CSD */
  int scoLineOffset;   /* 1 less than 1st score line in the CSD */
  char *csdname;
  /* original CSD name; do not free() */
  int parserNamedInstrFlag;
  int tran_nchnlsi;
  int scnt;             /* Count of strings */
  int strsiz;           /* length of current strings space */
  FUNC *sinetable;      /* A useful table */
  int sinelength;       /* Size of table */
  MYFLT *UNUSEDP;       /* pow2 table */
  MYFLT *cpsocfrc;      /* cps conv table */
  CORFIL *expanded_orc; /* output of preprocessor */
  CORFIL *expanded_sco; /* output of preprocessor */
  char *filedir[256];   /* for location directory */
  void *message_buffer;
  int jumpset;
  int info_message_request;
  int modules_loaded;
  MYFLT _system_sr;
  void *csdebug_data;     /* debugger data */
  int (*kperf)(CSOUND *); /* kperf function pointer, to switch between debug
                             and nodebug function */
  int score_parser;
  int print_version;
  int inZero; /* flag compilation of instr0 */
  struct _message_queue **msg_queue;
  volatile long msg_queue_wget;   /* Writer - Get index */
  volatile long msg_queue_wput;   /* Writer - Put Index */
  volatile long msg_queue_rstart; /* Reader - start index */
  volatile long msg_queue_items;
  int aftouch;
  void *directory;
  ALLOC_DATA *alloc_queue;
  volatile unsigned long alloc_queue_items;
  unsigned long alloc_queue_wp;
  spin_lock_t alloc_spinlock;
  EVTBLK *init_event;
  void (*csoundMessageStringCallback)(CSOUND *csound, int attr,
                                      const char *str);
  char *message_string;
  volatile unsigned long message_string_queue_items;
  unsigned long message_string_queue_wp;
  message_string_queue_t *message_string_queue;
  int io_initialised;
  char *op;
  int mode;
  char *opcodedir;
  char *score_srt;
  /*struct CSOUND_ **self;*/
  /**@}*/
};
