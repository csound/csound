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
#include "midiops.h"
#include "insert.h"     /* IV - Nov 10 2002 */

#define DKEY_DFLT  60

void *csoundCreate(void *);
int csoundQueryInterface(const char *, void **, int *);
void csoundDestroy(void *);
int csoundGetVersion(void);
int csoundGetAPIVersion(void);
void *csoundGetHostData(void *);
void csoundSetHostData(void *, void *);
int csoundPerform(void *, int, char **);
int csoundCompile(void *, int, char **);
int csoundPerformKsmps(void *);
int csoundPerformBuffer(void *);
void csoundCleanup(void *);
void csoundReset(void *);
MYFLT csoundGetSr(void *);
MYFLT csoundGetKr(void *);
int csoundGetKsmps(void *);
int csoundGetNchnls(void *);
int csoundGetSampleFormat(void *);
int csoundGetSampleSize(void *);
long csoundGetInputBufferSize(void *);
long csoundGetOutputBufferSize(void *);
void *csoundGetInputBuffer(void *);
void *csoundGetOutputBuffer(void *);
MYFLT *csoundGetSpin(void *);
MYFLT *csoundGetSpout(void *);
MYFLT csoundGetScoreTime(void *);
MYFLT csoundGetProgress(void *);
MYFLT csoundGetProfile(void *);
MYFLT csoundGetCpuUsage(void *);
int csoundIsScorePending(void *);
void csoundSetScorePending(void *, int);
MYFLT csoundGetScoreOffsetSeconds(void *);
void csoundSetScoreOffsetSeconds(void *, MYFLT);
void csoundRewindScore(void *);
void csoundMessage(void *, const char *, ...);
void csoundMessageV(void *, const char *, va_list);
void csoundThrowMessage(void *, const char *, ...);
void csoundThrowMessageV(void *, const char *, va_list);
void csoundSetMessageCallback(void *, void (*)(void *, const char *, va_list));
void csoundSetThrowMessageCallback(void *, void (*)(void *, const char *, va_list));
int csoundGetMessageLevel(void *);
void csoundSetMessageLevel(void *, int);
void csoundInputMessage(void *, const char *);
void csoundKeyPress(void *, char);
void csoundSetInputValueCallback(void *, void (*)(void *, char *, MYFLT *));
void csoundSetOutputValueCallback(void *, void (*)(void *, char *, MYFLT));
void csoundScoreEvent(void *, char, MYFLT *, long);
void csoundSetExternalMidiOpenCallback(void *, void (*)(void *));
void csoundSetExternalMidiReadCallback(void *, int (*)(void *, unsigned char *, int));
void csoundSetExternalMidiWriteCallback(void *, int (*)(void *, unsigned char *));
void csoundSetExternalMidiCloseCallback(void *, void (*)(void *));
int csoundIsExternalMidiEnabled(void *);
void csoundSetExternalMidiEnabled(void *, int);
void csoundSetIsGraphable(void *, int);
void csoundSetMakeGraphCallback(void *, void (*)(void *, WINDAT *, char *));
void csoundSetDrawGraphCallback(void *, void (*)(void *, WINDAT *));
void csoundSetKillGraphCallback(void *, void (*)(void *, WINDAT *windat));
void csoundSetExitGraphCallback(void *, int (*)(void *));
opcodelist *csoundNewOpcodeList(void);
void csoundDisposeOpcodeList(opcodelist *);
int csoundAppendOpcode(char *, int, int, char *, char *, int (*)(void *),
                       int (*)(void *), int (*)(void *), int (*)(void *));
int csoundLoadExternal(void *, const char *);
int csoundLoadExternals(void *);
void *csoundOpenLibrary(const char *);
void *csoundCloseLibrary(void *);
void *csoundGetLibrarySymbol(void *, const char *);
void csoundSetYieldCallback(void *, int (*)(void *));
void csoundSetEnv(void *, const char *, const char *);
void csoundSetPlayopenCallback(void *, void (*)(int, int, float, int));
void csoundSetRtplayCallback(void *, void (*)(char *, int));
void csoundSetRecopenCallback(void *, void (*)(int, int, float, int));
void csoundSetRtrecordCallback(void *, int (*)(char *, int));
void csoundSetRtcloseCallback(void *, void (*)(void));

void auxalloc(long, AUXCH*);
char *getstring(int, char*);
void die(char *);
FUNC *ftfind(MYFLT *);
int initerror(char *);
int perferror(char *);
void *mmalloc(long);
void mfree(void *);
void dispset(WINDAT *, MYFLT *, long, char *, int, char *);
void display(WINDAT *);
MYFLT intpow(MYFLT, long);
FUNC *ftfindp(MYFLT *argp);
FUNC *ftnp2find(MYFLT *);
char *unquote(char *);
MEMFIL *ldmemfile(char *);

static  MYFLT   *gbloffbas;

OPARMS  O_ = {0,0, 0,1,1,0, 0,0, 0,0, 0,0, 1,0,0,7, 0,0,0, 0,0,0,0, 0,0 };
GLOBALS cglob_ = {
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
        csoundSetExternalMidiOpenCallback,
        csoundSetExternalMidiReadCallback,
        csoundSetExternalMidiWriteCallback,
        csoundSetExternalMidiCloseCallback,
        csoundIsExternalMidiEnabled,
        csoundSetExternalMidiEnabled,
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
        csoundSetEnv,
        csoundSetPlayopenCallback,
        csoundSetRtplayCallback,
        csoundSetRecopenCallback,
        csoundSetRtrecordCallback,
        csoundSetRtcloseCallback,
        /*
         * Intrnal functions
         */
        auxalloc,
        getstring,
        die,
        ftfind,
        initerror,
        perferror,
        mmalloc,
        mfree,
        dispset,
        display,
        intpow,
        ftfindp,
        ftnp2find,
        unquote,
        ldmemfile,
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
        DFLT_DBFS,    /*	e0dbfs */
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
        NULL,  /*     dribble */
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
        NULL, NULL,   /* ssdirpath, sfdirpath */
        NULL, /*      tokenstring */
        NULL, /*      polish */
        NULL, NULL,   /* scorein, scorout */
        FL(0.0), FL(0.0),     /*      ensmps, hfkprd */
        NULL,   /*      pool */
        NULL,   /*      argoffspace */
        NULL,   /*      frstoff */
        0,      /*      sensType */
#if defined(__WATCOMC__) || defined(__POWERPC__) || defined(mills_macintosh)
        {0},
#else
        {{{0}}}, /*      exitjmp of type jmp_buf */
#endif
        NULL,   /*      frstbp */
        0,      /*      sectcnt */
        {0},    /*      m_chnbp */
        NULL, NULL,   /*  cpsocint, cpsocfrc */
        0, 0, 0,      /* inerrcnt, synterrcnt, perferrcnt */
        0,      /*      MIDIoutDONE */
        -1,     /*      midi_out */
        "",     /*      strmsg */
        {NULL}, /*      instxtanchor */
        {NULL}, /*      actanchor */
        {0L },  /*      rngcnt */
        0, 0,   /*      rngflg, multichan */
        {{NULL}}, /*      OrcTrigEvts */
        "",     /*      full_name */
        0, 0, 0,      /*      Mforcdecs, Mxtroffs, MTrkend */
        FL(-1.0), FL(-1.0),   /* tran_sr,tran_kr */
        FL(-1.0),     /* tran_ksmps */
        FL(32767.0),  /*      tran_0dbfs */
        DFLT_NCHNLS,  /*      tran_nchnls */
        FL(-1.0), FL(-1.0), FL(-1.0), FL(-1.0), /* tpidsr, pidsr, mpidsr, mtpdsr */
        NULL,   /*      sadirpath */
        NULL,   /*      oplibs */
        &O,     /*      oparms */
        NULL,   /*      hostData */
        NULL,   /*      opcodeInfo  */
        NULL,   /*      instrumentNames */
        FL(0.0),        /*      dbfs_to_short */
        FL(0.0),        /*      short_to_dbfs */
        FL(0.0),        /*      dbfs_to_float */
        FL(0.0),        /*      float_to_dbfs */
        FL(0.0),        /*      dbfs_to_long */
        FL(0.0),        /*      long_to_dbfs */
        1024,   /*      rtin_dev */
        1024,   /*      rtout_dev */
        0,	/*	MIDIINbufIndex */
        {{0}},	/*	MIDIINbuffer2 */
        -1	/*	displop4 */
};

OPARMS O;

GLOBALS cglob;

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
          mfree(ip->opcod_iobufs);
        if (ip->fdch.nxtchp) fdchclose(ip);
        if (ip->auxch.nxtchp) auxchfree(ip);
        mfree(ip); ip = nxtip;
      }
      while (bp) {                              /* and opcode texts */
        OPTXT *nxtbp = bp->nxtop;
        mfree(bp); bp = nxtbp;
      }
      mfree(tp);
      tp = nxttp;
    }
    mfree(instrtxtp);           /* Start again */
    maxinsno    = MAXINSNO;
    maxopcno = -1; instrtxtp = NULL;    /* IV - Oct 24 2002 */
    e0dbfs      = DFLT_DBFS;
    /**
     * The first time around, assign cglob_ to glob_ wholesale.
     */
    if (!cglob.GetVersion) {
      memcpy(&cglob, &cglob_, sizeof(GLOBALS));
      cglob.GetVersion = csoundGetVersion;
    }
    /**
     * The next time, copy everything EXCEPT the function pointers.
     * This is tricky because of those blasted macros!
     * We do it by saving them and copying them back again...
     */
    else {
      GLOBALS tempGlobals = cglob;
      size_t front = (size_t)&tempGlobals;
      size_t back = (size_t)&tempGlobals.SetRtcloseCallback;
      size_t length = back - front;
      back += sizeof(tempGlobals.SetRtcloseCallback);
      cglob = cglob_;
      memcpy(&cglob, &tempGlobals, length);
    }
    O = O_;
    cglob.oparms = &O;
    /* IV - Sep 8 2002: also reset saved globals */
    global_ksmps = ksmps_; global_ensmps = ensmps; global_ekr = ekr_;
    global_onedkr = onedkr; global_hfkprd = hfkprd; global_kicvt = kicvt;
    global_kcounter = kcounter;
    rtin_dev = 1024;
    rtout_dev = 1024;
}

void oload(void)
{
    long   n, nn, combinedsize, gblabeg, lclabeg, insno, *lp;
    MYFLT  *combinedspc, *gblspace, *fp1, *fp2;
/*     short  *pgmdim = NULL; */
    INSTRTXT *ip;
    OPTXT *optxt;
    esr_ = tran_sr; ekr_ = tran_kr; ksmps_ = (int) (ensmps = tran_ksmps);
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
        short rindex = (short)outoffp->indx[0] - (short)O.poolcount;
        MYFLT conval = pool[inoffp->indx[0] - 1];
        switch(rindex) {
        case 1:  esr_ = conval;  break;  /* & use those values */
        case 2:  ekr_ = conval;  break;  /*  to set params now */
        case 3:  ksmps_ = (int)(ensmps = conval);  break;
        case 4:  nchnls = (int)conval;  break;
        case 5:  e0dbfs = conval; break;
        default: break;
        }
      }
    }
    /* why I want oload() to return an error value.... */
    if (e0dbfs <= 0.0)
      die(Str(X_1711,"bad value for 0dbfs: must be positive."));
    if (O.odebug)
      printf("esr = %7.1f, ekr = %7.1f, ksmps = %d, nchnls = %d 0dbfs = %.1f\n",
             esr_ ,ekr_, ksmps_, nchnls,e0dbfs);  ;
    if (O.sr_override) {        /* if command-line overrides, apply now */
      esr_ = (MYFLT)O.sr_override;
      ekr_ = (MYFLT)O.kr_override;
      ksmps_ = (int)(ensmps = (MYFLT)(O.sr_override / O.kr_override));
      printf(Str(X_1173,"sample rate overrides: esr = %7.1f, ekr = %7.1f, ksmps = %d\n"),
             esr_, ekr_, ksmps_);
    }
    combinedsize = (O.poolcount + O.gblfixed + O.gblacount * ksmps_)
      * sizeof(MYFLT);
    combinedspc = (MYFLT *)mcalloc((long)combinedsize);
    for (fp1 = pool, fp2 = combinedspc, nn = O.poolcount; nn--; )
      *fp2++ = *fp1++;              /* copy pool into combined space */
    mfree((void *)pool);
    pool = combinedspc;
    gblspace = pool + O.poolcount;
    gblspace[0] = esr_;              /*   & enter        */
    gblspace[1] = ekr_;              /*   rsvd word      */
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
#ifdef __alpha__
      /*
       * On Alpha, we need to align on 2*sizeof(MYFLT) (i.e. 64 bits).  So
       * we round up to that size.  heh 981101
       */
      ip->localen = ((ip->lclfixed + ip->lclacnt*ksmps_ + 1) & ~0x1) *
        sizeof(MYFLT);
#else
     ip->localen = (ip->lclfixed + ip->lclacnt*ksmps_) * sizeof(MYFLT);
#endif
      for (insno=0, n=0; insno <= maxinsno; insno++)
        if (instrtxtp[insno] == ip)  n++;              /* count insnos  */
      lp = ip->inslist = (long *) mmalloc((long)(n+1) * sizeof(long));
      for (insno=0; insno <= maxinsno; insno++)
        if (instrtxtp[insno] == ip)  *lp++ = insno;    /* creat inslist */
      *lp = -1;                                        /*   & terminate */
      insno = *ip->inslist;                            /* get the first */
      while ((optxt = optxt->nxtop) !=  NULL) {
        TEXT *ttp = &optxt->t;
        ARGOFFS *aoffp;
        long  indx;
        long posndx;
        short *ndxp;
        int opnum = ttp->opnum;
        if (opnum == ENDIN || opnum == ENDOP) break;    /* IV - Sep 8 2002 */
        if (opnum == LABEL) continue;
        aoffp = ttp->outoffs;
        n=aoffp->count;
        for (ndxp=aoffp->indx; n--; ndxp++) {
/*           printf("**** indx = %d (%x); gblabeg=%d lclabeg=%d\n", *ndxp, *ndxp,gblabeg,lclabeg ); */
          if ((indx = *ndxp) > gblabeg) {
            indx = gblabeg + (indx - gblabeg) * ksmps_;
          }
          else if (indx <= 0 && (posndx = -indx) > lclabeg
                   && indx >= LABELIM) {
            indx = -(lclabeg + (posndx - lclabeg) * ksmps_);
          }
          else if (indx > 0 && indx <= 3 && O.sr_override
                   && ip == instxtanchor.nxtinstxt) { /* for instr 0 */
            indx += 3;    /* deflect any old sr,kr,ksmps targets */
          }
          else continue;
          if ((short)indx != indx) {
            printf(Str(X_910,"indx=%ld (%lx); (short)indx = %d (%x)\n"),
                   indx, indx, (short)indx, (short)indx);
/*             die(Str(X_909,"indexing overflow error")); */
          }
          *ndxp = (short)indx;
        }
        aoffp = ttp->inoffs;            /* inargs:                  */
        if (opnum >= SETEND) goto realops;
        switch(opnum) {                 /*      do oload SETs NOW  */
        case STRSET:
          if (strsets == NULL)
            strsets = (char **)
              mcalloc((long)((strsmax=STRSMAX)+1) * sizeof(char *));
          indx = (long)gbloffbas[*aoffp->indx];
          if (indx >= strsmax) {
            long newmax = strsmax + STRSMAX;
            int i;
            while (indx > newmax) newmax += STRSMAX;
            strsets = (char**)mrealloc(strsets, (newmax+1)*sizeof(char *));
            /* ??? */
            for (i=strsmax; i<newmax+1; i++) strsets[i] = NULL;
/*             for (i=0; i<newmax+1; i++) printf("strset[%d]: %p\n", i, strsets[i]); */
            strsmax = newmax;
          }
          if (strsets == NULL || indx < 0) { /* No space left or -ve index */
            if (O.msglevel & WARNMSG)
              printf(Str(X_887,"WARNING: illegal strset index\n"));
            longjmp(cglob.exitjmp,1);
          }
          if (strsets[indx] != NULL) {
            if (O.msglevel & WARNMSG)
              printf(Str(X_1249,"WARNING: strset index conflict\n"));
          }
          else {
            strsets[indx] = ttp->strargs[0];
          }
          printf("Strsets[%d]:%s\n", indx, strsets[indx]);
          break;
        case PSET:
          printf("PSET: isno=%d, pmax=%d\n", insno, ip->pmax);
          if ((n = aoffp->count) != ip->pmax) {
            if (O.msglevel & WARNMSG)
              printf(errmsg,Str(X_834,"WARNING: i%ld pset args != pmax\n"), insno);
            if (n < ip->pmax) n = ip->pmax; /* cf pset, pmax    */
          }                                   /* alloc the larger */
          ip->psetdata = (MYFLT *) mcalloc((long)n * sizeof(MYFLT));
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
            indx = gblabeg + (indx - gblabeg) * ksmps_;
          }
          else if (indx <= 0 && (posndx = -indx) > lclabeg
                   && indx >= LABELIM) {
            indx = -(lclabeg + (posndx - lclabeg) * ksmps_);
          }
          else continue;
          if ((short)indx != indx) {
            printf(Str(X_218,"Case2: indx=%ld (%lx); (short)indx = %d (%x)\n"),
                   indx, indx, (short)indx, (short)indx);
/*          die(Str(X_909,"indexing overflow error")); */
          }
          *ndxp = (short)indx;
        }
      }
      if (!POLL_EVENTS()) longjmp(cglob.exitjmp,1); /* on Mac/Win, allow system events */
    }
/*     if (pgmdim != NULL) free((char *)pgmdim); */
/*     pctlist = (MYFLT **) mcalloc((long)256 * sizeof(MYFLT *)); */
/*     insbusy = (short *) mcalloc((long)((maxinsno+1) * sizeof(short))); */

    if ((nn = init0()) > 0)                             /* run instr 0 inits */
      die(Str(X_828,"header init errors"));
    if ((ensmps != (MYFLT) ksmps_) ||
        (gblspace[0]/gblspace[1] != gblspace[2])) {     /* & chk consistency */
      printf("sr = %f, kr = %f, ksmps = %f\n",
             gblspace[0], gblspace[1], gblspace[2]);
      die(Str(X_903,"inconsistent sr, kr, ksmps"));             /*   one more time   */
    }
    tpidsr = TWOPI_F / esr_;                             /* now set internal  */
    mtpdsr = -tpidsr;                                   /*    consts         */
    pidsr = PI_F / esr_;
    mpidsr = -pidsr;
    sicvt = FMAXLEN / esr_;
    kicvt = FMAXLEN / ekr_;
    hfkprd = FL(0.5) / ekr_;
    onedsr = FL(1.0) / esr_;
    onedkr = FL(1.0) / ekr_;
    /* IV - Sep 8 2002: save global variables that depend on ksmps */
    global_ksmps = ksmps_; global_ensmps = ensmps; global_ekr = ekr_;
    global_onedkr = onedkr; global_hfkprd = hfkprd; global_kicvt = kicvt;
    global_kcounter = kcounter;
/*  dv32768 = FL(1.0) / FL(32768.0);            IV - Jul 11 2002 */
    cpsoctinit();
/*     reverbinit(); */
    sssfinit();
    dbfs_init(e0dbfs);
/*  dv32768 = dbfs_to_float;                    IV - Jul 11 2002 */
    nspin = nspout = ksmps_ * nchnls;                    /* alloc spin & spout */
    spin =  (MYFLT *) mcalloc((long)nspin*sizeof(MYFLT));
    spout = (MYFLT *) mcalloc((long)nspout*sizeof(MYFLT));
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
        short   indx, posndx;
        short   *ndxp;
        MCHNBLK *chp = NULL;

        lopds = (LBLBLK**)mmalloc(sizeof(LBLBLK*)*nlabels);
        lopdsp = lopds;
        larg = (LARGNO*)mmalloc(sizeof(LARGNO)*ngotos);
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
                printf(Str(X_929,"instr %d seeking midi chnl data, assigned chnl %d\n"),
                       insno, chp->insno);
                break;
              }
              if (chp->insno != insno)
                continue;
            }
          }
        }
        pextent = sizeof(INSDS) + tp->pextrab;          /* alloc new space,  */
        ip = (INSDS *) mcalloc((long)pextent + tp->localen + tp->opdstot);
                ip->csound = &cglob;
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
          ip->nxtact = tp->act_instance;        /* next free instance for */
          tp->act_instance = ip;                /* user opcodes */
          pcnt = (int) tp->opcode_info->perf_incnt;
          pcnt += (int) tp->opcode_info->perf_outcnt;
          pcnt = sizeof(OPCOD_IOBUFS) + sizeof(MYFLT*) * (pcnt << 1);
          ip->opcod_iobufs = (void*) mmalloc(pcnt);
        }
        lcloffbas = &ip->p0;
        lclbas = (MYFLT *)((char *)ip + pextent);       /* split local space */
        nxtopds = (char *)lclbas + tp->localen;
        opdslim = nxtopds + tp->opdstot;
        if (O.odebug)
          printf(Str(X_923,"instr %d allocated at %p\n\tlclbas %p, opds %p\n"),
                 insno,ip,lclbas,nxtopds); 
        optxt = (OPTXT *)tp;
        prvids = prvpds = (OPDS *)ip;
        while ((optxt = optxt->nxtop) != NULL) {     /* for each op in instr */
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
            ep = &opcodlst[opnum];                      /* for all ops:     */
            opds = (OPDS *) nxtopds;                    /*   take reqd opds */
            nxtopds += ep->dsblksiz;
            if (O.odebug)
              printf(Str(X_1091,"op %d (%s) allocated at %p\n"),
                     opnum,ep->opname,opds); 
            opds->optext = optxt;                       /* set common headata */
            opds->insdshead = ip;
            if (opnum == LABEL) {                       /* LABEL:       */
                LBLBLK  *lblbp = (LBLBLK *) opds;
                lblbp->prvi = prvids;                   /*    save i/p links */
                lblbp->prvp = prvpds;
                *lopdsp++ = lblbp;                      /*    log the lbl bp */
                continue;                               /*    for later refs */
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
            if ((ep->thread & 01)!=0) {                 /* thread 1:        */
                prvids = prvids->nxti = opds;           /* link into ichain */
                opds->iopadr = ep->iopadr;              /*   & set exec adr */
                if (opds->iopadr == NULL)
                    die(Str(X_1082,"null iopadr"));
            }
            if ((n = ep->thread & 06)!=0) {                     /* thread 2 OR 4:   */
                prvpds = prvpds->nxtp = opds;           /* link into pchain */
                if (!(n & 04) ||
                    (ttp->pftype == 'k' && ep->kopadr != NULL))
                    opds->opadr = ep->kopadr;           /*      krate or    */
                else opds->opadr = ep->aopadr;          /*      arate       */
                if (O.odebug) printf("opadr = %p\n",opds->opadr); 
                if (opds->opadr == NULL)
                    die(Str(X_1083,"null opadr"));
            }
            opds->dopadr = ep->dopadr;
        args:
            argpp = (MYFLT **)((char *)opds + sizeof(OPDS));
            if (O.odebug) printf("argptrs:"); 
            aoffp = ttp->outoffs;               /* for outarg codes: */
            reqd = strlen(ep->outypes);
            for (n=aoffp->count, ndxp=aoffp->indx; n--; reqd--) {
                if ((indx = *ndxp++) > 0)           /* cvt index to lcl/gbl adr */
                    fltp = gbloffbas + indx;
                else if ((posndx = -indx) < CBAS)
                    fltp = lcloffbas + posndx;
                else
                    fltp = csetoffbas + posndx - CBAS;
                if (O.odebug) printf("\t%p", fltp); 
                *argpp++ = fltp;
            }
            while (reqd--) {                    /* if more outypes, pad */
                if (O.odebug) printf("\tPADOUT"); 
                *argpp++ = NULL;
            }
            aoffp = ttp->inoffs;                /* for inarg codes: */
            for (n=aoffp->count, ndxp=aoffp->indx; n--; ) {
                if ((indx = *ndxp++) < LABELIM) {
                    if (O.odebug) printf("\t***lbl"); 
                    largp->lblno = indx - MIN_SHORT;  /* if label ref, defer */
                    largp->argpp = argpp++;
                    largp++;
                } else {
                    if (indx > 0)                   /* else cvt ndx to lcl/gbl */
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
          printf(Str(X_1087,"nxtopds = %p opdslim = %p\n"), nxtopds, opdslim);
          if (nxtopds > opdslim) die(Str(X_902,"inconsistent opds total"));
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

