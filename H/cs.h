#ifndef CS_H            /*                                      CS.H    */
#define CS_H (1)

#ifdef __cplusplus
extern "C" {
#endif

/*  
    cs.h:

    Copyright (C) 1991-2003 Barry Vercoe, John ffitch

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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#ifdef HAVE_STRING_H
# include <string.h>
#elif HAVE_STRINGS_H
# include <strings.h>
#endif
#include "sysdep.h"
#include "cwindow.h"
#include "opcode.h"

#include "version.h"

#define OK (0)
#define NOTOK (-1)

#define INSTR     1
#define ENDIN     2
#define OPCODE    3
#define ENDOP     4
#define LABEL     5
#define SETBEG    6
#define STRSET    6
#define PSET      7
#define SETEND    8
/* #define VSET      6 */
/* #define GVSET     7 */
/* #define VDIM      8 */
/* #define VPROGS    9 */
/* #define DVPROGS   10 */
/* #define PGMINIT   17 */
/* #define DPGMINIT  18 */

#define MAXINSNO 200
#define PMAX     1000
#define VARGMAX  1001
/* IV - Oct 24 2002: max number of input/output args for user defined opcodes */
#define OPCODENUMOUTS   24

#define ORTXT      h.optext->t
#define INCOUNT    ORTXT.inlist->count
#define OUTCOUNT   ORTXT.outlist->count
#define INOCOUNT   ORTXT.inoffs->count
#define OUTOCOUNT  ORTXT.outoffs->count
#define XINCODE    ORTXT.xincod
#  define XINARG1  (p->XINCODE & 2)
#  define XINARG2  (p->XINCODE & 1)
#  define XINARG3  (p->XINCODE & 4)
#  define XINARG4  (p->XINCODE & 8)
#define XOUTCODE   ORTXT.xoutcod        /* IV - Sep 1 2002 */
#define STRARG     ORTXT.strargs[0]
#define STRARG2    ORTXT.strargs[1]
#define STRARG3    ORTXT.strargs[2]
#define STRARG4    ORTXT.strargs[3]

#define MAXLEN     0x1000000L
#define FMAXLEN    ((MYFLT)(MAXLEN))
#define PHMASK     0x0FFFFFFL
#define PFRAC(x)   ((MYFLT)((x) & ftp->lomask) * ftp->lodiv)
#define MAXPOS     0x7FFFFFFFL

#define BYTREVS(n) ((n>>8  & 0xFF) | (n<<8 & 0xFF00))
#define BYTREVL(n) ((n>>24 & 0xFF) | (n>>8 & 0xFF00L) | (n<<8 & 0xFF0000L) | (n<<24 & 0xFF000000L))

#define NOCTS      20
#define OCTRES     8192
#define RESMASK    8191L
#define RESHIFT    13
#define CPSOCTL(n) cpsocint[n >> RESHIFT] * cpsocfrc[n & RESMASK]

#define LOBITS     10
#define LOFACT     1024
  /* LOSCAL is 1/LOFACT as MYFLT */
#define LOSCAL     FL(0.0009765625)

#define LOMASK     1023

#define SSTRCOD    0xFFFFFL
#define SSTRSIZ    200
#define ALLCHNLS   0x7fff
#define DFLT_SR    FL(44100.0)
#define DFLT_KR    FL(4410.0)
#define DFLT_KSMPS 10
#define DFLT_NCHNLS 1
#define MAXCHNLS   256

#define MAXNAME 128

#define DFLT_DBFS (FL(32767.0))
extern MYFLT e0dbfs;
void dbfs_init(MYFLT dbfs);

typedef struct {
        int     odebug, initonly;
        int     sfread, sfwrite, sfheader, filetyp;
        int     inbufsamps, outbufsamps;
        int     informat, outformat;
        int     insampsiz, outsampsiz;
        int     displays, graphsoff, postscript, msglevel;
        int     Beatmode, cmdTempo, oMaxLag;
        int     usingcscore, Linein, Midiin, FMidiin;
        int     OrcEvts;        /* - for triginstr (re Aug 1999) */
        int     RTevents, ksensing;
        int     ringbell, termifend, stdoutfd;
        int     rewrt_hdr, heartbeat, gen01defer;
#ifdef LINUX
        int     Volume;         /* Jonathan Mohr  1995 Oct 17 */
#endif
        long    sr_override, kr_override;
        long    instxtcount, optxtsize;
        long    poolcount, gblfixed, gblacount;
        long    argoffsize, strargsize, filnamsize;
        char    *argoffspace, *strargspace, *filnamspace;
        char    *infilename, *outfilename, *playscore;
        char    *Linename, *Midiname, *FMidiname;
#ifdef __BEOS__
        char *Midioutname; /* jjk 09252000 - MIDI output device, -Q option */
#endif
} OPARMS;

#define  ONEPT          1.021975               /* A440 tuning factor */
#define  LOG10D20       0.11512925             /* for db to ampfac   */
#define  DV32768        FL(0.000030517578125)

typedef struct polish {
        char    opcod[12];
        short   incount;
        char    *arg[4];     /* Was [4][12] */
} POLISH;

typedef struct arglst {
        short   count;
        char    *arg[1];
} ARGLST;

typedef struct argoffs {
        short   count;
        short   indx[1];
} ARGOFFS;

/* Storage for parsed orchestra code, for each opcode in an INSTRTXT. */
typedef struct text {
        short   linenum;        /* Line num in orch file (currently buggy!)  */
        short   opnum;          /* Opcode index in opcodlst[] */
        char    *opcod;         /* Pointer to opcode name in global pool */
        char    *strargs[4];    /* (Unquoted) array of file names if opcode uses */
        ARGLST  *inlist;        /* Input args (pointer to item in name list) */
        ARGLST  *outlist;
        ARGOFFS *inoffs;        /* Input args (index into list of values) */
        ARGOFFS *outoffs;
        short   xincod;         /* Rate switch for multi-rate opcode functions */
        short   xoutcod;        /* output rate switch (IV - Sep 1 2002) */
        char    intype;         /* Type of first input argument (g,k,a,w etc) */
        char    pftype;         /* Type of output argument (k, a etc) */
} TEXT;

/* This struct is filled out by otran() at orch parse time.
   It is used as a template for instrument events. */
typedef struct instr {
        struct op * nxtop;              /* Linked list of instr opcodes */
        TEXT    t;                      /* Text of instrument (same in nxtop) */
        short   pmax, vmax, pextrab;    /* Arg count, size of data for all
                                           opcodes in instr */
        short   mdepends;               /* Opcode type (i/k/a) */
        short   lclkcnt, lcldcnt;       /* Storage reqs for this instr */
        short   lclwcnt, lclacnt;
        short   lclpcnt;
        short   lclfixed, optxtcount;
        short   muted;
        long    localen;
        long    opdstot;                /* Total size of opds structs in instr */
        long    *inslist;               /* Only used in parsing (?) */
        MYFLT   *psetdata;              /* Used for pset opcode */
        struct insds * instance;        /* Chain of allocated instances of
                                           this instrument */
        struct insds * lst_instance, *act_instance;     /* IV - Oct 26 2002 */
        struct instr * nxtinstxt;       /* Next instrument in orch (num order) */
        int     active;                 /* To count activations for control */
        int     maxalloc;
        MYFLT   cpuload;                /* % load this instrumemnt makes */
        struct opcodinfo *opcode_info;  /* IV - Nov 10 2002 */
        char    *insname;               /* instrument name */
} INSTRTXT;


/* A chain of TEXT structs. Note that this is identical with the first two
   members of struct INSTRTEXT, and is so typecast at various points in code. */
typedef struct op {
        struct op * nxtop;
        TEXT    t;
} OPTXT;

typedef struct fdch {
        struct fdch * nxtchp;
        int    fd;
} FDCH;

typedef struct auxch {
        struct auxch * nxtchp;
        long   size;
        void   *auxp, *endp;    /* was char* */
} AUXCH;

typedef struct monblk {
        short  pch;
        struct monblk *prv;
} MONPCH;

typedef struct {
        int     notnum[4];
} DPEXCL;

typedef struct {
        DPEXCL dpexcl[8];
        int    exclset[75];     /* for keys 25-99 */
} DPARM;

typedef struct dklst {
        struct dklst *nxtlst;
        long   pgmno;
        MYFLT  keylst[1];       /* cnt + keynos */
} DKLST;

#define DKBAS  25

typedef struct mchnblk {
        short  pgmno;
        short  insno;
/*      short  Omni; */
/*      short  Poly; */
/*         short  bas_chnl; */
/*      short  nchnls; */
        short  RegParNo;
        short  mono;
        MONPCH *monobas;
        MONPCH *monocur;
        struct insds *kinsptr[128];
        struct insds *ksusptr[128];
        MYFLT  polyaft[128];
        MYFLT  ctl_val[128];    /* ... with GS vib_rate,, stored in c102-c109 */
/*      float  katouch[128]; */
/*      short  ctl_byt[128]; */
        short  ksuscnt;
        short  sustaining;
        MYFLT  aftouch;
/*      float  chnpress; */
        MYFLT  pchbend;
/*      float  posbend; */
/*      float  pbensens; */
        DKLST  *klists;         /* chain of dpgm keylists */
        DPARM  *dparms;         /* drumset params     */
        int    dpmsb;
        int    dplsb;
/*      float  finetune; */
/*      float  crsetune; */
  /*    float  tuning; */    /* displ in semitones */
} MCHNBLK;

/*
*       Forward declaration.
*/
struct ENVIRON_;

/* This struct holds the info for a concrete instrument event
   instance in performance. */
typedef struct insds {
        struct opds * nxti;             /* Chain of init-time opcodes */
        struct opds * nxtp;             /* Chain of performance-time opcodes */
        struct insds * nxtinstance;     /* Next allocated instance */
        struct insds * prvinstance;     /* Previous allocated instance */
        struct insds * nxtact;          /* Next in list of active instruments */
        struct insds * prvact;          /* Previous in list of active instruments */
        struct insds * nxtoff;          /* Next instrument to terminate */
        FDCH    fdch;           /* Chain of files used by opcodes in this instr */
        AUXCH   auxch;          /* Extra memory used by opcodes in this instr */
        MCHNBLK *m_chnbp;       /* MIDI note info block if event started from MIDI */
        short   m_pitch;        /* MIDI pitch, for simple access */
        short   m_veloc;        /* ...ditto velocity */
        int     xtratim;        /* Extra release time requested with xtratim opcode */
        char    relesing;       /* Flag to indicate we are releasing, test with release opcode */
        char    actflg;         /* Set if instr instance is active (perfing) */
        short   insno;          /* Instrument number */
        MYFLT   offbet;         /* Time to turn off event, in score beats */
        MYFLT   offtim;         /* Time to turn off event, in seconds (negative on indef/tie) */
        struct insds * nxtolap; /* ptr to next overlapping voice */
         /* end of overlap */
        struct ENVIRON_ *csound;/* ptr to Csound engine and API for externals */
        void    *opcod_iobufs;  /* IV - Sep 8 2002: user opcode I/O buffers */
        void    *opcod_deact, *subins_deact;    /* IV - Oct 24 2002 */
        MYFLT   p0;             /* Copy of required p-field values for quick access */
        MYFLT   p1;
        MYFLT   p2;
        MYFLT   p3;
} INSDS;

typedef int    (*SUBR)(void *);

/* This struct holds the info for one opcode in a concrete
   instrument instance in performance. */
typedef struct opds {
        struct opds * nxti;     /* Next opcode in init-time chain */
        struct opds * nxtp;     /* Next opcode in perf-time chain */
        SUBR    iopadr;         /* Initialization (i-time) function pointer */
        SUBR    opadr;          /* Perf-time (k- or a-rate) function pointer */
/**
* Deinitialization function pointer;
* if not null, called during cleanup on each opcode instance;
* useful for deallocating memory or other resources managed by the opcode.
*/
        SUBR    dopadr;         /* Deinitialization function pointer */
        OPTXT   *optext;        /* Orch file template part for this opcode */
        INSDS   *insdshead;     /* Owner instrument instance data structure */
} OPDS;

typedef struct lblblk {
        OPDS    h;
        OPDS    *prvi;
        OPDS    *prvp;
} LBLBLK;

typedef struct oentry {
        char    *opname;
        unsigned short  dsblksiz;
        unsigned short  thread;
        char    *outypes;
        char    *intypes;
        SUBR    iopadr;
        SUBR    kopadr;
        SUBR    aopadr;
/**
* Deinitialization function pointer;
* if not null, called during cleanup on each opcode instance;
* useful for deallocating memory or other resources managed by the opcode.
*/
        SUBR    dopadr;
        void    *useropinfo;    /* IV - Oct 12 2002: user opcode parameters */
        int     prvnum;         /* IV - Oct 31 2002 */
} OENTRY;

typedef struct {
        MYFLT   *begp, *curp, *endp, feedback[6];
        long    scount;
} OCTDAT;

#define MAXOCTS 8

typedef struct {
        long    npts, nocts, nsamps;
        MYFLT   lofrq, hifrq, looct, srate;
        OCTDAT  octdata[MAXOCTS];
        AUXCH   auxch;
} DOWNDAT;

typedef struct {
        long    ktimstamp, ktimprd;
        long    npts, nfreqs, dbout;
        DOWNDAT *downsrcp;
        AUXCH   auxch;
} SPECDAT;

#define AIFF_MAXCHAN 8
typedef struct {
        MYFLT   natcps;
        MYFLT   gainfac;
        short   loopmode1;
        short   loopmode2;
        long    begin1, end1;
        long    begin2, end2;
        MYFLT   fmaxamps[AIFF_MAXCHAN+1];
} AIFFDAT;

typedef struct {
        MYFLT gen01;
        MYFLT ifilno;
        MYFLT iskptim;
        MYFLT iformat;
        MYFLT channel;
        MYFLT sample_rate;
        char  strarg[SSTRSIZ];
} GEN01ARGS;

typedef struct {
        long    flen;
        long    lenmask;
        long    lobits;
        long    lomask;
        MYFLT   lodiv;
        MYFLT   cvtbas, cpscvt;
        short   loopmode1;
        short   loopmode2;
        long    begin1, end1;   /* all these in ..  */
        long    begin2, end2;
        long    soundend, flenfrms; /* .. sample frames */
        long    nchanls;
        long    fno;
        GEN01ARGS gen01args;
        MYFLT   ftable[1];
} FUNC;

typedef struct MEMFIL {
        char    filename[256];  /* Made larger RWD */
        char    *beginp;
        char    *endp;
        long    length;
 struct MEMFIL  *next;
} MEMFIL;

/* This struct holds the data for one score event. */
typedef struct event {
        char    *strarg;        /* Original argument list string of event */
        char    opcod;          /* Event type */
        short   pcnt;           /* Number of p-fields */
        MYFLT   p2orig;         /* Event start time */
        MYFLT   p3orig;         /* Length */
        MYFLT   offtim;         /* k-time to turn off this event */
        MYFLT   p[PMAX+1];      /* All p-fields for this event */
} EVTBLK;

typedef struct eventnode {
        EVTBLK evt; /* Must be first in struct so it can be typecast & freed */
        struct eventnode *nxtevt;
        int    kstart, insno;
} EVTNODE;

typedef struct {
        OPDS    h;
        MYFLT   *ktempo, *istartempo;
        MYFLT   prvtempo;
} TEMPO;

typedef struct opcodinfo {              /* IV - Oct 24 2002 */
        long    instno;
        char    *name, *intypes, *outtypes;
        short   inchns, outchns, perf_incnt, perf_outcnt;
        short   *in_ndx_list, *out_ndx_list;
        INSTRTXT *ip;
        struct  opcodinfo *prv;
} OPCODINFO;

typedef void (*RSET)(struct ENVIRON_*);
typedef struct resetter {
       RSET            fn;
       struct resetter *next;
} RESETTER;

FUNC   *ftfind(MYFLT*);
MEMFIL *ldmemfile(char *);
#define MAXCHAN       96        /* for 6 ports */

#include "sort.h"
#include "midiops2.h"

typedef struct ENVIRON_
{
  int (*GetVersion)(void);
  int (*GetAPIVersion)(void);
  void *(*GetHostData)(void *csound);
  void (*SetHostData)(void *csound, void *hostData);
  int (*Perform)(void *csound, int argc, char **argv);
  int (*Compile)(void *csound, int argc, char **argv);
  int (*PerformKsmps)(void *csound);
  int (*PerformBuffer)(void *csound);
  void (*Cleanup)(void *csound);
  void (*Reset)(void *csound);
  MYFLT (*GetSr)(void *csound);
  MYFLT (*GetKr)(void *csound);
  int (*GetKsmps)(void *csound);
  int (*GetNchnls)(void *csound);
  int (*GetSampleFormat)(void *csound);
  int (*GetSampleSize)(void *csound);
  long (*GetInputBufferSize)(void *csound);
  long (*GetOutputBufferSize)(void *csound);
  void *(*GetInputBuffer)(void *csound);
  void *(*GetOutputBuffer)(void *csound);
  MYFLT *(*GetSpin)(void *csound);
  MYFLT *(*GetSpout)(void *csound);
  MYFLT (*GetScoreTime)(void *csound);
  MYFLT (*GetProgress)(void *csound);
  MYFLT (*GetProfile)(void *csound);
  MYFLT (*GetCpuUsage)(void *csound);
  int (*IsScorePending)(void *csound);
  void (*SetScorePending)(void *csound, int pending);
  MYFLT (*GetScoreOffsetSeconds)(void *csound);
  void (*SetScoreOffsetSeconds)(void *csound, MYFLT offset);
  void (*RewindScore)(void *csound);
  void (*Message)(void *csound, const char *format, ...);
  void (*MessageV)(void *csound, const char *format, va_list args);
  void (*ThrowMessage)(void *csound, const char *format, ...);
  void (*ThrowMessageV)(void *csound, const char *format, va_list args);
  void (*SetMessageCallback)(void *csound,
                             void (*csoundMessageCallback)(void *hostData,
                                                           const char *format,
                                                           va_list valist));
  void (*SetThrowMessageCallback)(void *csound,
                                  void (*throwMessageCallback)(void *hostData,
                                                               const char *format,
                                                               va_list valist));
  int (*GetMessageLevel)(void *csound);
  void (*SetMessageLevel)(void *csound, int messageLevel);
  void (*InputMessage)(void *csound, const char *message_);
  void (*KeyPress)(void *csound, char c);
  void (*SetInputValueCallback)(void *csound,
                                void (*inputValueCalback)(void *hostData,
                                                          char *channelName,
                                                          MYFLT *value));
  void (*SetOutputValueCallback)(void *csound,
                                 void (*outputValueCalback)(void *hostData,
                                                            char *channelName,
                                                            MYFLT value));
  void (*ScoreEvent)(void *csound, char type, MYFLT *pFields, long numFields);
  void (*SetExternalMidiOpenCallback)(void *csound,
                                      void (*midiOpenCallback)(void *hostData));
  void (*SetExternalMidiReadCallback)(void *csound,
                                      int (*readMidiCallback)(void *hostData,
                                                              unsigned char *midiData,
                                                              int size));
  void (*SetExternalMidiWriteCallback)(void *csound,
                                       int (*writeMidiCallback)(void *hostData,
                                                                unsigned char *midiData));
  void (*SetExternalMidiCloseCallback)(void *csound,
                                       void (*closeMidiCallback)(void *hostData));
  int (*IsExternalMidiEnabled)(void *csound);
  void (*SetExternalMidiEnabled)(void *csound, int enabled);
  void (*SetIsGraphable)(void *csound, int isGraphable);
  void (*SetMakeGraphCallback)(void *csound,
                               void (*makeGraphCallback)(void *hostData,
                                                         WINDAT *p,
                                                         char *name));
  void (*SetDrawGraphCallback)(void *csound,
                               void (*drawGraphCallback)(void *hostData, WINDAT *p));
  void (*SetKillGraphCallback)(void *csound,
                               void (*killGraphCallback)(void *hostData, WINDAT *p));
  void (*SetExitGraphCallback)(void *csound, int (*exitGraphCallback)(void *hostData));
  opcodelist *(*NewOpcodeList)(void);
  void (*DisposeOpcodeList)(opcodelist *opcodelist_);
  int (*AppendOpcode)(char *opname, int dsblksiz, int thread,
                      char *outypes, char *intypes, SUBR iopadr,
                      SUBR kopadr, SUBR aopadr, SUBR dopadr);
  int (*LoadExternal)(void *csound, const char *libraryPath);
  int (*LoadExternals)(void *csound);
  void *(*OpenLibrary)(const char *libraryPath);
  void *(*CloseLibrary)(void *library);
  void *(*GetLibrarySymbol)(void *library, const char *procedureName);
  void (*SetYieldCallback)(void *csound, int (*yieldCallback)(void *hostData));
  void (*SetEnv)(void *csound, const char *environmentVariableName, const char *path);
  void (*SetPlayopenCallback)(void *csound,
                              void (*playopen__)(int nchanls, int dsize,
                                                 float sr, int scale));
  void (*SetRtplayCallback)(void *csound, void (*rtplay__)(char *outBuf, int nbytes));
  void (*SetRecopenCallback)(void *csound,
                             void (*recopen__)(int nchanls, int dsize,
                                               float sr, int scale));
  void (*SetRtrecordCallback)(void *csound, int (*rtrecord__)(char *inBuf, int nbytes));
  void (*SetRtcloseCallback)(void *csound, void (*rtclose__)(void));
  /* Internal functions that are needed */
  void (*auxalloc_)(long nbytes, AUXCH *auxchp);
  char *(*getstring_)(int, char*);
  void (*die_)(char *);
  FUNC *(*ftfind_)(MYFLT *);
  int (*initerror_)(char *);
  int (*perferror_)(char *);
  void *(*mmalloc_)(long);
  void *(*mcalloc_)(long);
  void (*mfree_)(void *);
  void (*dispset)(WINDAT *, MYFLT *, long, char *, int, char *);
  void (*display)(WINDAT *);
  MYFLT (*intpow_)(MYFLT, long);
  FUNC *(*ftfindp)(MYFLT *argp);
  FUNC *(*ftnp2find)(MYFLT *);
  char *(*unquote)(char *);
  MEMFIL *(*ldmemfile)(char *);
  
  
  /* End of internals */
  int           ksmps_, nchnls_;
  int           global_ksmps_;
  MYFLT         global_ensmps_, global_ekr_, global_onedkr_;
  MYFLT         global_hfkprd_, global_kicvt_;
  long          global_kcounter_;
  MYFLT         esr_, ekr_;
  char          *orchname_, *scorename_, *xfilename_;
  MYFLT 	e0dbfs_;
  /* oload.h */
  RESETTER      *reset_list_;
  short         nlabels_;
  short         ngotos_;
  int           strsmax_;
  char          **strsets_;
  int           peakchunks_;
  MYFLT         *zkstart_;
  MYFLT         *zastart_;
  long          zklast_;
  long          zalast_;
  long          kcounter_;
  EVTBLK        *currevent_;
  MYFLT         onedkr_;
  MYFLT         onedsr_;
  MYFLT         kicvt_;
  MYFLT         sicvt_;
  MYFLT         *spin_;
  MYFLT         *spout_;
  int           nspin_;
  int           nspout_;
  int           spoutactive_;
  int           keep_tmp_;
  int           dither_output_;
  OENTRY        *opcodlst_;
  void          *opcode_list_;   /* IV - Oct 31 2002 */
  OENTRY        *oplstend_;
  FILE*         dribble_;
  long          holdrand_;
  int           maxinsno_;
  int           maxopcno_;       /* IV - Oct 24 2002 */
  INSDS         *curip_;
  EVTBLK        *Linevtblk_;
  long          nrecs_;
  FILE*         Linepipe_;
  int           Linefd_;
  MYFLT         *ls_table_;
  MYFLT         curr_func_sr_;
  char          *retfilnam_;
  INSTRTXT      **instrtxtp_;
#define ERRSIZ (200)
  char          errmsg_[ERRSIZ];   /* sprintf space for compiling msgs */
  FILE*         scfp_;
  FILE*         oscfp_;
  MYFLT         maxamp_[MAXCHNLS];
  MYFLT         smaxamp_[MAXCHNLS];
  MYFLT         omaxamp_[MAXCHNLS];
  MYFLT         *maxampend_;
  unsigned long maxpos_[MAXCHNLS], smaxpos_[MAXCHNLS], omaxpos_[MAXCHNLS];
  int           tieflag_;
  char          *ssdirpath_, *sfdirpath_;
  char          *tokenstring_;
  POLISH        *polish_;
  FILE*         scorein_;
  FILE*         scoreout_;
  MYFLT         ensmps_, hfkprd_;
  MYFLT         *pool_;
  short         *argoffspace_;
  INSDS         *frstoff_;
  int           sensType_;
  jmp_buf       exitjmp_;
  SRTBLK        *frstbp_;
  int           sectcnt_;
  MCHNBLK       *m_chnbp_[MAXCHAN];
  MYFLT         *cpsocint_, *cpsocfrc_;
  int           inerrcnt_, synterrcnt_, perferrcnt_;
  int           MIDIoutDONE_;
  int           midi_out_;
  char          strmsg_[100];
  INSTRTXT      instxtanchor_;
  INSDS         actanchor_;
  long          rngcnt_[MAXCHNLS];
  short         rngflg_, multichan_;
  EVTNODE       OrcTrigEvts_; /* List of started events, used in playevents() */
  char          name_full_[256];            /* Remember name used */
  int           Mforcdecs_, Mxtroffs_, MTrkend_;
  MYFLT         tran_sr_,tran_kr_,tran_ksmps_;
  MYFLT         tran_0dbfs_;
  int           tran_nchnls_;
  MYFLT         tpidsr_, pidsr_, mpidsr_, mtpdsr_;
  char          *sadirpath_;
  char          *oplibs_;
  OPARMS        *oparms_;
  void          *hostdata_;
  OPCODINFO     *opcodeInfo_;    /* IV - Oct 20 2002 */
  void          *instrumentNames_;
  MYFLT         dbfs_to_short_;
  MYFLT         short_to_dbfs_;
  MYFLT         dbfs_to_float_;
  MYFLT         float_to_dbfs_;
  MYFLT         dbfs_to_long_;
  MYFLT         long_to_dbfs_;
  unsigned int  rtin_dev_;
  unsigned int  rtout_dev_;
  int		MIDIINbufIndex_;
  MIDIMESSAGE   MIDIINbuffer2_[MIDIINBUFMAX];
  int		displop4_;
} ENVIRON;
#define ksmps  cenviron.ksmps_
#define esr    cenviron.esr_
#define ekr    cenviron.ekr_
#define global_ksmps    cenviron.global_ksmps_
#define global_ensmps   cenviron.global_ensmps_
#define global_ekr      cenviron.global_ekr_
#define global_onedkr   cenviron.global_onedkr_
#define global_hfkprd   cenviron.global_hfkprd_
#define global_kicvt    cenviron.global_kicvt_
#define global_kcounter cenviron.global_kcounter_
#define reset_list      cenviron.reset_list_
#define nchnls  cenviron.nchnls_
#define nlabels cenviron.nlabels_
#define ngotos  cenviron.ngotos_
#define strsets cenviron.strsets_
#define strsmax cenviron.strsmax_
#define peakchunks cenviron.peakchunks_
#define zkstart cenviron.zkstart_
#define zastart cenviron.zastart_
#define zklast  cenviron.zklast_
#define zalast  cenviron.zalast_
#define kcounter cenviron.kcounter_
#define currevent cenviron.currevent_
#define onedkr  cenviron.onedkr_
#define onedsr  cenviron.onedsr_
#define kicvt   cenviron.kicvt_
#define sicvt   cenviron.sicvt_
#define spin    cenviron.spin_
#define spout   cenviron.spout_
#define nspin    cenviron.nspin_
#define nspout   cenviron.nspout_
#define spoutactive cenviron.spoutactive_
#define keep_tmp cenviron.keep_tmp_
#define dither_output cenviron.dither_output_
#define opcodlst cenviron.opcodlst_
#define opcode_list cenviron.opcode_list_   /* IV - Oct 31 2002 */
#define oplstend cenviron.oplstend_
#define dribble  cenviron.dribble_
#define holdrand cenviron.holdrand_
#define maxinsno cenviron.maxinsno_
#define maxopcno cenviron.maxopcno_         /* IV - Oct 24 2002 */
#define curip   cenviron.curip_
#define Linevtblk cenviron.Linevtblk_
#define nrecs   cenviron.nrecs_
#ifdef PIPES
#define Linepipe cenviron.Linepipe_
#endif
#define Linefd  cenviron.Linefd_
#define ls_table cenviron.ls_table_
#define curr_func_sr cenviron.curr_func_sr_
#define retfilnam cenviron.retfilnam_
#define orchname cenviron.orchname_
#define scorename cenviron.scorename_
#define xfilename cenviron.xfilename_
#define e0dbfs cenviron.e0dbfs_
#define instrtxtp cenviron.instrtxtp_
#define errmsg  cenviron.errmsg_
#define scfp    cenviron.scfp_
#define oscfp   cenviron.oscfp_
#define maxamp  cenviron.maxamp_
#define smaxamp cenviron.smaxamp_
#define omaxamp cenviron.omaxamp_
#define maxampend cenviron.maxampend_
#define maxpos  cenviron.maxpos_
#define smaxpos cenviron.smaxpos_
#define omaxpos cenviron.omaxpos_
#define tieflag cenviron.tieflag_
#define ssdirpath cenviron.ssdirpath_
#define sfdirpath cenviron.sfdirpath_
#define tokenstring cenviron.tokenstring_
#define polish cenviron.polish_
#define SCOREIN cenviron.scorein_
#define SCOREOUT cenviron.scoreout_
#define ensmps  cenviron.ensmps_
#define hfkprd  cenviron.hfkprd_
#define pool    cenviron.pool_
#define ARGOFFSPACE cenviron.argoffspace_
#define frstoff cenviron.frstoff_
#define sensType cenviron.sensType_
#define frstbp  cenviron.frstbp_
#define sectcnt cenviron.sectcnt_
#define M_CHNBP cenviron.m_chnbp_
#define cpsocint cenviron.cpsocint_
#define cpsocfrc cenviron.cpsocfrc_
#define inerrcnt cenviron.inerrcnt_
#define synterrcnt cenviron.synterrcnt_
#define perferrcnt cenviron.perferrcnt_
#define MIDIoutDONE cenviron.MIDIoutDONE_
#define midi_out cenviron.midi_out_
#define strmsg  cenviron.strmsg_
#define instxtanchor cenviron.instxtanchor_
#define actanchor cenviron.actanchor_
#define rngcnt  cenviron.rngcnt_
#define rngflg  cenviron.rngflg_
#define multichan cenviron.multichan_
#define OrcTrigEvts cenviron.OrcTrigEvts_
#define name_full cenviron.name_full_
#define Mforcdecs cenviron.Mforcdecs_
#define Mxtroffs cenviron.Mxtroffs_
#define MTrkend cenviron.MTrkend_
#define tran_sr cenviron.tran_sr_
#define tran_kr cenviron.tran_kr_
#define tran_ksmps cenviron.tran_ksmps_
#define tran_0dbfs cenviron.tran_0dbfs_
#define tran_nchnls cenviron.tran_nchnls_
#define tpidsr cenviron.tpidsr_
#define pidsr cenviron.pidsr_
#define mpidsr cenviron.mpidsr_
#define mtpdsr cenviron.mtpdsr_
#define sadirpath cenviron.sadirpath_
#define hostdata cenviron.hostdata_
#define oparms cenviron.oparms_
#define opcodeInfo cenviron.opcodeInfo_     /* IV - Oct 20 2002 */
#define instrumentNames cenviron.instrumentNames_
#define dbfs_to_short cenviron.dbfs_to_short_
#define short_to_dbfs cenviron.short_to_dbfs_
#define dbfs_to_float cenviron.dbfs_to_float_
#define float_to_dbfs cenviron.float_to_dbfs_
#define dbfs_to_long cenviron.dbfs_to_long_
#define long_to_dbfs cenviron.long_to_dbfs_
#define rtin_dev cenviron.rtin_dev_
#define rtout_dev cenviron.rtout_dev_
#define MIDIINbufIndex cenviron.MIDIINbufIndex_
#define MIDIINbuffer2 cenviron.MIDIINbuffer2_
#define displop4 cenviron.displop4_
#include "text.h"

#if defined CWIN
#elif defined(mac_classic) || defined(SYMANTEC)
# define POLL_EVENTS() STasks()
# define __cdecl
#endif

#ifdef LINUX
# ifdef HAVE_FLTK
   extern int POLL_EVENTS(void);
# else
#  define POLL_EVENTS()     (1)
# endif
#endif

#if !defined(__BEOS__) || defined(__MWERKS__)
#  define __cdecl
#endif

#include "prototyp.h"
#ifdef FLTK_GUI
#define printf csoundMessage0
extern void csoundMessage0(const char *, ...);
extern void err_printf(char *, ...);
#else
#ifdef CWIN
#include <stdlib.h>
#define printf cwin_printf
extern void err_printf(char *, ...);
#undef putchar
#define putchar cwin_putchar
#define exit(n) cwin_exit(n)
extern void cwin_printf(char *, ...);
extern void cwin_fprintf(FILE *, char *, ...);
extern int  cwin_ensure_screen(void);
extern int cwin_poll_window_manager(void);
extern void cwin_putchar(int);
extern void cwin_exit(int);
#define atexit(x) cwin_atexit(x)
#define POLL_EVENTS() cwin_poll_window_manager()/*cwin_ensure_screen()*/
typedef void ExitFunction(void);
extern int cwin_atexit(ExitFunction*);
#else
#define printf dribble_printf
extern void dribble_printf(char *, ...);
#ifdef mills_macintosh
#define err_printf dribble_printf
#else
extern void err_printf(char *, ...);
#endif
#endif
#endif /* POLL_EVENTS */

#ifdef WIN32
#define tmpnam mytmpnam
char *mytmpnam(char *);
#endif

#ifndef PI
#define PI      (3.14159265358979323846)
#endif
#define TWOPI   (6.28318530717958647692)
#define PI_F    (FL(3.14159265358979323846))
#define TWOPI_F (FL(6.28318530717958647692))


#define WARNMSG 04

/*
* Move the C++ guards to enclose the entire file,
* in order to enable C++ to #include this file.
*/
#ifdef __cplusplus
};
#endif

#endif /* CS_H */
