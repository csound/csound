/*
    oload.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Michael Gogins
              (C) 2005 Istvan Varga

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
                                /*                              OLOAD.C   */
#include "csoundCore.h"
#include <math.h>
#include "oload.h"
#include "midiops.h"
#include "insert.h"
#include "fgens.h"
#include "namedins.h"
#include "soundio.h"
#include "pvfileio.h"
#include "fftlib.h"

int     csoundGetAPIVersion(void);
void    writeheader(CSOUND *csound, int ofd, char *ofname);
int     playopen_dummy(CSOUND *csound, csRtAudioParams *parm);
void    rtplay_dummy(CSOUND *csound, MYFLT *outBuf, int nbytes);
int     recopen_dummy(CSOUND *csound, csRtAudioParams *parm);
int     rtrecord_dummy(CSOUND *csound, MYFLT *inBuf, int nbytes);
void    rtclose_dummy(CSOUND *csound);
void    csoundDefaultMessageCallback(CSOUND *csound, int attr,
                                     const char *format, va_list args);
void    csoundDefaultThrowMessageCallback(CSOUND *csound, const char *format,
                                                           va_list args);
void    defaultCsoundMakeGraph(CSOUND *csound, WINDAT *windat, char *name);
void    defaultCsoundDrawGraph(CSOUND *csound, WINDAT *windat);
void    defaultCsoundKillGraph(CSOUND *csound, WINDAT *windat);
int     defaultCsoundExitGraph(CSOUND *csound);
int     defaultCsoundYield(CSOUND *csound);
void    close_all_files(CSOUND *csound);
char    *getstrformat(int format);
int     sfsampsize(int format);
char    *type2string(int type);

const CSOUND cenviron_ = {
    /* ----------------- interface functions (288 total) ----------------- */
        csoundGetVersion,
        csoundGetAPIVersion,
        csoundGetHostData,
        csoundSetHostData,
        csoundPerform,
        csoundCompile,
        csoundPerformKsmps,
        csoundPerformBuffer,
        csoundCleanup,
        csoundReset,
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
        csoundGetProgress,
        csoundGetProfile,
        csoundGetCpuUsage,
        csoundIsScorePending,
        csoundSetScorePending,
        csoundGetScoreOffsetSeconds,
        csoundSetScoreOffsetSeconds,
        csoundRewindScore,
        csoundMessage,
        csoundMessageS,
        csoundMessageV,
        csoundThrowMessage,
        csoundThrowMessageV,
        csoundSetMessageCallback,
        csoundSetThrowMessageCallback,
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
        csoundLoadExternal,
        csoundLoadExternals,
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
        csoundFTFind,
        csoundFTFindP,
        csoundFTnp2Find,
        csoundGetTable,
        mmalloc,
        mcalloc,
        mrealloc,
        mfree,
        csoundDie,
        csoundInitError,
        csoundPerfError,
        csoundWarning,
        csoundDebugMsg,
        dispset,
        display,
        dispexit,
        intpow,
        ldmemfile,
        csoundLoadSoundFile,
        hfgens,
        getopnum,
        strarg2insno,
        strarg2opcno,
        strarg2name,
        insert_score_event,
        rewriteheader,
        writeheader,
        SAsndgetset,
        sndgetset,
        getsndin,
        csoundPerformKsmpsAbsolute,
        csoundGetDebug,
        csoundSetDebug,
        csoundTableLength,
        csoundTableGet,
        csoundTableSet,
        csoundCreateThread,
        csoundJoinThread,
        csoundCreateThreadLock,
        csoundWaitThreadLock,
        csoundNotifyThreadLock,
        csoundDestroyThreadLock,
        csoundSetFLTKThreadLocking,
        csoundGetFLTKThreadLocking,
        timers_struct_init,
        timers_get_real_time,
        timers_get_CPU_time,
        timers_random_seed,
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
        (int (*)(CSOUND*, const char*, void*, void*)) pvoc_openfile,
        pvoc_closefile,
        pvoc_putframes,
        pvoc_getframes,
        pvoc_framecount,
        pvoc_rewind,
        pvoc_errorstr,
        PVOCEX_LoadFile,
        csoundLongJmp,
        csoundErrorMsg,
        csoundErrMsgV,
        getstrformat,
        sfsampsize,
        type2string,
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
        csoundFTAlloc,
        csoundFTDelete,
        fdrecord,
        fdclose,
        NULL,
        { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL  },
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
        0.0,            /*  curTime             */
        0.0,            /*  curTime_inc         */
        0.0,            /*  curBeat             */
        0.0,            /*  curBeat_inc         */
        0.0,            /*  beatTime            */
        1024U,          /*  rtin_dev            */
        1024U,          /*  rtout_dev           */
        NULL,           /*  rtin_devs           */
        NULL,           /*  rtout_devs          */
        0, 0,           /*  nchanik, nchania    */
        0, 0,           /*  nchanok, nchanoa    */
        NULL, NULL,     /*  chanik, chania      */
        NULL, NULL,     /*  chanok, chanoa      */
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
        2345678,        /*  holdrand            */
        256,            /*  strVarMaxLen        */
        MAXINSNO,       /*  maxinsno            */
        0,              /*  strsmax             */
        (char**) NULL,  /*  strsets             */
        NULL,           /*  instrtxtp           */
        { NULL },       /*  m_chnbp             */
    /* ------- private data (not to be used by hosts or externals) ------- */
        /* callback function pointers */
        (SUBR) NULL,    /*  first_callback_     */
        (void (*)(CSOUND*, char*, MYFLT*)) NULL,
        (void (*)(CSOUND*, char*, MYFLT)) NULL,
        csoundDefaultMessageCallback,
        csoundDefaultThrowMessageCallback,
        defaultCsoundMakeGraph,
        defaultCsoundDrawGraph,
        defaultCsoundKillGraph,
        defaultCsoundExitGraph,
        defaultCsoundYield,
        (SUBR) NULL,    /*  last_callback_      */
        /* these are not saved on RESET */
        playopen_dummy,
        rtplay_dummy,
        recopen_dummy,
        rtrecord_dummy,
        rtclose_dummy,
        /* end of callbacks */
        FL(0.0),        /*  cpu_power_busy      */
        (char*) NULL,   /*  xfilename           */
        NLABELS,        /*  nlabels             */
        NGOTOS,         /*  ngotos              */
        1,              /*  peakchunks          */
        0,              /*  keep_tmp            */
        0,              /*  dither_output       */
        NULL,           /*  opcodlst            */
        NULL,           /*  opcode_list         */
        NULL,           /*  opcodlstend         */
        -1,             /*  maxopcno            */
        0,              /*  nrecs               */
        NULL,           /*  Linepipe            */
        0,              /*  Linefd              */
        NULL,           /*  ls_table            */
        NULL,           /*  scfp                */
        NULL,           /*  oscfp               */
        { FL(0.0)},     /*  maxamp              */
        { FL(0.0)},     /*  smaxamp             */
        { FL(0.0)},     /*  omaxamp             */
        {0}, {0}, {0},  /*  maxpos, smaxpos, omaxpos */
        NULL, NULL,     /*  scorein, scoreout   */
        NULL,           /*  pool                */
        NULL,           /*  argoffspace         */
        NULL,           /*  frstoff             */
#if defined(__WATCOMC__) || defined(__POWERPC__) || defined(mac_classic)
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
        {{NULL}, 0.0, 0,0,0,0l,0l,0l},  /*  ff  */
        NULL,           /*  flist               */
        0,              /*  maxfnum             */
        NULL,           /*  gensub              */
        GENMAX+1,       /*  genmax              */
        100,            /*  ftldno              */
        1,              /*  doFLTKThreadLocking */
        NULL,           /*  namedGlobals        */
        0,              /*  namedGlobalsCurrLimit */
        0,              /*  namedGlobalsMaxLimit */
        NULL,           /*  cfgVariableDB       */
        0.0, 0.0, 0.0,  /*  prvbt, curbt, nxtbt */
        0.0, 0.0,       /*  curp2, nxtim        */
        0,              /*  cyclesRemaining     */
        { NULL, '\0', 0, FL(0.0), FL(0.0), { FL(0.0) } },   /*  evt     */
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
        (void (*)(CSOUND*)) NULL,       /*  spinrecv        */
        (void (*)(CSOUND*)) NULL,       /*  spoutran        */
        (int (*)(CSOUND*, MYFLT*, int)) NULL,   /*  audrecv */
        (void (*)(CSOUND*, MYFLT*, int)) NULL,  /*  audtran */
        0,              /*  warped              */
        0,              /*  sstrlen             */
        (char*) NULL,   /*  sstrbuf             */
        1,              /*  enableMsgAttr       */
        0,              /*  sampsNeeded         */
        FL(0.0),        /*  csoundScoreOffsetSeconds_   */
        0,              /*  inChar_             */
        1,              /*  isGraphable_        */
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
        NULL,           /*  pvbufreadaddr       */
        NULL            /*  tbladr              */
};

/* otran.c */
extern  void    get_strpool_ptrs(CSOUND *csound,
                                 int *strpool_cnt, char ***strpool);
extern  void    strpool_delete(CSOUND *csound);

static  int     strlen_to_samples(const char *s);
static  void    unquote_string(char *dst, const char *src);
static  int     create_strconst_ndx_list(CSOUND *csound, int **lst, int offs);
static  void    convert_strconst_pool(CSOUND *csound, MYFLT *dst);

/* RWD for reentry */

void oloadRESET(CSOUND *csound)
{
    CSOUND    *saved_env;
    void      *p1, *p2;
    uintptr_t length;

    csound->oparms->odebug = 0;
    /* RWD 9:2000 not terribly vital, but good to do this somewhere... */
    pvsys_release(csound);
    close_all_files(csound);
    /* delete temporary files created by this Csound instance */
    remove_tmpfiles(csound);
    rlsmemfiles(csound);
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
    csound->oparms = saved_env->oparms;
    csound->hostdata = saved_env->hostdata;
    p1 = (void*) &(csound->first_callback_);
    p2 = (void*) &(csound->last_callback_);
    length = (uintptr_t) p2 - (uintptr_t) p1;
    memcpy(p1, (void*) &(saved_env->first_callback_), (size_t) length);
    memcpy(&(csound->exitjmp), &(saved_env->exitjmp), sizeof(jmp_buf));
    csound->memalloc_db = saved_env->memalloc_db;
    free(saved_env);
    memset(csound->oparms, 0, sizeof(OPARMS));
    csound->oparms->sfwrite  = 1;
    csound->oparms->sfheader = 1;
    csound->oparms->displays = 1;
    csound->oparms->msglevel = 135;
}

#ifdef FLOAT_COMPARE
#undef FLOAT_COMPARE
#endif
#ifdef USE_DOUBLE
#define FLOAT_COMPARE(x,y)  (fabs((double) (x) / (double) (y) - 1.0) > 1.0e-12)
#else
#define FLOAT_COMPARE(x,y)  (fabs((double) (x) / (double) (y) - 1.0) > 5.0e-7)
#endif

void oload(CSOUND *p)
{
    long    n, nn, combinedsize, insno, *lp;
    long    gblabeg, gblsbeg, gblscbeg, lclabeg, lclsbeg;
    MYFLT   *combinedspc, *gblspace, *fp1;
    INSTRTXT *ip;
    OPTXT   *optxt;
    OPARMS  *O = p->oparms;
    int     *strConstIndexList;
    MYFLT   ensmps;

    p->esr = p->tran_sr; p->ekr = p->tran_kr;
    p->ksmps = (int) ((ensmps = p->tran_ksmps) + FL(0.5));
    ip = p->instxtanchor.nxtinstxt;        /* for instr 0 optxts:  */
    optxt = (OPTXT *) ip;
    while ((optxt = optxt->nxtop) !=  NULL) {
      TEXT  *ttp = &optxt->t;
      ARGOFFS *inoffp, *outoffp;
      int opnum = ttp->opnum;
      if (opnum == ENDIN) break;
      if (opnum == LABEL) continue;
      outoffp = ttp->outoffs;           /* use unexpanded ndxes */
      inoffp = ttp->inoffs;             /* to find sr.. assigns */
      if (outoffp->count == 1 && inoffp->count == 1) {
        int rindex = (int) outoffp->indx[0] - (int) O->poolcount;
        if (rindex > 0 && rindex <= 5) {
          MYFLT conval = p->pool[inoffp->indx[0] - 1];
          switch (rindex) {
            case 1:  p->esr = conval;   break;  /* & use those values */
            case 2:  p->ekr = conval;   break;  /*  to set params now */
            case 3:  p->ksmps = (int) ((ensmps = conval) + FL(0.5)); break;
            case 4:  p->nchnls = (int) (conval + FL(0.5));  break;
            default: p->e0dbfs = conval; break;
          }
        }
      }
    }
    /* why I want oload() to return an error value.... */
    if (p->e0dbfs <= FL(0.0))
      p->Die(p, Str("bad value for 0dbfs: must be positive."));
    if (O->odebug)
      p->Message(p, "esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d "
                    "0dbfs = %.1f\n",
                    p->esr, p->ekr, p->ksmps, p->nchnls, p->e0dbfs);
    if (O->sr_override) {        /* if command-line overrides, apply now */
      p->esr = (MYFLT) O->sr_override;
      p->ekr = (MYFLT) O->kr_override;
      p->ksmps = (int) ((ensmps = ((MYFLT) O->sr_override
                                   / (MYFLT) O->kr_override)) + FL(0.5));
      p->Message(p, Str("sample rate overrides: "
                        "esr = %7.1f, ekr = %7.1f, ksmps = %d\n"),
                    p->esr, p->ekr, p->ksmps);
    }
    /* number of MYFLT locations to allocate for a string variable */
    p->strVarSamples = (p->strVarMaxLen + (int) sizeof(MYFLT) - 1)
                       / (int) sizeof(MYFLT);
    p->strVarMaxLen = p->strVarSamples * (int) sizeof(MYFLT);
    /* calculate total size of global pool */
    combinedsize = O->poolcount                 /* floating point constants */
                   + O->gblfixed                /* k-rate / spectral        */
                   + O->gblacount * p->ksmps            /* a-rate variables */
                   + O->gblscount * p->strVarSamples;   /* string variables */
    gblscbeg = combinedsize + 1;                /* string constants         */
    combinedsize += create_strconst_ndx_list(p, &strConstIndexList, gblscbeg);

    combinedspc = (MYFLT*) mcalloc(p, combinedsize * sizeof(MYFLT));
    /* copy pool into combined space */
    memcpy(combinedspc, p->pool, O->poolcount * sizeof(MYFLT));
    mfree(p, (void*) p->pool);
    p->pool = combinedspc;
    gblspace = p->pool + O->poolcount;
    gblspace[0] = p->esr;           /*   & enter        */
    gblspace[1] = p->ekr;           /*   rsvd word      */
    gblspace[2] = (MYFLT) p->ksmps; /*   curr vals      */
    gblspace[3] = (MYFLT) p->nchnls;
    gblspace[4] = p->e0dbfs;
    p->gbloffbas = p->pool - 1;
    /* string constants: unquote, convert escape sequences, and copy to pool */
    convert_strconst_pool(p, (MYFLT*) p->gbloffbas + (long) gblscbeg);

    gblabeg = O->poolcount + O->gblfixed + 1;
    gblsbeg = gblabeg + O->gblacount;
    ip = &(p->instxtanchor);
    while ((ip = ip->nxtinstxt) != NULL) {      /* EXPAND NDX for A & S Cells */
      optxt = (OPTXT *) ip;                     /*   (and set localen)        */
      lclabeg = (long) (ip->pmax + ip->lclfixed + 1);
      lclsbeg = (long) (lclabeg + ip->lclacnt);
      if (O->odebug) p->Message(p, "lclabeg %ld, lclsbeg %ld\n",
                                   lclabeg, lclsbeg);
      ip->localen = ((long) ip->lclfixed
                     + (long) ip->lclacnt * (long) p->ksmps
                     + (long) ip->lclscnt * (long) p->strVarSamples)
                    * (long) sizeof(MYFLT);
      /* align to 64 bits */
      ip->localen = (ip->localen + 7L) & (~7L);
      for (insno=0, n=0; insno <= p->maxinsno; insno++)
        if (p->instrtxtp[insno] == ip)  n++;            /* count insnos  */
      lp = ip->inslist = (long *) mmalloc(p, (long)(n+1) * sizeof(long));
      for (insno=0; insno <= p->maxinsno; insno++)
        if (p->instrtxtp[insno] == ip)  *lp++ = insno;  /* creat inslist */
      *lp = -1;                                         /*   & terminate */
      insno = *ip->inslist;                             /* get the first */
      while ((optxt = optxt->nxtop) !=  NULL) {
        TEXT    *ttp = &optxt->t;
        ARGOFFS *aoffp;
        long    indx;
        long    posndx;
        int     *ndxp;
        int     opnum = ttp->opnum;
        if (opnum == ENDIN || opnum == ENDOP) break;    /* IV - Sep 8 2002 */
        if (opnum == LABEL) continue;
        aoffp = ttp->outoffs;           /* ------- OUTARGS -------- */
        n = aoffp->count;
        for (ndxp = aoffp->indx; n--; ndxp++) {
          indx = *ndxp;
          if (indx > 0) {               /* positive index: global   */
            if (indx >= STR_OFS)        /* string constant          */
              p->Die(p, Str("internal error: string constant outarg"));
            if (indx > gblsbeg)         /* global string variable   */
              indx = gblsbeg + (indx - gblsbeg) * p->strVarSamples;
            else if (indx > gblabeg)    /* global a-rate variable   */
              indx = gblabeg + (indx - gblabeg) * p->ksmps;
            else if (indx <= 3 && O->sr_override &&
                     ip == p->instxtanchor.nxtinstxt)   /* for instr 0 */
              indx += 3;        /* deflect any old sr,kr,ksmps targets */
          }
          else {                        /* negative index: local    */
            posndx = -indx;
            if (indx < LABELIM)         /* label                    */
              continue;
            if (posndx > lclsbeg)       /* local string variable    */
              indx = -(lclsbeg + (posndx - lclsbeg) * p->strVarSamples);
            else if (posndx > lclabeg)  /* local a-rate variable    */
              indx = -(lclabeg + (posndx - lclabeg) * p->ksmps);
          }
          *ndxp = (int) indx;
        }
        aoffp = ttp->inoffs;            /* inargs:                  */
        if (opnum >= SETEND) goto realops;
        switch (opnum) {                /*      do oload SETs NOW   */
        case PSET:
          p->Message(p, "PSET: isno=%ld, pmax=%d\n", insno, ip->pmax);
          if ((n = aoffp->count) != ip->pmax) {
            p->Warning(p, Str("i%d pset args != pmax"), (int) insno);
            if (n < ip->pmax) n = ip->pmax; /* cf pset, pmax    */
          }                                 /* alloc the larger */
          ip->psetdata = (MYFLT *) mcalloc(p, (long)n * sizeof(MYFLT));
          for (n=aoffp->count,fp1=ip->psetdata,ndxp=aoffp->indx;
               n--; ) {
            *fp1++ = p->gbloffbas[*ndxp++];
            p->Message(p,"..%f..", *(fp1-1));
          }
          p->Message(p, "\n");
          break;
        }
        continue;       /* no runtime role for the above SET types */

      realops:
        n = aoffp->count;               /* -------- INARGS -------- */
        for (ndxp = aoffp->indx; n--; ndxp++) {
          indx = *ndxp;
          if (indx > 0) {               /* positive index: global   */
            if (indx >= STR_OFS)        /* string constant          */
              indx = (long) strConstIndexList[indx - (long) (STR_OFS + 1)];
            else if (indx > gblsbeg)    /* global string variable   */
              indx = gblsbeg + (indx - gblsbeg) * p->strVarSamples;
            else if (indx > gblabeg)    /* global a-rate variable   */
              indx = gblabeg + (indx - gblabeg) * p->ksmps;
          }
          else {                        /* negative index: local    */
            posndx = -indx;
            if (indx < LABELIM)         /* label                    */
              continue;
            if (posndx > lclsbeg)       /* local string variable    */
              indx = -(lclsbeg + (posndx - lclsbeg) * p->strVarSamples);
            else if (posndx > lclabeg)  /* local a-rate variable    */
              indx = -(lclabeg + (posndx - lclabeg) * p->ksmps);
          }
          *ndxp = (int) indx;
        }
      }
    }
    p->Free(p, strConstIndexList);

    p->tpidsr = TWOPI_F / p->esr;               /* now set internal  */
    p->mtpdsr = -(p->tpidsr);                   /*    consts         */
    p->pidsr = PI_F / p->esr;
    p->mpidsr = -(p->pidsr);
    p->onedksmps = FL(1.0) / (MYFLT) p->ksmps;
    p->sicvt = FMAXLEN / p->esr;
    p->kicvt = FMAXLEN / p->ekr;
    p->onedsr = FL(1.0) / p->esr;
    p->onedkr = FL(1.0) / p->ekr;
    /* IV - Sep 8 2002: save global variables that depend on ksmps */
    p->global_ksmps     = p->ksmps;
    p->global_ekr       = p->ekr;
    p->global_kcounter  = p->kcounter;
    reverbinit(p);
    dbfs_init(p, p->e0dbfs);
    p->nspout = p->ksmps * p->nchnls;  /* alloc spin & spout */
    p->nspin = p->nspout;
    p->spin  = (MYFLT *) mcalloc(p, p->nspin * sizeof(MYFLT));
    p->spout = (MYFLT *) mcalloc(p, p->nspout * sizeof(MYFLT));
    /* chk consistency one more time (FIXME: needed ?) */
    {
      char  s[256];
      sprintf(s, Str("sr = %.7g, kr = %.7g, ksmps = %.7g\nerror:"),
                 p->esr, p->ekr, ensmps);
      if (p->ksmps < 1 || FLOAT_COMPARE(ensmps, p->ksmps))
        csoundDie(p, Str("%s invalid ksmps value"), s);
      if (p->esr <= FL(0.0))
        csoundDie(p, Str("%s invalid sample rate"), s);
      if (p->ekr <= FL(0.0))
        csoundDie(p, Str("%s invalid control rate"), s);
      if (FLOAT_COMPARE(p->esr, (double) p->ekr * ensmps))
        csoundDie(p, Str("%s inconsistent sr, kr, ksmps"), s);
    }
    /* initialise sensevents state */
    p->prvbt = p->curbt = p->nxtbt = 0.0;
    p->curp2 = p->nxtim = p->timeOffs = p->beatOffs = 0.0;
    p->curTime = p->curBeat = 0.0;
    p->curTime_inc = 1.0 / (double) p->ekr;
    if (O->Beatmode && O->cmdTempo > 0) {
      /* if performing from beats, set the initial tempo */
      p->curBeat_inc = (double) O->cmdTempo / (60.0 * (double) p->ekr);
      p->beatTime = 60.0 / (double) O->cmdTempo;
    }
    else {
      p->curBeat_inc = 1.0 / (double) p->ekr;
      p->beatTime = 1.0;
    }
    p->cyclesRemaining = 0;
    memset(&(p->evt), 0, sizeof(EVTBLK));

    /* run instr 0 inits */
    if ((nn = init0(p)) != 0)
      csoundDie(p, Str("header init errors"));
}

/* get size of string in MYFLT units */

static int strlen_to_samples(const char *s)
{
    int n = (int) strlen(s);
    n = (n + (int) sizeof(MYFLT)) / (int) sizeof(MYFLT);
    return n;
}

/* convert string constant */

static void unquote_string(char *dst, const char *src)
{
    int i, j, n = (int) strlen(src) - 1;
    for (i = 1, j = 0; i < n; i++) {
      if (src[i] != '\\')
        dst[j++] = src[i];
      else {
        switch (src[++i]) {
        case 'a':   dst[j++] = '\a';  break;
        case 'b':   dst[j++] = '\b';  break;
        case 'f':   dst[j++] = '\f';  break;
        case 'n':   dst[j++] = '\n';  break;
        case 'r':   dst[j++] = '\r';  break;
        case 't':   dst[j++] = '\t';  break;
        case 'v':   dst[j++] = '\v';  break;
        case '"':   dst[j++] = '"';   break;
        case '\\':  dst[j++] = '\\';  break;
        default:
          if (src[i] >= '0' && src[i] <= '7') {
            int k = 0, l = (int) src[i] - '0';
            while (++k < 3 && src[i + 1] >= '0' && src[i + 1] <= '7')
              l = (l << 3) | ((int) src[++i] - '0');
            dst[j++] = (char) l;
          }
          else {
            dst[j++] = '\\'; i--;
          }
        }
      }
    }
    dst[j] = '\0';
}

static int create_strconst_ndx_list(CSOUND *csound, int **lst, int offs)
{
    int     *ndx_lst;
    char    **strpool;
    int     strpool_cnt, ndx, i;

    get_strpool_ptrs(csound, &strpool_cnt, &strpool);
    /* strpool_cnt always >= 1 because of empty string at index 0 */
    ndx_lst = (int*) csound->Malloc(csound, strpool_cnt * sizeof(int));
    for (i = 0, ndx = offs; i < strpool_cnt; i++) {
      ndx_lst[i] = ndx;
      ndx += strlen_to_samples(strpool[i]);
    }
    *lst = ndx_lst;
    /* return with total size in MYFLT units */
    return (ndx - offs);
}

static void convert_strconst_pool(CSOUND *csound, MYFLT *dst)
{
    char    **strpool, *s;
    int     strpool_cnt, ndx, i;

    get_strpool_ptrs(csound, &strpool_cnt, &strpool);
    for (ndx = i = 0; i < strpool_cnt; i++) {
      s = (char*) ((MYFLT*) dst + (int) ndx);
      unquote_string(s, strpool[i]);
      ndx += strlen_to_samples(strpool[i]);
    }
    /* original pool is no longer needed */
    strpool_delete(csound);
}

