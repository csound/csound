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

#define DKEY_DFLT  60

int csoundGetAPIVersion(void);

void dispset(WINDAT *, MYFLT *, long, char *, int, char *);
void display(WINDAT *);
MYFLT intpow(MYFLT, long);
char *unquote(char *);
long strarg2insno (MYFLT *p, char *s);
long strarg2opcno (MYFLT *p, char *s, int);
void rewriteheader(SNDFILE* ofd, int verbose);
void writeheader(int ofd, char *ofname);
int playopen_dummy(void *csound, csRtAudioParams *parm);
void rtplay_dummy(void *csound, void *outBuf, int nbytes);
int recopen_dummy(void *csound, csRtAudioParams *parm);
int rtrecord_dummy(void *csound, void *inBuf, int nbytes);
void rtclose_dummy(void *csound);

static  MYFLT   *gbloffbas;

OPARMS  O_ = {0,0,          /* odebug, initonly */
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

ENVIRON cenviron_ = {
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
        unquote,
        ldmemfile,
        err_printf,
        hfgens,
        IsPowerOfTwo,
        FFT2torlpacked,
        FFT2realpacked,
        cxmult,
        getopnum,
        strarg2insno,
        strarg2opcno,
        instance,
        rewriteheader,
        writeheader,
        csoundPrintf,
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
        playopen_dummy,
        rtplay_dummy,
        recopen_dummy,
        rtrecord_dummy,
        rtclose_dummy,
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
        /*
        * Data fields.
        */
        DFLT_KSMPS, /*  ksmps */
        DFLT_NCHNLS,  /*      nchnls */
        DFLT_KSMPS,     /*      global_ksmps */
        FL(0.0),        /*      global_ensmps */
        FL(0.0),        /*      global_ekr */
        FL(0.0),        /*      global_onedkr */
        FL(0.0),        /*      global_hfkprd */
        FL(0.0),        /*      global_kicvt */
        0L,     /* global_kcounter */
        FL(0.0),      /*      esr */
        FL(0.0),      /*      ekr */
        NULL, NULL, NULL,     /* orchname, scorename, xfilename */
        DFLT_DBFS,    /*        e0dbfs */
        NULL,         /*      reset_list */
        NLABELS,      /*      nlabels */
        NGOTOS,       /*      ngotos */
        0,    /*      strsmax */
        NULL,
        1,    /* peakchunks */
        NULL, /* zkstart */
        NULL, /* zastart */
        0,    /* zklast */
        0,    /* zalast */
        0,    /*  kcounter */
        NULL, /*  currevent */
        FL(0.0), /*   onedkr */
        FL(0.0), /*   onedsr */
        FL(0.0),      /*      kicvt */
        FL(0.0),      /*      sicvt */
        NULL, /*      spin */
        NULL, /*      spout */
        0,    /*      nspin */
        0,    /*      nspout */
        0,    /*      spoutactive */
        0,    /*      keep_tmp */
        0,    /*      dither_output */
        NULL, /*      opcodlst */
        NULL, /*      opcode_list */
        NULL, /*      opcodlstend */
        2345678L,     /* holdrand */
        MAXINSNO,     /* maxinsno */
        -1,   /*      maxopcno */
        NULL, /*      curip */
        NULL, /*      Livevtblk */
        0,    /*      nrecs */
        NULL, /*      Linepipe */
        0,    /*      Linefd */
        NULL, /*      ls_table */
        FL(0.0),      /*     curr_func_sr */
        NULL, /*      retfilnam */
        NULL, /*      instrtxtp */
        "",   /*      errmsg */
        NULL, /*      scfp */
        NULL, /*      oscfp */
        { FL(0.0)},   /*      maxamp */
        { FL(0.0)},   /*      smaxamp */
        { FL(0.0)},   /*      omaxamp */
        NULL, /*      maxampend */
        {0}, {0}, {0},        /*      maxpos, smaxpos, omaxpos */
        0,    /*      tieflag */
        NULL, /*      tokenstring */
        NULL, /*      polish */
        NULL, NULL,   /* scorein, scorout */
        FL(0.0), FL(0.0),     /*      ensmps, hfkprd */
        NULL,   /*      pool */
        NULL,   /*      argoffspace */
        NULL,   /*      frstoff */
#if defined(__WATCOMC__) || defined(__POWERPC__) || defined(mills_macintosh)
        {0},
#else
        {{{0}}}, /*      exitjmp_ of type jmp_buf */
#endif
        NULL,   /*      frstbp */
        0,      /*      sectcnt */
        {0},    /*      m_chnbp */
        NULL, NULL,     /*      cpsocint, cpsocfrc */
        0, 0, 0,        /*      inerrcnt, synterrcnt, perferrcnt */
        "",             /*      strmsg */
        {NULL},         /*      instxtanchor */
        {NULL},         /*      actanchor */
        {0L },          /*      rngcnt */
        0, 0,           /*      rngflg, multichan */
        NULL,           /*      OrcTrigEvts */
        NULL,           /*      freeEvtNodes */
        "",             /*      full_name */
        0, 0, 0,        /*      Mforcdecs, Mxtroffs, MTrkend */
        FL(-1.0), FL(-1.0), /*  tran_sr,tran_kr */
        FL(-1.0),       /*      tran_ksmps */
        DFLT_DBFS,      /*      tran_0dbfs */
        DFLT_NCHNLS,    /*      tran_nchnls */
        FL(-1.0), FL(-1.0), FL(-1.0), FL(-1.0), /* tpidsr,pidsr,mpidsr,mtpdsr */
        &O,             /*      oparms */
        NULL,           /*      hostData */
        NULL,           /*      opcodeInfo  */
        NULL,           /*      instrumentNames */
        FL(1.0) / DFLT_DBFS, /* dbfs_to_float ( = 1.0 / e0dbfs) */
        1024,           /*      rtin_dev */
        NULL,           /*      rtin_devs */
        1024,           /*      rtout_dev */
        NULL,           /*      rtout_devs */
        -1,             /*      displop4 */
        NULL,           /*      file_opened  */
        0,              /*      file_max */
        -1,             /*      file_num */
        0, NULL,        /*      nchanik, chanik */
        0, NULL,        /*      nchania, chania */
        0, NULL,        /*      nchanok, chanok */
        0, NULL,        /*      nchanoa, chanoa */
        {{NULL}, 0.0, 0,0,0,0l,0l,0l},      /*       ff */
        NULL,           /* flist */
        0,              /* maxfnum */
        NULL,           /* gensub */
        GENMAX+1,       /* genmax */
        100,            /* ftldno */
        1,              /* doFLTKThreadLocking */
        NULL,           /* namedGlobals -- IV - Jan 28 2005 */
        0,              /* namedGlobalsCurrLimit */
        0,              /* namedGlobalsMaxLimit */
        NULL,           /* cfgVariableDB */
        { 0.0 },        /* sensEvents_state */
        NULL,           /* rtRecord_userdata */
        NULL,           /* rtPlay_userdata */
        NULL,           /* memalloc_db */
        (MGLOBAL*) NULL, /* midiGlobals */
        NULL,           /* envVarDB */
        0,              /* evt_poll_cnt */
        0,              /* evt_poll_maxcnt */
        (MEMFIL*) NULL, /* memfiles */
        (MEMFIL*) NULL, /* rwd_memfiles */
        0,              /* FFT_max_size */
        NULL,           /* FFT_table_1 */
        NULL            /* FFT_table_2 */
};

OPARMS O;

ENVIRON cenviron;

int     pnum(char*);

extern  void    cpsoctinit(void), sssfinit(void);

/* RWD for reentry */
void oloadRESET(void)
{
    INSTRTXT    *tp = instxtanchor.nxtinstxt;

    memset(&instxtanchor,0,sizeof(INSTRTXT));
    ARGOFFSPACE = NULL;
    pool        = NULL;
    gbloffbas   = NULL;
    spin        = NULL;
    spout       = NULL;
    O.odebug    = 0;
    /* IV - Oct 31 2002: clear instrtxtp array */
    while (tp) {
      INSTRTXT  *nxttp = tp->nxtinstxt;
      OPTXT *bp = tp->nxtop;
      INSDS *ip = tp->instance;
      while (ip) {                              /* free all instances, */
        INSDS *nxtip = ip->nxtinstance;
        if (ip->opcod_iobufs && ip->insno > maxinsno)   /* IV - Nov 10 2002 */
          mfree(&cenviron, ip->opcod_iobufs);
        if (ip->fdch.nxtchp) fdchclose(ip);
        if (ip->auxch.nxtchp) auxchfree(&cenviron, ip);
        mfree(&cenviron, ip); ip = nxtip;
      }
      while (bp) {                              /* and opcode texts */
        OPTXT *nxtbp = bp->nxtop;
        mfree(&cenviron, bp); bp = nxtbp;
      }
      mfree(&cenviron, tp);
      tp = nxttp;
    }
    mfree(&cenviron, instrtxtp);        /* Start again */
    maxinsno    = MAXINSNO;
    maxopcno = -1; instrtxtp = NULL;    /* IV - Oct 24 2002 */
    e0dbfs      = DFLT_DBFS;
    /**
     * The first time around, assign cenviron_ to glob_ wholesale.
     */
    if (!cenviron.GetVersion) {
      memcpy(&cenviron, &cenviron_, sizeof(ENVIRON));
      cenviron.GetVersion = csoundGetVersion;
    }
    /**
     * The next time, copy everything EXCEPT the function pointers.
     * This is tricky because of those blasted macros!
     * We do it by saving them and copying them back again...
     */
    else {
      ENVIRON tempGlobals = cenviron;
      size_t front = (size_t)&tempGlobals;
      size_t back = (size_t)&tempGlobals.ksmps_;
      size_t length = back - front;
      void *saved_memalloc_db;
      /* save memalloc chain pointer for memRESET() */
      saved_memalloc_db = cenviron.memalloc_db;
      cenviron = cenviron_;
      memcpy(&cenviron, &tempGlobals, length);
      cenviron.memalloc_db = saved_memalloc_db;
      /* reset rtaudio function pointers */
      cenviron.recopen_callback = recopen_dummy;
      cenviron.playopen_callback = playopen_dummy;
      cenviron.rtrecord_callback = rtrecord_dummy;
      cenviron.rtplay_callback = rtplay_dummy;
      cenviron.rtclose_callback = rtclose_dummy;
    }
    O = O_;
    O.expr_opt = 0; /* disable --expression-opt by default for safety */
    cenviron.oparms_ = &O;
    /* IV - Sep 8 2002: also reset saved globals */
    global_ksmps = ksmps; global_ensmps = ensmps; global_ekr = ekr;
    global_onedkr = onedkr; global_hfkprd = hfkprd; global_kicvt = kicvt;
    global_kcounter = kcounter;
    rtin_dev = 1024;
    rtout_dev = 1024;
}

void oload(ENVIRON *csound)
{
    long   n, nn, combinedsize, gblabeg, lclabeg, insno, *lp;
    MYFLT  *combinedspc, *gblspace, *fp1, *fp2;
/*     int  *pgmdim = NULL; */
    INSTRTXT *ip;
    OPTXT *optxt;
    esr = tran_sr; ekr = tran_kr;
    ksmps = (int) ((ensmps = tran_ksmps) + FL(0.5));
    ip = instxtanchor.nxtinstxt;                /* for instr 0 optxts:  */
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
        int rindex = (int) outoffp->indx[0] - (int) O.poolcount;
        MYFLT conval = pool[inoffp->indx[0] - 1];
        switch (rindex) {
        case 1:  esr = conval;  break;  /* & use those values */
        case 2:  ekr = conval;  break;  /*  to set params now */
        case 3:  ksmps = (int) ((ensmps = conval) + FL(0.5));   break;
        case 4:  nchnls = (int) (conval + FL(0.5));  break;
        case 5:  e0dbfs = conval; break;
        default: break;
        }
      }
    }
    /* why I want oload() to return an error value.... */
    if (e0dbfs <= 0.0)
      csoundDie(csound, Str("bad value for 0dbfs: must be positive."));
    if (O.odebug)
      printf("esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d 0dbfs = %.1f\n",
             esr, ekr, ksmps, nchnls, e0dbfs);  ;
    if (O.sr_override) {        /* if command-line overrides, apply now */
      esr = (MYFLT) O.sr_override;
      ekr = (MYFLT) O.kr_override;
      ksmps = (int) ((ensmps = ((MYFLT) O.sr_override / (MYFLT) O.kr_override))
                     + FL(0.5));
      printf(Str("sample rate overrides: esr = %7.1f, ekr = %7.1f, ksmps = %d\n"),
             esr, ekr, ksmps);
    }
    combinedsize = (O.poolcount + O.gblfixed + O.gblacount * ksmps)
      * sizeof(MYFLT);
    combinedspc = (MYFLT *)mcalloc(csound, (long)combinedsize);
    for (fp1 = pool, fp2 = combinedspc, nn = O.poolcount; nn--; )
      *fp2++ = *fp1++;              /* copy pool into combined space */
    mfree(csound, (void *)pool);
    pool = combinedspc;
    gblspace = pool + O.poolcount;
    gblspace[0] = esr;              /*   & enter        */
    gblspace[1] = ekr;              /*   rsvd word      */
    gblspace[2] = ensmps;           /*   curr vals  */
    gblspace[3] = (MYFLT)nchnls;
    gblspace[4] = e0dbfs;
    gbloffbas = pool - 1;

    gblabeg = O.poolcount + O.gblfixed + 1;
    ip = &instxtanchor;
    while ((ip = ip->nxtinstxt) != NULL) {      /* EXPAND NDX for A Cells */
      optxt = (OPTXT *) ip;             /*   (and set localen)    */
      lclabeg = (long)(ip->pmax + ip->lclfixed + 1);
      if (O.odebug) printf("lclabeg %ld\n",lclabeg);
      ip->localen = ((long) ip->lclfixed + (long) ip->lclacnt * (long) ksmps)
                    * (long) sizeof(MYFLT);
      /* align to 64 bits */
      ip->localen = (ip->localen + 7L) & (~7L);
      for (insno=0, n=0; insno <= maxinsno; insno++)
        if (instrtxtp[insno] == ip)  n++;              /* count insnos  */
      lp = ip->inslist = (long *) mmalloc(csound, (long)(n+1) * sizeof(long));
      for (insno=0; insno <= maxinsno; insno++)
        if (instrtxtp[insno] == ip)  *lp++ = insno;    /* creat inslist */
      *lp = -1;                                        /*   & terminate */
      insno = *ip->inslist;                            /* get the first */
      while ((optxt = optxt->nxtop) !=  NULL) {
        TEXT *ttp = &optxt->t;
        ARGOFFS *aoffp;
        long  indx;
        long posndx;
        int *ndxp;
        int opnum = ttp->opnum;
        if (opnum == ENDIN || opnum == ENDOP) break;    /* IV - Sep 8 2002 */
        if (opnum == LABEL) continue;
        aoffp = ttp->outoffs;
        n=aoffp->count;
        for (ndxp=aoffp->indx; n--; ndxp++) {
/*       printf("**** indx = %d (%x); gblabeg=%d lclabeg=%d\n",
                *ndxp, *ndxp,gblabeg,lclabeg ); */
          if ((indx = *ndxp) > gblabeg) {
            indx = gblabeg + (indx - gblabeg) * ksmps;
          }
          else if (indx <= 0 && (posndx = -indx) > lclabeg
                   && indx >= LABELIM) {
            indx = -(lclabeg + (posndx - lclabeg) * ksmps);
          }
          else if (indx > 0 && indx <= 3 && O.sr_override
                   && ip == instxtanchor.nxtinstxt) { /* for instr 0 */
            indx += 3;    /* deflect any old sr,kr,ksmps targets */
          }
          else continue;
          *ndxp = (int) indx;
        }
        aoffp = ttp->inoffs;            /* inargs:                  */
        if (opnum >= SETEND) goto realops;
        switch (opnum) {                /*      do oload SETs NOW  */
        case STRSET:
          if (strsets == NULL)
            strsets = (char **)
              mcalloc(csound, (long)((strsmax=STRSMAX)+1) * sizeof(char *));
          indx = (long)gbloffbas[*aoffp->indx];
          if (indx >= strsmax) {
            long newmax = strsmax + STRSMAX;
            int i;
            while (indx > newmax) newmax += STRSMAX;
            strsets = (char**)mrealloc(csound, strsets, (newmax+1)*sizeof(char *));
            /* ??? */
            for (i=strsmax; i<newmax+1; i++) strsets[i] = NULL;
/*             for (i=0; i<newmax+1; i++)
                   printf("strset[%d]: %p\n", i, strsets[i]); */
            strsmax = newmax;
          }
          if (strsets == NULL || indx < 0) { /* No space left or -ve index */
            if (O.msglevel & WARNMSG)
              printf(Str("WARNING: illegal strset index\n"));
            longjmp(cenviron.exitjmp_,1);
          }
          if (strsets[indx] != NULL) {
            if (O.msglevel & WARNMSG)
              printf(Str("WARNING: strset index conflict\n"));
          }
          else {
            strsets[indx] = ttp->strargs[0];
          }
          printf("Strsets[%ld]:%s\n", indx, strsets[indx]);
          break;
        case PSET:
          printf("PSET: isno=%ld, pmax=%d\n", insno, ip->pmax);
          if ((n = aoffp->count) != ip->pmax) {
            if (O.msglevel & WARNMSG)
              printf(errmsg,Str("WARNING: i%ld pset args != pmax\n"), insno);
            if (n < ip->pmax) n = ip->pmax; /* cf pset, pmax    */
          }                                   /* alloc the larger */
          ip->psetdata = (MYFLT *) mcalloc(csound, (long)n * sizeof(MYFLT));
          for (n=aoffp->count,fp1=ip->psetdata,ndxp=aoffp->indx;
               n--; ) {
            *fp1++ = gbloffbas[*ndxp++];
            printf("..%f..", *(fp1-1));
          }
          printf("\n");
          break;
        }

        continue;       /* no runtime role for the above SET types */

      realops:
        n = aoffp->count;
        for (ndxp=aoffp->indx; n--; ndxp++) {
/*           printf("**** indx = %d (%x)\n", *ndxp, *ndxp); */
          if ((indx = (long)*ndxp) > gblabeg) {
            indx = gblabeg + (indx - gblabeg) * ksmps;
          }
          else if (indx <= 0 && (posndx = -indx) > lclabeg
                   && indx >= LABELIM) {
            indx = -(lclabeg + (posndx - lclabeg) * ksmps);
          }
          else continue;
          *ndxp = (int) indx;
        }
      }
    }
/*     if (pgmdim != NULL) free((char *)pgmdim); */
/*     pctlist = (MYFLT **) mcalloc(csound, (long)256 * sizeof(MYFLT *)); */
/*     insbusy = (int *) mcalloc(csound, (long)((maxinsno+1) * sizeof(int))); */

    sssfinit(); /* must be called before instr 0 initiates */

    if ((nn = init0(csound)) > 0)                       /* run instr 0 inits */
      csoundDie(csound, Str("header init errors"));
    /* IV - Feb 18 2003 */
    {
#ifndef USE_DOUBLE
      double    max_err = 1.0e-6;
#else
      double    max_err = 1.0e-12;
#endif
      char  s[256];
      sprintf(s, "sr = %.7g, kr = %.7g, ksmps = %.7g\n", /* & chk consistency */
              gblspace[0], gblspace[1], gblspace[2]);    /*   one more time   */
      if (ksmps < 1 ||
          fabs(((double) gblspace[2] / (double) ksmps) - 1.0) > max_err) {
        strcat(s, Str("error: invalid ksmps value"));
        csoundDie(csound, s);
      }
      gblspace[2] = ensmps = (MYFLT) ksmps;
      if (gblspace[0] <= FL(0.0)) {
        strcat(s, Str("error: invalid sample rate"));
        csoundDie(csound, s);
      }
      if (gblspace[1] <= FL(0.0)) {
        strcat(s, Str("error: invalid control rate"));
        csoundDie(csound, s);
      }
      if (fabs(((double) gblspace[0]
                / ((double) gblspace[1] * (double) gblspace[2])) - 1.0)
          > max_err) {
        strcat(s, Str("error: inconsistent sr, kr, ksmps"));
        csoundDie(csound, s);
      }
    }
    tpidsr = TWOPI_F / esr;                             /* now set internal  */
    mtpdsr = -tpidsr;                                   /*    consts         */
    pidsr = PI_F / esr;
    mpidsr = -pidsr;
    sicvt = FMAXLEN / esr;
    kicvt = FMAXLEN / ekr;
    hfkprd = FL(0.5) / ekr;
    onedsr = FL(1.0) / esr;
    onedkr = FL(1.0) / ekr;
    /* IV - Sep 8 2002: save global variables that depend on ksmps */
    global_ksmps = ksmps; global_ensmps = ensmps; global_ekr = ekr;
    global_onedkr = onedkr; global_hfkprd = hfkprd; global_kicvt = kicvt;
    global_kcounter = kcounter;
    cpsoctinit();
    reverbinit();
    dbfs_init(e0dbfs);
    nspin = nspout = ksmps * nchnls;                    /* alloc spin & spout */
    spin =  (MYFLT *) mcalloc(csound, (long)nspin*sizeof(MYFLT));
    spout = (MYFLT *) mcalloc(csound, (long)nspout*sizeof(MYFLT));
}

INSDS *
instance(int insno)             /* create instance of an instr template */
                                /*   allocates and sets up all pntrs    */
{
    INSTRTXT *tp;
    INSDS   *ip;
    OPTXT   *optxt;
    OPDS    *opds, *prvids, *prvpds;
    OENTRY  *ep;
    LBLBLK  **lopds, **lopdsp;
    LARGNO  *larg, *largp/* = larg */;
    int     n, pextent, opnum, reqd;
    char    *nxtopds, *opdslim;
    MYFLT   **argpp, *fltp, *lclbas, *lcloffbas, *csetoffbas = NULL;
    ARGOFFS *aoffp;
    int     indx, posndx;
    int     *ndxp;
    MCHNBLK *chp = NULL;

    lopds = (LBLBLK**)mmalloc(&cenviron, sizeof(LBLBLK*)*nlabels);
    lopdsp = lopds;
    larg = (LARGNO*)mmalloc(&cenviron, sizeof(LARGNO)*ngotos);
    largp = larg;
    lopdsp = lopds;
    tp = instrtxtp[insno];
    if (tp->mdepends & 06) {                /* if need midi chan, chk ok */
      MCHNBLK **chpp = M_CHNBP;
      for (n = MAXCHAN; n--; ) {
        if ((chp = *chpp++) !=((MCHNBLK*)NULL))     {
          csetoffbas = chp->ctl_val;
          if (!chp->insno) {
            chp->insno = insno;
            printf(Str("instr %d seeking midi chnl data, assigned chnl %d\n"),
                   insno, chp->insno);
            break;
          }
          if (chp->insno != insno)
            continue;
        }
      }
    }
    pextent = sizeof(INSDS) + tp->pextrab;          /* alloc new space,  */
    ip = (INSDS *) mcalloc(&cenviron, (long)pextent + tp->localen + tp->opdstot);
    ip->csound = &cenviron;
    if (tp->mdepends & 06)
      ip->m_chnbp = chp;
    /* IV - Oct 26 2002: replaced with faster version (no search) */
    ip->prvinstance = tp->lst_instance;
    if (tp->lst_instance)
      tp->lst_instance->nxtinstance = ip;
    else
      tp->instance = ip;
    tp->lst_instance = ip;
    /* IV - Nov 10 2002 */
    if (insno > maxinsno) {
      int pcnt;
      ip->nxtact = tp->act_instance;                /* next free instance */
      tp->act_instance = ip;                        /* for user opcodes */
      pcnt = (int) tp->opcode_info->perf_incnt;
      pcnt += (int) tp->opcode_info->perf_outcnt;
      pcnt = sizeof(OPCOD_IOBUFS) + sizeof(MYFLT*) * (pcnt << 1);
      ip->opcod_iobufs = (void*) mmalloc(&cenviron, pcnt);
    }
    lcloffbas = &ip->p0;
    lclbas = (MYFLT *)((char *)ip + pextent);       /* split local space */
    nxtopds = (char *)lclbas + tp->localen;
    opdslim = nxtopds + tp->opdstot;
    if (O.odebug)
      printf(Str("instr %d allocated at %p\n\tlclbas %p, opds %p\n"),
             insno,ip,lclbas,nxtopds);
    optxt = (OPTXT *)tp;
    prvids = prvpds = (OPDS *)ip;
    while ((optxt = optxt->nxtop) != NULL) {        /* for each op in instr */
      TEXT *ttp = &optxt->t;
      if ((opnum = ttp->opnum) == ENDIN           /*  (until ENDIN)  */
          || opnum == ENDOP)      /* IV - Sep 8 2002: (or ENDOP) */
        break;
      if (opnum == STRSET) {
        nxtopds +=  2 * sizeof(MYFLT*);
        continue; /* Only at load time */
      }
      if (opnum == PSET) {
        ip->p1 = (MYFLT)insno; continue;
      }
      ep = &opcodlst[opnum];                        /* for all ops:     */
      opds = (OPDS *) nxtopds;                      /*   take reqd opds */
      nxtopds += ep->dsblksiz;
      if (O.odebug)
        printf(Str("op %d (%s) allocated at %p\n"),
               opnum,ep->opname,opds);
      opds->optext = optxt;                         /* set common headata */
      opds->insdshead = ip;
      if (opnum == LABEL) {                         /* LABEL:       */
        LBLBLK  *lblbp = (LBLBLK *) opds;
        lblbp->prvi = prvids;                       /*    save i/p links */
        lblbp->prvp = prvpds;
        *lopdsp++ = lblbp;                          /*    log the lbl bp */
        continue;                                   /*    for later refs */
      }
      if ((ep->thread & 07)==0) {                   /* thread 1 OR 2:  */
        if (ttp->pftype == 'b') {
          prvids = prvids->nxti = opds;
          opds->iopadr = ep->iopadr;
        }
        else {
          prvpds = prvpds->nxtp = opds;
          opds->opadr = ep->kopadr;
        }
        goto args;
      }
      if ((ep->thread & 01)!=0) {                   /* thread 1:        */
        prvids = prvids->nxti = opds;               /* link into ichain */
        opds->iopadr = ep->iopadr;                  /*   & set exec adr */
        if (opds->iopadr == NULL)
          csoundDie(&cenviron, Str("null iopadr"));
      }
      if ((n = ep->thread & 06)!=0) {               /* thread 2 OR 4:   */
        prvpds = prvpds->nxtp = opds;               /* link into pchain */
        if (!(n & 04) ||
            (ttp->pftype == 'k' && ep->kopadr != NULL))
          opds->opadr = ep->kopadr;                 /*      krate or    */
        else opds->opadr = ep->aopadr;              /*      arate       */
        if (O.odebug) printf("opadr = %p\n",opds->opadr);
        if (opds->opadr == NULL)
          csoundDie(&cenviron, Str("null opadr"));
      }
      opds->dopadr = ep->dopadr;
    args:
      argpp = (MYFLT **)((char *)opds + sizeof(OPDS));
      if (O.odebug) printf("argptrs:");
      aoffp = ttp->outoffs;                         /* for outarg codes: */
      reqd = strlen(ep->outypes);
      for (n=aoffp->count, ndxp=aoffp->indx; n--; reqd--) {
        if ((indx = *ndxp++) > 0)            /* cvt index to lcl/gbl adr */
          fltp = gbloffbas + indx;
        else if ((posndx = -indx) < CBAS)
          fltp = lcloffbas + posndx;
        else
          fltp = csetoffbas + posndx - CBAS;
        if (O.odebug) printf("\t%p", fltp);
        *argpp++ = fltp;
      }
      while (reqd--) {                              /* if more outypes, pad */
        if (O.odebug) printf("\tPADOUT");
        *argpp++ = NULL;
      }
      aoffp = ttp->inoffs;                          /* for inarg codes: */
      for (n=aoffp->count, ndxp=aoffp->indx; n--; ) {
        if ((indx = *ndxp++) < LABELIM) {
          if (O.odebug) printf("\t***lbl");
          largp->lblno = indx - LABELOFS;           /* if label ref, defer */
          largp->argpp = argpp++;
          largp++;
        } else {
          if (indx > 0)                         /* else cvt ndx to lcl/gbl */
            fltp = gbloffbas + indx;
          else if ((posndx = -indx) < CBAS)
            fltp = lcloffbas + posndx;
          else
            fltp = csetoffbas + posndx - CBAS;
          if (O.odebug) printf("\t%p", fltp);
          *argpp++ = fltp;
        }
      }
      if (O.odebug) printf("\n");
    }
    if (nxtopds != opdslim) {
      printf(Str("nxtopds = %p opdslim = %p\n"), nxtopds, opdslim);
      if (nxtopds > opdslim)
        csoundDie(&cenviron, Str("inconsistent opds total"));
    }
    while (--largp >= larg)
      *largp->argpp = (MYFLT *) lopds[largp->lblno]; /* now label refs */
    return(ip);
}

int pnum(char *s)               /* check a char string for pnum format  */
                                /*   and return the pnum ( >= 0 )       */
{                               /* else return -1                       */
    int n;

    if (*s == 'p' || *s == 'P')
      if (sscanf(++s, "%d", &n))
        return(n);
    return(-1);
}

