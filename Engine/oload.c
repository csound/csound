/*
    oload.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Michael Gogins

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

#include "cs.h"                 /*                              OLOAD.C   */
#include "oload.h"
#include <math.h>
#include "midiops.h"
#include "insert.h"
#include "ftgen.h"
#include "csound.h"
#include "namedins.h"

#define DKEY_DFLT  60

int csoundGetAPIVersion(void);

void dispset(WINDAT *, MYFLT *, long, char *, int, char *);
void display(WINDAT *);
void writeheader(int ofd, char *ofname);
int playopen_dummy(void *csound, csRtAudioParams *parm);
void rtplay_dummy(void *csound, void *outBuf, int nbytes);
int recopen_dummy(void *csound, csRtAudioParams *parm);
int rtrecord_dummy(void *csound, void *inBuf, int nbytes);
void rtclose_dummy(void *csound);

static const OPARMS O_ = {
              0,0,          /* odebug, initonly */
              0,1,1,0,      /* sfread, sfwrite, sfheader, filetyp */
              0,0,          /* inbufsamps, outbufsamps */
              0,0,          /* informat, outformat */
              0,0,          /* insampsiz, sfsampsize */
              1,0,0,7,      /* displays, graphsoff, postscript, msglevel */
              0,0,0,        /* Beatmode, cmdTempo, oMaxLag */
              0,0,0,0,      /* usingcscore, Linein, Midiin, FMidiin */
              0,            /* OrcEvts */
              0,0           /* RTevents, ksensing */
};

const ENVIRON cenviron_ = {
        /*
        * Interface functions.
        */
        0, /*csoundGetVersion,*/
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
        csoundLoadExternal,
        csoundLoadExternals,
        csoundOpenLibrary,
        csoundCloseLibrary,
        csoundGetLibrarySymbol,
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
        intpow,
        ldmemfile,
        hfgens,
        getopnum,
        strarg2insno,
        strarg2opcno,
        strarg2name,
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
        /* IV - Jan 27 2005: new functions */
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
        csoundAddUtility,
        csoundRunUtility,
        playopen_dummy,
        rtplay_dummy,
        recopen_dummy,
        rtrecord_dummy,
        rtclose_dummy,
        (void (*)(void*, char*, MYFLT*)) NULL,
        (void (*)(void*, char*, MYFLT)) NULL,
        /*
        * Data fields.
        */
        (OPDS*) NULL,   /*  ids                 */
        (OPDS*) NULL,   /*  pds                 */
        DFLT_KSMPS,     /*  ksmps               */
        DFLT_NCHNLS,    /*  nchnls              */
        FL(0.0),        /*  esr                 */
        FL(0.0),        /*  ekr                 */
        DFLT_KSMPS,     /*  global_ksmps        */
        FL(0.0),        /*  global_ensmps       */
        FL(0.0),        /*  global_ekr          */
        FL(0.0),        /*  global_onedkr       */
        FL(0.0),        /*  global_hfkprd       */
        FL(0.0),        /*  global_kicvt        */
        FL(0.0),        /*  cpu_power_busy      */
        0L,             /*  global_kcounter     */
        NULL, NULL, NULL,     /* orchname, scorename, xfilename */
        DFLT_DBFS,      /*  e0dbfs              */
        NULL,           /*  reset_list          */
        NLABELS,        /*  nlabels             */
        NGOTOS,         /*  ngotos              */
        0,              /*  strsmax             */
        NULL,           /*  strsets             */
        1,              /*  peakchunks          */
        NULL,           /*  zkstart             */
        NULL,           /*  zastart             */
        0,              /*  zklast              */
        0,              /*  zalast              */
        0,              /*  kcounter            */
        NULL,           /*  currevent           */
        FL(0.0),        /*  onedkr              */
        FL(0.0),        /*  onedsr              */
        FL(0.0),        /*  kicvt               */
        FL(0.0),        /*  sicvt               */
        NULL,           /*  spin                */
        NULL,           /*  spout               */
        0,              /*  nspin               */
        0,              /*  nspout              */
        0,              /*  spoutactive         */
        0,              /*  keep_tmp            */
        0,              /*  dither_output       */
        NULL,           /*  opcodlst            */
        NULL,           /*  opcode_list         */
        NULL,           /*  opcodlstend         */
        2345678L,       /*  holdrand            */
        MAXINSNO,       /*  maxinsno            */
        -1,             /*  maxopcno            */
        NULL,           /*  curip               */
        NULL,           /*  Livevtblk           */
        0,              /*  nrecs               */
        NULL,           /*  Linepipe            */
        0,              /*  Linefd              */
        NULL,           /*  ls_table            */
        FL(0.0),        /*  curr_func_sr        */
        NULL,           /*  retfilnam           */
        NULL,           /*  instrtxtp           */
        "",             /*  errmsg              */
        NULL,           /*  scfp                */
        NULL,           /*  oscfp               */
        { FL(0.0)},     /*  maxamp              */
        { FL(0.0)},     /*  smaxamp             */
        { FL(0.0)},     /*  omaxamp             */
        NULL,           /*  maxampend           */
        {0}, {0}, {0},  /*  maxpos, smaxpos, omaxpos */
        0,              /*  reinitflag          */
        0,              /*  tieflag             */
        NULL, NULL,     /*  scorein, scorout    */
        FL(0.0), FL(0.0), /* ensmps, hfkprd     */
        NULL,           /*  pool                */
        NULL,           /*  argoffspace         */
        NULL,           /*  frstoff             */
#if defined(__WATCOMC__) || defined(__POWERPC__) || defined(mills_macintosh)
        {0},
#else
        {{{0}}},        /*  exitjmp_ of type jmp_buf */
#endif
        NULL,           /*  frstbp              */
        0,              /*  sectcnt             */
        {0},            /*  m_chnbp             */
        NULL,           /*  cpsocfrc            */
        0, 0, 0,        /*  inerrcnt, synterrcnt, perferrcnt */
        "",             /*  strmsg              */
        {NULL},         /*  instxtanchor        */
        {NULL},         /*  actanchor           */
        {0L },          /*  rngcnt              */
        0, 0,           /*  rngflg, multichan   */
        NULL,           /*  OrcTrigEvts         */
        NULL,           /*  freeEvtNodes        */
        "",             /*  name_full           */
        0, 0, 0,        /*  Mforcdecs, Mxtroffs, MTrkend */
        FL(-1.0), FL(-1.0), /*  tran_sr,tran_kr */
        FL(-1.0),       /*  tran_ksmps          */
        DFLT_DBFS,      /*  tran_0dbfs          */
        DFLT_NCHNLS,    /*  tran_nchnls         */
        FL(-1.0), FL(-1.0), FL(-1.0), FL(-1.0), /* tpidsr,pidsr,mpidsr,mtpdsr */
        (OPARMS*) NULL, /*  oparms              */
        NULL,           /*  hostData            */
        NULL,           /*  opcodeInfo          */
        NULL,           /*  instrumentNames     */
        NULL,           /*  strsav_str          */
        NULL,           /*  strsav_space        */
        FL(1.0) / DFLT_DBFS, /* dbfs_to_float ( = 1.0 / e0dbfs) */
        1024,           /*  rtin_dev            */
        NULL,           /*  rtin_devs           */
        1024,           /*  rtout_dev           */
        NULL,           /*  rtout_devs          */
        NULL,           /*  file_opened         */
        0,              /*  file_max            */
        -1,             /*  file_num            */
        0, NULL,        /*  nchanik, chanik     */
        0, NULL,        /*  nchania, chania     */
        0, NULL,        /*  nchanok, chanok     */
        0, NULL,        /*  nchanoa, chanoa     */
        {{NULL}, 0.0, 0,0,0,0l,0l,0l},  /*  ff  */
        NULL,           /*  flist               */
        0,              /*  maxfnum             */
        NULL,           /*  gensub              */
        GENMAX+1,       /*  genmax              */
        100,            /*  ftldno              */
        1,              /*  doFLTKThreadLocking */
        NULL,           /*  namedGlobals -- IV - Jan 28 2005 */
        0,              /*  namedGlobalsCurrLimit */
        0,              /*  namedGlobalsMaxLimit */
        NULL,           /*  cfgVariableDB       */
        { 0.0 },        /*  sensEvents_state    */
        NULL,           /*  rtRecord_userdata   */
        NULL,           /*  rtPlay_userdata     */
        NULL,           /*  memalloc_db         */
        (MGLOBAL*) NULL, /* midiGlobals         */
        NULL,           /*  envVarDB            */
        0,              /*  evt_poll_cnt        */
        0,              /*  evt_poll_maxcnt     */
        (MEMFIL*) NULL, /*  memfiles            */
        (MEMFIL*) NULL, /*  rwd_memfiles        */
        0,              /*  FFT_max_size        */
        NULL,           /*  FFT_table_1         */
        NULL,           /*  FFT_table_2         */
        NULL, NULL, NULL, /* tseg, tpsave, tplim */
        0L,             /*  fout_kreset         */
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
        0,              /*  advanceCnt          */
        (MYFLT*) NULL,  /*  gbloffbas           */
        NULL,           /*  otranGlobals        */
        NULL,           /*  rdorchGlobals       */
        NULL,           /*  sreadGlobals        */
        256,            /*  strVarMaxLen        */
        0               /*  strVarSamples       */
};

/* otran.c */
extern  void    get_strpool_ptrs(ENVIRON *csound,
                                 int *strpool_cnt, char ***strpool);
extern  void    strpool_delete(ENVIRON *csound);

static  int     strlen_to_samples(const char *s);
static  void    unquote_string(char *dst, const char *src);
static  int     create_strconst_ndx_list(ENVIRON *csound, int **lst, int offs);
static  void    convert_strconst_pool(ENVIRON *csound, MYFLT *dst);

/* stub to be removed soon... */
ENVIRON cenviron = { NULL };

/* RWD for reentry */
void oloadRESET(ENVIRON *csound)
{
    INSTRTXT    *tp = csound->instxtanchor.nxtinstxt;

    memset(&(csound->instxtanchor), 0, sizeof(INSTRTXT));
    csound->argoffspace     = NULL;
    csound->pool            = NULL;
    csound->gbloffbas       = NULL;
    csound->spin            = NULL;
    csound->spout           = NULL;
    csound->oparms->odebug  = 0;
    /* IV - Oct 31 2002: clear instrtxtp array */
    while (tp) {
      INSTRTXT  *nxttp = tp->nxtinstxt;
      OPTXT *bp = tp->nxtop;
      INSDS *ip = tp->instance;
      while (ip) {                              /* free all instances, */
        INSDS *nxtip = ip->nxtinstance;
        if (ip->opcod_iobufs && ip->insno > csound->maxinsno)
          mfree(csound, ip->opcod_iobufs);
        if (ip->fdch.nxtchp)
          fdchclose(csound, ip);
        if (ip->auxch.nxtchp)
          auxchfree(csound, ip);
        mfree(csound, ip);
        ip = nxtip;
      }
      while (bp) {                              /* and opcode texts */
        OPTXT *nxtbp = bp->nxtop;
        mfree(csound, bp); bp = nxtbp;
      }
      mfree(csound, tp);
      tp = nxttp;
    }
    mfree(csound, csound->instrtxtp);           /* Start again */
    /**
     * Copy everything EXCEPT the function pointers.
     * We do it by saving them and copying them back again...
     */
    {
      void    *tempGlobals, *saved_memalloc_db = csound->memalloc_db;
      void    *saved_hostdata = csound->hostdata;
      OPARMS  *saved_oparms = csound->oparms;
      size_t  length = (size_t) ((uintptr_t) &(csound->ids)
                                 - (uintptr_t) csound);
      tempGlobals = malloc(length);     /* hope that this does not fail... */
      memcpy(tempGlobals, (void*) csound, length);
      memcpy(csound, &cenviron_, sizeof(ENVIRON));
      memcpy((void*) csound, tempGlobals, length);
      free(tempGlobals);
      csound->hostdata = saved_hostdata;
      csound->oparms = saved_oparms;
      csound->memalloc_db = saved_memalloc_db;
      /* reset rtaudio function pointers */
      csound->recopen_callback = recopen_dummy;
      csound->playopen_callback = playopen_dummy;
      csound->rtrecord_callback = rtrecord_dummy;
      csound->rtplay_callback = rtplay_dummy;
      csound->rtclose_callback = rtclose_dummy;
    }
    memcpy(csound->oparms, &O_, sizeof(OPARMS));
    /* IV - Sep 8 2002: also reset saved globals */
    csound->global_ksmps     = csound->ksmps;
    csound->global_ensmps    = csound->ensmps;
    csound->global_ekr       = csound->ekr;
    csound->global_onedkr    = csound->onedkr;
    csound->global_hfkprd    = csound->hfkprd;
    csound->global_kicvt     = csound->kicvt;
    csound->global_kcounter  = csound->kcounter;
    csound->rtin_dev         = 1024;
    csound->rtout_dev        = 1024;
}

#ifdef FLOAT_COMPARE
#undef FLOAT_COMPARE
#endif
#ifdef USE_DOUBLE
#define FLOAT_COMPARE(x,y)  (fabs((double) (x) / (double) (y) - 1.0) > 1.0e-12)
#else
#define FLOAT_COMPARE(x,y)  (fabs((double) (x) / (double) (y) - 1.0) > 5.0e-7)
#endif

void oload(ENVIRON *p)
{
    long    n, nn, combinedsize, insno, *lp;
    long    gblabeg, gblsbeg, gblscbeg, lclabeg, lclsbeg;
    MYFLT   *combinedspc, *gblspace, *fp1;
    INSTRTXT *ip;
    OPTXT   *optxt;
    OPARMS  *O = p->oparms;
    int     *strConstIndexList;

    p->esr = p->tran_sr; p->ekr = p->tran_kr;
    p->ksmps = (int) ((p->ensmps = p->tran_ksmps) + FL(0.5));
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
        MYFLT conval = p->pool[inoffp->indx[0] - 1];
        switch (rindex) {
        case 1:  p->esr = conval;  break;  /* & use those values */
        case 2:  p->ekr = conval;  break;  /*  to set params now */
        case 3:  p->ksmps = (int) ((p->ensmps = conval) + FL(0.5));   break;
        case 4:  p->nchnls = (int) (conval + FL(0.5));  break;
        case 5:  p->e0dbfs = conval; break;
        default: break;
        }
      }
    }
    /* why I want oload() to return an error value.... */
    if (p->e0dbfs <= FL(0.0))
      csoundDie(p, Str("bad value for 0dbfs: must be positive."));
    if (O->odebug)
      p->Message(p, "esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d "
                    "0dbfs = %.1f\n",
                    p->esr, p->ekr, p->ksmps, p->nchnls, p->e0dbfs);
    if (O->sr_override) {        /* if command-line overrides, apply now */
      p->esr = (MYFLT) O->sr_override;
      p->ekr = (MYFLT) O->kr_override;
      p->ksmps = (int) ((p->ensmps = ((MYFLT) O->sr_override
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
    gblspace[2] = p->ensmps;        /*   curr vals      */
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
/*        p->Message(p, "**** indx = %d (%x); gblabeg=%d lclabeg=%d\n",
                        *ndxp, *ndxp, gblabeg, lclabeg); */
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
/*        p->Message(p, "**** indx = %d (%x); gblabeg=%d lclabeg=%d\n",
                        *ndxp, *ndxp, gblabeg, lclabeg); */
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

/*  pctlist = (MYFLT **) mcalloc(p, 256 * sizeof(MYFLT *)); */
/*  insbusy = (int *) mcalloc(p, (p->maxinsno+1) * sizeof(int)); */

    csoundInitEnv(p);      /* must be called before instr 0 initiates */

    if ((nn = init0(p)) > 0)                       /* run instr 0 inits */
      csoundDie(p, Str("header init errors"));
    /* IV - Feb 18 2003 */
    {
      char  s[256];
      sprintf(s, "sr = %.7g, kr = %.7g, ksmps = %.7g\n", /* & chk consistency */
              gblspace[0], gblspace[1], gblspace[2]);    /*   one more time   */
      if (p->ksmps < 1 || FLOAT_COMPARE(gblspace[2], p->ksmps)) {
        strcat(s, Str("error: invalid ksmps value"));
        csoundDie(p, s);
      }
      gblspace[2] = p->ensmps = (MYFLT) p->ksmps;
      if (gblspace[0] <= FL(0.0)) {
        strcat(s, Str("error: invalid sample rate"));
        csoundDie(p, s);
      }
      if (gblspace[1] <= FL(0.0)) {
        strcat(s, Str("error: invalid control rate"));
        csoundDie(p, s);
      }
      if (FLOAT_COMPARE(gblspace[0], (double) gblspace[1] * gblspace[2])) {
        strcat(s, Str("error: inconsistent sr, kr, ksmps"));
        csoundDie(p, s);
      }
    }
    p->tpidsr = TWOPI_F / p->esr;               /* now set internal  */
    p->mtpdsr = -(p->tpidsr);                   /*    consts         */
    p->pidsr = PI_F / p->esr;
    p->mpidsr = -(p->pidsr);
    p->sicvt = FMAXLEN / p->esr;
    p->kicvt = FMAXLEN / p->ekr;
    p->hfkprd = FL(0.5) / p->ekr;
    p->onedsr = FL(1.0) / p->esr;
    p->onedkr = FL(1.0) / p->ekr;
    /* IV - Sep 8 2002: save global variables that depend on ksmps */
    p->global_ksmps     = p->ksmps;
    p->global_ensmps    = p->ensmps;
    p->global_ekr       = p->ekr;
    p->global_onedkr    = p->onedkr;
    p->global_hfkprd    = p->hfkprd;
    p->global_kicvt     = p->kicvt;
    p->global_kcounter  = p->kcounter;
    cpsoctinit(p);
    reverbinit(p);
    dbfs_init(p, p->e0dbfs);
    p->nspout = p->ksmps * p->nchnls;  /* alloc spin & spout */
    p->nspin = p->nspout;
    p->spin  = (MYFLT *) mcalloc(p, p->nspin * sizeof(MYFLT));
    p->spout = (MYFLT *) mcalloc(p, p->nspout * sizeof(MYFLT));
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

static int create_strconst_ndx_list(ENVIRON *csound, int **lst, int offs)
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

static void convert_strconst_pool(ENVIRON *csound, MYFLT *dst)
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

