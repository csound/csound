/*
  csoundCore.h: csound engine structures and module API

  Copyright (C) 1991-2024 Barry Vercoe, John ffitch, Istvan Varga,              
                           V Lazzarini, S Yi

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

#if !defined(__BUILDING_LIBCSOUND) && !defined(CSOUND_CSDL_H)
#  error "Csound plugins and host applications should not include csoundCore.h"
#endif

#ifndef CSOUNDCORE_H
#define CSOUNDCORE_H

#if defined(__EMSCRIPTEN__) && !defined(EMSCRIPTEN)
#define EMSCRIPTEN
#endif

#include "sysdep.h"
#if !defined(EMSCRIPTEN) && !defined(CABBAGE)
#if defined(HAVE_PTHREAD)
#include <pthread.h>
#endif
#endif
#include "cs_par_structs.h"
#include <stdarg.h>
#include <setjmp.h>
#include "csound_type_system.h"
#include "csound.h"
#include "csound_files.h"
#include "csound_graph_display.h"
#include "csound_circular_buffer.h"
#include "csound_threads.h"
#include "csound_compiler.h"
#include "csound_misc.h"
#include "csound_server.h"
#include "cscore.h"
#include "csound_data_structures.h"
#include "pools.h"
#include "soundfile.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifndef CSOUND_CSDL_H
/* VL not sure if we need to check for SSE */
#if defined(__SSE__) && !defined(EMSCRIPTEN)
#include <xmmintrin.h>
#ifndef _MM_DENORMALS_ZERO_ON
#define _MM_DENORMALS_ZERO_MASK   0x0040
#define _MM_DENORMALS_ZERO_ON     0x0040
#define _MM_DENORMALS_ZERO_OFF    0x0000
#define _MM_SET_DENORMALS_ZERO_MODE(mode)                               \
  _mm_setcsr((_mm_getcsr() & ~_MM_DENORMALS_ZERO_MASK) | (mode))
#define _MM_GET_DENORMALS_ZERO_MODE()           \
  (_mm_getcsr() & _MM_DENORMALS_ZERO_MASK)
#endif
#else
#ifndef _MM_DENORMALS_ZERO_ON
#define _MM_DENORMALS_ZERO_MASK   0
#define _MM_DENORMALS_ZERO_ON     0
#define _MM_DENORMALS_ZERO_OFF    0
#define _MM_SET_DENORMALS_ZERO_MODE(mode)
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus */


  /* VL: these were moved here from csound.h
     as they are not relevant to the new host API
  */
  typedef struct xyindat_ XYINDAT;

  /**
   * Real-time audio parameters structure
   */
  typedef struct {
    /** device name (NULL/empty: default) */
    char    *devName;
    /** device number (0-1023), 1024: default */
    int32_t     devNum;
    /** buffer fragment size (-b) in sample frames */
    uint32_t     bufSamp_SW;
    /** total buffer size (-B) in sample frames */
    int32_t     bufSamp_HW;
    /** number of channels */
    int32_t     nChannels;
    /** sample format (AE_SHORT etc.) */
    int32_t     sampleFormat;
    /** sample rate in Hz */
    float   sampleRate;
    /** ksmps */
    int32_t ksmps;
  } csRtAudioParams;

  uint32_t csoundGetNchnls(CSOUND *);
  uint32_t csoundGetNchnlsInput(CSOUND *csound);
  long csoundGetInputBufferSize(CSOUND *);
  long csoundGetOutputBufferSize(CSOUND *);
  void *csoundGetNamedGens(CSOUND *);
  int32_t *csoundGetChannelLock(CSOUND *csound, const char *name);

  void **csoundGetRtRecordUserData(CSOUND *);
  void **csoundGetRtPlayUserData(CSOUND *);

  void
  csoundSetPlayopenCallback(CSOUND *,
                            int32_t (*playopen__)(CSOUND *,
                                              const csRtAudioParams *parm));
  void csoundSetRtplayCallback(CSOUND *,
                               void (*rtplay__)(CSOUND *,
                                                const MYFLT *outBuf,
                                                int32_t nbytes));
  void csoundSetRecopenCallback(CSOUND *,
                                int32_t (*recopen_)(CSOUND *,
                                                const csRtAudioParams *parm));
  void csoundSetRtrecordCallback(CSOUND *,
                                 int32_t (*rtrecord__)(CSOUND *,
                                                   MYFLT *inBuf,
                                                   int32_t nbytes));
  void csoundSetRtcloseCallback(CSOUND *, void (*rtclose__)(CSOUND *));
  void csoundSetAudioDeviceListCallback(CSOUND *csound,
                                        int32_t (*audiodevlist__)(CSOUND *,
                                                              CS_AUDIODEVICE *list,
                                                              int32_t isOutput));

  int32_t csoundCompileCsd(CSOUND *csound, const char *csd_filename);
  int32_t csoundCompileCsdText(CSOUND *csound, const char *csd_text);
  int32_t csoundCleanup(CSOUND *);

  void csoundInputMessage(CSOUND *csound, const char * sc);
  int32_t csoundScoreEvent(CSOUND *, char type, const MYFLT *pFields,
                        long numFields);

#if defined(__MACH__) || defined(__FreeBSD__) || defined(__DragonFly__)
#include <xlocale.h>
#endif

#if (defined(__MACH__) || defined(ANDROID) || defined(NACL) \
  || defined(__CYGWIN__) || defined(__HAIKU__))
#include <pthread.h>
#define BARRIER_SERIAL_THREAD (-1)
  typedef struct {
    pthread_mutex_t mut;
    pthread_cond_t cond;
    uint32_t count, max, iteration;
  } barrier_t;

#ifndef PTHREAD_BARRIER_SERIAL_THREAD
#define pthread_barrier_t barrier_t
#endif /* PTHREAD_BARRIER_SERIAL_THREAd */
#endif /* __MACH__ */

#define OK        (0)
#define NOTOK     (-1)

#define DEFAULT_STRING_SIZE 64

#define CSFILE_FD_R     1
#define CSFILE_FD_W     2
#define CSFILE_STD      3
#define CSFILE_SND_R    4
#define CSFILE_SND_W    5

  /* check for power of two ftable sizes */
#define IS_POW_TWO(N) ((N != 0) ? !(N & (N - 1)) : 0)

#define MAXINSNO  (200)
#define PMAX      (1998)
#define VARGMAX   (1999)
#define NOT_AN_INSTRUMENT INT32_MAX

#define ORTXT       h.optext->t
#define INCOUNT     ORTXT.inlist->count
#define OUTCOUNT    ORTXT.outlist->count   /* Not used */
#define INOCOUNT    ORTXT.inArgCount
#define OUTOCOUNT   ORTXT.outArgCount
#define IS_ASIG_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "a"))
#define IS_STR_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "S"))
#define IS_KSIG_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "k"))
#define IS_INIT_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "i"))
#define IS_FSIG_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "f"))
#define IS_ARRAY_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "["))

#define CURTIME (((double)csound->icurTime)/((double)csound->esr))
#define CURTIME_inc (((double)csound->ksmps)/((double)csound->esr))

#ifndef  SHORT_TABLE_LENGTH  // long max table length is the default
static const uint32_t MAXLEN = 1U << 30;
static const double FMAXLEN = (double) (1U << 30);
static const uint32_t PHMASK = (1U << 30) - 1U;
#else   // this is the original max table length
static const uint32_t MAXLEN =  1U << 24;
static const double FMAXLEN = (double) (1U << 24);
static const double FMAXLEN = (1U << 24) - 1;
#endif


#define MAX_STRING_CHANNEL_DATASIZE 16384

#define PFRAC(x)   ((MYFLT)((x) & ftp->lomask) * ftp->lodiv)
#define MAXPOS     0x7FFFFFFFL

#define BYTREVS(n) ((n>>8  & 0xFF) | (n<<8 & 0xFF00))
#define BYTREVL(n) ((n>>24 & 0xFF) | (n>>8 & 0xFF00L) |         \
                    (n<<8 & 0xFF0000L) | (n<<24 & 0xFF000000L))

#define OCTRES     8192
#define CPSOCTL(n) ((MYFLT)(1<<((int32_t)(n)>>13))*csound->cpsocfrc[(int32_t)(n)&8191])

#ifdef USE_DOUBLE
  extern int64_t MYNAN;
#define SSTRCOD    (double)NAN
#else
  extern int32 MYNAN;
#define SSTRCOD    (float)NAN
#endif
  extern int32_t ISSTRCOD(MYFLT);

#define SSTRSIZ    1024
#define ALLCHNLS   0x7fff
#define DFLT_SR    FL(44100.0)
#define DFLT_KR    FL(4410.0)
#define DFLT_KSMPS 10
#define DFLT_NCHNLS 1
#define MAXCHNLS   256

#define MAXNAME   (256)

#define DFLT_DBFS (FL(32768.0))

#define MAXOCTS         8
#define MAXCHAN         16      /* 16 MIDI channels; only one port for now */

  /* A440 tuning factor */
#define ONEPT           (csound->GetA4(csound)/430.5389646099018460319362438314060262605)
#define LOG10D20        0.11512925              /* for db to ampfac   */
#define DV32768         FL(0.000030517578125)

#ifndef PI
#define PI      (3.141592653589793238462643383279502884197)
#endif /* pi */
#define TWOPI   (6.283185307179586476925286766559005768394)
#define HALFPI  (1.570796326794896619231321691639751442099)
#define PI_F    ((MYFLT) PI)
#define TWOPI_F ((MYFLT) TWOPI)
#define HALFPI_F ((MYFLT) HALFPI)
#define INF     (2147483647.0)
#define ROOT2   (1.414213562373095048801688724209698078569)

  /* CONSTANTS FOR USE IN MSGLEVEL */
#define CS_AMPLMSG 01
#define CS_RNGEMSG 02
#define CS_WARNMSG 04
#define CS_NOMSG   0x10
#define CS_RAWMSG  0x40
#define CS_TIMEMSG 0x80
#define CS_NOQQ    0x400

#define IGN(X)  (void) X

#define ARG_CONSTANT 0
#define ARG_STRING 1
#define ARG_PFIELD 2
#define ARG_GLOBAL 3
#define ARG_LOCAL 4
#define ARG_LABEL 5

#define ASYNC_GLOBAL 1
#define ASYNC_LOCAL  2

  enum {FFT_LIB=0, PFFT_LIB, VDSP_LIB};
  enum {FFT_FWD=0, FFT_INV};

  /* advance declaration for
     API  message queue struct
  */
  struct _message_queue;

  typedef struct CSFILE_ {
    struct CSFILE_  *nxt;
    struct CSFILE_  *prv;
    int32_t             type;
    int32_t             fd;
    FILE            *f;
    SNDFILE         *sf;
    void            *cb;
    int32_t             async_flag;
    int32_t             items;
    int32_t             pos;
    MYFLT           *buf;
    int32_t             bufsize;
    char            fullName[1];
  } CSFILE;


  typedef struct CORFIL {
    char    *body;
    uint32_t     len;
    uint32_t     p;
  } CORFIL;

  typedef struct arglst {
    int32_t count;
    char    *arg[1];
  } ARGLST;

  typedef struct arg {
    int32_t type;
    void* argPtr;
    int32_t index;
    char* structPath;
    struct arg* next;
  } ARG;

  typedef struct oentry {
    char    *opname;
    uint16  dsblksiz;
    uint16  flags;
    char    *outypes;
    char    *intypes;
    int32_t  (*init)(CSOUND *, void *p);
    int32_t  (*perf)(CSOUND *, void *p);
    int32_t  (*deinit)(CSOUND *, void *p);
    void    *useropinfo; /* user opcode parameters */
  } OENTRY;

  /**
   * Storage for parsed orchestra code, for each opcode in an INSTRTXT.
   */
  typedef struct text {
    uint16_t        linenum;        /* Line num in orch file (currently buggy!)  */
    uint64_t        locn;           /* and location */
    OENTRY          *oentry;
    char            *opcod;         /* Pointer to opcode name in global pool */
    ARGLST          *inlist;        /* Input args (pointer to item in name list) */
    ARGLST          *outlist;
    ARG             *inArgs;        /* Input args (index into list of values) */
    uint32_t        inArgCount;
    ARG             *outArgs;
    uint32_t       outArgCount;
    //    char            intype;         /* Type of first input argument (g,k,a,w etc) */
    char            pftype;         /* Type of output argument (k,a etc) */
  } TEXT;


  /**
   * This struct is filled out by otran() at orch parse time.
   * It is used as a template for instrument events.
   */
  typedef struct instr {
    struct op * nxtop;              /* Linked list of instr opcodes */
    TEXT    t;                      /* Text of instrument (same in nxtop) */
    int32_t     pmax, vmax, pextrab;    /* Arg count, size of data for all
                                       opcodes in instr */
    //int     mdepends;               /* Opcode type (i/k/a) */
    CS_VAR_POOL* varPool;

    //    int32_t     optxtcount;
    int16   muted;
    //    int32   localen;
    int32   opdstot;                /* Total size of opds structs in instr */
    //    int32   *inslist;               /* Only used in parsing (?) */
    MYFLT   *psetdata;              /* Used for pset opcode */
    struct insds * instance;        /* Chain of allocated instances of
                                       this instrument */
    struct insds * lst_instance;    /* last allocated instance */
    struct insds * act_instance;    /* Chain of free (inactive) instances */
                                    /* (pointer to next one is INSDS.nxtact) */
    struct instr * nxtinstxt;       /* Next instrument in orch (num order) */
    int32_t     active;                 /* To count activations for control */
    int32_t     pending_release;        /* To count instruments in release phase */
    int32_t     maxalloc;
    int32_t     turnoff_mode;       /* Optionally turnoff instruments instances above maxalloc*/
    MYFLT   cpuload;                /* % load this instrumemnt makes */
    struct opcodinfo *opcode_info;  /* UDO info (when instrs are UDOs) */
    char    *insname;               /* instrument name */
    int32_t     instcnt;                /* Count number of instances ever */
    int32_t     isNew;                  /* is this a new definition */
    int32_t     nocheckpcnt;            /* Control checks on pcnt */
  } INSTRTXT;

  typedef struct namedInstr {
    int32        instno;
    char        *name;
    INSTRTXT    *ip;
    struct namedInstr   *next;
  } INSTRNAME;

  /**
   * A chain of TEXT structs. Note that this is identical with the first two
   * members of struct INSTRTEXT, and is so typecast at various points in code.
   */
  typedef struct op {
    struct op *nxtop;
    TEXT    t;
  } OPTXT;

  typedef struct fdch {
    struct fdch *nxtchp;
    /** handle returned by csound->FileOpen() */
    void    *fd;
  } FDCH;

  typedef struct auxch {
    struct auxch *nxtchp;
    size_t  size;
    void    *auxp, *endp;
  } AUXCH;

  /**  this callback is used to notify the
       availability of new storage in AUXCH *.
       It can be used to swap the old storage
       for the new one and return it for deallocation.
  */
  typedef AUXCH* (*aux_cb)(CSOUND *, void *, AUXCH *);

  /**
   * AuxAllocAsync data
   */
  typedef struct {
    CSOUND *csound;
    size_t nbytes;
    AUXCH *auxchp;
    void *userData;
    aux_cb notify;
  } AUXASYNC;

  typedef struct {
    int32_t     size;             /* 0...size-1 */
    MYFLT   *data;
    AUXCH   aux;
  } TABDAT;

  /*
   * Type definition for array data
   */
  struct arraydat {
    int32_t      dimensions; /* number of array dimensions */
    int32_t*     sizes;  /* size of each dimensions */
    int32_t      arrayMemberSize; /* size of each item */
    const struct cstype* arrayType; /* type of array */
    MYFLT*   data; /* data */
    size_t   allocated; /* size of allocated data */
  };

#define MAX_STRINGDAT_SIZE 0xFFFFFFFF
  /*
   * Type definition for string data 
   */
  struct stringdat {
    char *data;         // null-terminated string
    size_t size;        // total allocated size
    int64_t timestamp;  // used internally for updates
  };

  /*
   * Type definition for complex data
   */
  typedef struct complexdat {
    MYFLT real;
    MYFLT imag;
    int32_t isPolar;
  } COMPLEXDAT;
  
  typedef struct monblk {
    int16   pch;
    struct monblk *prv;
  } MONPCH;

  typedef struct {
    int32_t     notnum[4];
  } DPEXCL;

  typedef struct {
    DPEXCL  dpexcl[8];
    /** for keys 25-99 */
    int32_t     exclset[75];
  } DPARM;

  typedef struct dklst {
    struct dklst *nxtlst;
    int32    pgmno;
    /** cnt + keynos */
    MYFLT   keylst[1];
  } DKLST;

  typedef struct mchnblk {
    /** most recently received program change */
    int16   pgmno;
    /** instrument number assigned to this channel */
    int16   insno;
    int16   RegParNo;
    int16   mono;
    /** channel number */
    int16   channel;
    MONPCH  *monobas;
    MONPCH  *monocur;
    /** list of active notes (NULL: not active) */
    struct insds *kinsptr[128];
    /** polyphonic pressure indexed by note number */
    MYFLT   polyaft[128];
    /** ... with GS vib_rate, stored in c128-c135 */
    MYFLT   ctl_val[136];
    /** program change to instr number (<=0: ignore) */
    int16   pgm2ins[128];
    /** channel pressure (0-127) */
    MYFLT   aftouch;
    /** pitch bend (-1 to 1) */
    MYFLT   pchbend;
    /** pitch bend sensitivity in semitones */
    MYFLT   pbensens;
    /** number of held (sustaining) notes */
    int16   ksuscnt;
    /** current state of sustain pedal (0: off) */
    int16   sustaining;
    int32_t     dpmsb;
    int32_t     dplsb;
    int32_t     datenabl;
    /** chain of dpgm keylists */
    DKLST   *klists;
    /** drumset params         */
    DPARM   *dparms;
  } MCHNBLK;

  /**
   * This struct holds the data for one score event.
   */
  typedef struct event {
    /** String argument(s) (NULL if none) */
    int32_t     scnt;
    char    *strarg;
    /* instance pointer */
    void  *pinstance;  /* used in nstance opcode */
    /** Event type */
    char    opcod;
    /** Number of p-fields */
    int16   pcnt;
    /** Event start time */
    MYFLT   p2orig;
    /** Length */
    MYFLT   p3orig;
    /** All p-fields for this event (SSTRCOD: string argument) */
    MYFLT   p[PMAX + 1];
    union {                   /* To ensure size is same as earlier */
      MYFLT   *extra;
      MYFLT   p[2];
    } c;
  } EVTBLK;
  /**
   * This struct holds the info for a concrete instrument event
   * instance in performance.
   */
  typedef struct insds {
    /* Chain of init-time opcodes */
    struct opds * nxti;
    /* Chain of performance-time opcodes */
    struct opds * nxtp;
    /* Chain of deinit opcodes */
    struct opds * nxtd;
    /* Next allocated instance */
    struct insds * nxtinstance;
    /* Previous allocated instance */
    struct insds * prvinstance;
    /* Next in list of active instruments */
    struct insds * nxtact;
    /* Previous in list of active instruments */
    struct insds * prvact;
    /* Next instrument to terminate */
    struct insds * nxtoff;
    /* Chain of files used by opcodes in this instr */
    FDCH    *fdchp;
    /* Extra memory used by opcodes in this instr */
    AUXCH   *auxchp;
    /* Extra release time requested with xtratim opcode */
    int32_t      xtratim;
    /* MIDI note info block if event started from MIDI */
    MCHNBLK *m_chnbp;
    /* ptr to next overlapping MIDI voice */
    struct insds * nxtolap;
    /* Instrument number */
    int16   insno;
    /* Instrument def address */
    INSTRTXT *instr;
    /* non-zero for sustaining MIDI note */
    int16    m_sust;
    /* MIDI pitch, for simple access */
    unsigned char m_pitch;
    /* ...ditto velocity */
    unsigned char m_veloc;
    /* Flag to indicate we are releasing, test with release opcode */
    char     relesing;
    /* Set if instr instance is active (perfing) */
    char     actflg;
    /* Time to turn off event, in score beats */
    double   offbet;
    /* Time to turn off event, in seconds (negative on indef/tie) */
    double   offtim;
    /* pointer to Csound engine and API for externals */
    CSOUND  *csound;
    uint64_t kcounter;
    MYFLT    esr, sicvt, pidsr;                  /* local sr */
    MYFLT    onedsr;
    int32_t     in_cvt, out_cvt; /* resampling converter modes for in and out */
    uint32_t ksmps;     /* Instrument copy of ksmps */
    MYFLT    ekr;                /* and of rates */

    MYFLT    onedksmps, onedkr, kicvt;
    struct opds  *pds;          /* Used for jumping */
    MYFLT    scratchpad[4];      /* Persistent data */

    /* user defined opcode I/O buffers */
    void    *opcod_iobufs;
    void    *opcod_deact, *subins_deact;
    uint32_t ksmps_offset; /* ksmps offset for sample accuracy */
    uint32_t no_end;      /* samps left at the end for sample accuracy
                             (calculated) */
    uint32_t ksmps_no_end; /* samps left at the end for sample accuracy
                              (used by opcodes) */
    MYFLT   *spin;         /* offset into csound->spin */
    MYFLT   *spout;        /* offset into csound->spout, or local spout */
    int32_t      init_done;
    int32_t      tieflag;
    int32_t      reinitflag;
    MYFLT    retval;
    MYFLT   *lclbas;  /* base for variable memory pool */
    char    *strarg;       /* string argument */
    /* Copy of required p-field values for quick access */
    CS_VAR_MEM  p0;
    CS_VAR_MEM  p1;
    CS_VAR_MEM  p2;
    CS_VAR_MEM  p3;
  } INSDS;

#define CS_KSMPS     (p->h.insdshead->ksmps)
#define CS_KCNT      (p->h.insdshead->kcounter)
#define CS_EKR       (p->h.insdshead->ekr)
#define CS_ONEDKSMPS (p->h.insdshead->onedksmps)
#define CS_ONEDKR    (p->h.insdshead->onedkr)
#define CS_KICVT     (p->h.insdshead->kicvt)
#define CS_ONEDDBFS  (FL(1.0/csound->Get0dBFS(csound)))
#define CS_ESR       (p->h.insdshead->esr)
#define CS_ONEDSR    (p->h.insdshead->onedsr)
#define CS_SICVT     (p->h.insdshead->sicvt)
#define CS_TPIDSR    (2.*p->h.insdshead->pidsr)
#define CS_PIDSR     (p->h.insdshead->pidsr)
#define CS_MPIDSR    (-p->h.insdshead->pidsr)
#define CS_MTPIDSR   (-2.*p->h.insdshead->pidsr)
#define CS_PDS       (p->h.insdshead->pds)
#define CS_SPIN      (p->h.insdshead->spin)
#define CS_SPOUT     (p->h.insdshead->spout)

/* Phase modulo-1 for oscillators */
static inline double PHMOD1(double p) {
    return p < 0 ? -(1. - FLOOR(p)) : p - (uint64_t) p;
}

  typedef int32_t (*SUBR)(CSOUND *, void *);

  /**
   * This struct holds the info for one opcode in a concrete
   * instrument instance in performance.
   */
  typedef struct opds {
    /** Next opcode in init-time chain */
    struct opds * nxti;
    /** Next opcode in perf-time chain */
    struct opds * nxtp;
    /** Next opcode in deinit chain */
    struct opds * nxtd;
    /** Initialization (i-time) function pointer */
    SUBR    init;
    /** Perf-time (k- or a-rate) function pointer */
    SUBR    perf;
    /** deinit function pointer */
    SUBR    deinit;
    /** Orch file template part for this opcode */
    OPTXT   *optext;
    /** Owner instrument instance data structure */
    INSDS   *insdshead;
  } OPDS;

  typedef struct {
    char        *opname;
    char        *outypes;
    char        *intypes;
    int32_t         flags;
  } opcodeListEntry;


  typedef struct lblblk {
    OPDS    h;
    OPDS    *prvi;
    OPDS    *prvp;
    OPDS    *prvd;
  } LBLBLK;

  typedef struct {
    MYFLT   *begp, *curp, *endp, feedback[6];
    int32    scount;
  } OCTDAT;

  typedef struct {
    int32    npts, nocts, nsamps;
    MYFLT   lofrq, hifrq, looct, srate;
    OCTDAT  octdata[MAXOCTS];
    AUXCH   auxch;
  } DOWNDAT;

  typedef struct {
    uint32_t   ktimstamp, ktimprd;
    int32    npts, nfreqs, dbout;
    DOWNDAT *downsrcp;
    AUXCH   auxch;
  } SPECDAT;

  typedef struct {
    MYFLT   gen01;
    MYFLT   ifilno;
    MYFLT   iskptim;
    MYFLT   iformat;
    MYFLT   channel;
    MYFLT   sample_rate;
    char    strarg[SSTRSIZ];
  } GEN01ARGS;

  typedef struct {
    /** table length, not including the guard point */
    uint32_t flen;
    /** length mask ( = flen - 1) for power of two table size, 0 otherwise */
    int32    lenmask;
    /** log2(MAXLEN / flen) for power of two table size, 0 otherwise */
    int32    lobits;
    /** 2^lobits - 1 */
    int32    lomask;
    /** 1 / 2^lobits */
    MYFLT   lodiv;
    /** LOFACT * (table_sr / orch_sr), cpscvt = cvtbas / base_freq */
    MYFLT   cvtbas, cpscvt;
    /** sustain loop mode (0: none, 1: forward, 2: forward and backward) */
    int16   loopmode1;
    /** release loop mode (0: none, 1: forward, 2: forward and backward) */
    int16   loopmode2;
    /** sustain loop start and end in sample frames */
    int32    begin1, end1;
    /** release loop start and end in sample frames */
    int32    begin2, end2;
    /** sound file length in sample frames (flenfrms = soundend - 1) */
    int32    soundend, flenfrms;
    /** number of channels */
    int32    nchanls;
    /** table number */
    int32    fno;
    /** sampling rate */
    MYFLT   sr;
    /** args  */
    MYFLT args[PMAX - 4];
    /** arg count */
    int32_t argcnt;
    /** GEN01 parameters */
    GEN01ARGS gen01args;
    /** table data (flen + 1 MYFLT values) */
    MYFLT   *ftable;
  } FUNC;

  typedef struct {
    CSOUND  *csound;
    int32   flen;
    int32_t     fno, guardreq;
    EVTBLK  e;
  } FGDATA;

  typedef struct {
    char    *name;
    int32_t (*fn)(FGDATA *, FUNC *);
  } NGFENS;

  typedef int32_t (*GEN)(FGDATA *, FUNC *);

  typedef struct MEMFIL {
    char    filename[256];      /* Made larger RWD */
    char    *beginp;
    char    *endp;
    int32    length;
    struct MEMFIL *next;
  } MEMFIL;

  typedef struct {
    int16   type;
    int16   chan;
    int16   dat1;
    int16   dat2;
  } MEVENT;

  typedef struct SNDMEMFILE_ {
    /** file ID (short name)          */
    char            *name;
    struct SNDMEMFILE_ *nxt;
    /** full path filename            */
    char            *fullName;
    /** file length in sample frames  */
    size_t          nFrames;
    /** sample rate in Hz             */
    double          sampleRate;
    /** number of channels            */
    int32_t             nChannels;
    /** AE_SHORT, AE_FLOAT, etc.      */
    int32_t             sampleFormat;
    /** TYP_WAV, TYP_AIFF, etc.       */
    int32_t             fileType;
    /**
     * loop mode:
     *   0: no loop information
     *   1: off
     *   2: forward
     *   3: backward
     *   4: bidirectional
     */
    int32_t             loopMode;
    /** playback start offset frames  */
    double          startOffs;
    /** loop start (sample frames)    */
    double          loopStart;
    /** loop end (sample frames)      */
    double          loopEnd;
    /** base frequency (in Hz)        */
    double          baseFreq;
    /** amplitude scale factor        */
    double          scaleFac;
    /** interleaved sample data       */
    MYFLT           data[1];
  } SNDMEMFILE;

  typedef struct pvx_memfile_ {
    char        *filename;
    struct pvx_memfile_ *nxt;
    float       *data;
    uint32 nframes;
    int32_t         format;
    int32_t         fftsize;
    int32_t         overlap;
    int32_t         winsize;
    int32_t         wintype;
    int32_t         chans;
    MYFLT       srate;
  } PVOCEX_MEMFILE;

/* The definitions and declarations in this section 
   are not available externally to plugins.
*/
#ifdef __BUILDING_LIBCSOUND

/* max number of input/output args for user defined opcodes */
#define OPCODENUMOUTS_LOW   16
#define OPCODENUMOUTS_HIGH  64
#define OPCODENUMOUTS_MAX   256

#define MBUFSIZ         (4096)
#define MIDIINBUFMAX    (1024)
#define MIDIINBUFMSK    (MIDIINBUFMAX-1)
#define MIDIMAXPORTS    (64)

  typedef union {
    uint32 dwData;
    unsigned char bData[4];
  } MIDIMESSAGE;

  /* MIDI globals */

  typedef struct midiglobals {
    MEVENT  *Midevtblk;
    int32_t     sexp;
    int32_t     MIDIoutDONE;
    int32_t     MIDIINbufIndex;
    MIDIMESSAGE MIDIINbuffer2[MIDIINBUFMAX];
    int32_t     (*MidiInOpenCallback)(CSOUND *, void **, const char *);
    int32_t     (*MidiReadCallback)(CSOUND *, void *, unsigned char *, int32_t);
    int32_t     (*MidiInCloseCallback)(CSOUND *, void *);
    int32_t     (*MidiOutOpenCallback)(CSOUND *, void **, const char *);
    int32_t     (*MidiWriteCallback)(CSOUND *, void *, const unsigned char *, int32_t);
    int32_t     (*MidiOutCloseCallback)(CSOUND *, void *);
    const char *(*MidiErrorStringCallback)(int32_t);
    void    *midiInUserData;
    void    *midiOutUserData;
    void    *midiFileData;
    void    *midiOutFileData;
    int32_t     rawControllerMode;
    char    muteTrackList[256];
    unsigned char mbuf[MBUFSIZ];
    unsigned char *bufp, *endatp;
    int16   datreq, datcnt;
  } MGLOBAL;

  typedef struct osc_mess {
    char *address;
    char *type;
    char *data;
    int32_t size;
    int32_t flag;
    struct osc_mess *nxt;
  } OSC_MESS;

  typedef struct eventnode {
    struct eventnode  *nxt;
    uint32     start_kcnt;
    EVTBLK            evt;
  } EVTNODE;

  typedef struct {
    OPDS    h;
    MYFLT   *ktempo, *istartempo;
    MYFLT   prvtempo;
  } TEMPO;

  typedef struct names {
    char    *mac;
    struct names *next;
  } NAMES;

  typedef struct threadInfo {
    struct threadInfo *next;
    void * threadId;
  } THREADINFO;

#include "sort.h"
#include "text.h"
#include "prototyp.h"
#include "cwindow.h"
#include "envvar.h"
#include "remote.h"

#define CS_STATE_PRE    (1)
#define CS_STATE_COMP   (2)
#define CS_STATE_UTIL   (4)
#define CS_STATE_CLN    (8)
#define CS_STATE_JMP    (16)

  /* These are used to set/clear bits in csound->tempStatus.
     If the bit is set, it indicates that the given file is
     a temporary. */
  extern const uint32_t csOrcMask;
  extern const uint32_t csScoInMask;
  extern const uint32_t csScoSortMask;
  extern const uint32_t csMidiScoMask;
  extern const uint32_t csPlayScoMask;

  /* kperf function protoypes. Used by the debugger to switch between debug
   * and nodebug kperf functions */
  int32_t kperf_nodebug(CSOUND *csound);
  int32_t kperf_debug(CSOUND *csound);

#endif  /* __BUILDING_LIBCSOUND */

#define MARGS   (3)
#define MAX_INCLUDE_DEPTH 100
  struct MACRO;

  typedef struct MACRON {
    int32_t             n;
    uint32_t    line;
    struct MACRO    *s;
    char            *path;
    int32_t             included;
  } MACRON;

  typedef struct MACRO {          /* To store active macros */
    char          *name;        /* Use is by name */
    int32_t           acnt;         /* Count of arguments */
    char          *body;        /* The text of the macro */
    struct MACRO  *next;        /* Chain of active macros */
    int32_t           margs;        /* amount of space for args */
    char          *arg[MARGS];  /* With these arguments */
  } MACRO;

  typedef struct in_stack_s {     /* Stack of active inputs */
    int16       is_marked_repeat;     /* 1 if this input created by 'n' stmnt */
    int16       args;                 /* Argument count for macro */
    //CORFIL      *cf;                  /* In core file */
    //void        *fd;                  /* for closing stream */
    MACRO       *mac;
    int32_t         line;
    int32       oposit;
  } IN_STACK;

  typedef struct marked_sections {
    char        *name;
    int32       posit;
    int32_t         line;
  } MARKED_SECTIONS;

  typedef struct namelst {
    char           *name;
    struct namelst *next;
  } NAMELST;

  typedef struct NAME__ {
    char          *namep;
    struct NAME__  *nxt;
    int32_t           type, count;
  } NAME;

  /* Holds UDO information, when an instrument is
     defined as a UDO
  */
  typedef struct opcodinfo {
    int32    instno;
    char    *name, *intypes, *outtypes;
    int16   inchns, outchns;
    bool newStyle;
    bool passByRef;
    CS_VAR_POOL* out_arg_pool;
    CS_VAR_POOL* in_arg_pool;
    INSTRTXT *ip;
    struct opcodinfo *prv;
  } OPCODINFO;

  /**
   * This struct will hold the current engine state after compilation
   */
  typedef struct engine_state {
    CS_VAR_POOL    *varPool;  /* global variable pool */
    CS_HASH_TABLE  *constantsPool;
    CS_HASH_TABLE  *stringPool;
    int32_t            maxopcno;
    INSTRTXT      **instrtxtp; /* instrument list      */
    INSTRTXT      instxtanchor;
    CS_HASH_TABLE *instrumentNames; /* instrument names */
    int32_t           maxinsno;
  } ENGINE_STATE;


  /**
   * Nen FFT interface
   */
  typedef struct _FFT_SETUP{
    int32_t N, M;
    void  *setup;
    MYFLT *buffer;
    int32_t    lib;
    int32_t    d;
    int32_t  p2;
  } CSOUND_FFT_SETUP;


  /**
   * plugin module info
   */
  typedef struct {
    char module[12];
    char type[12];
  } MODULE_INFO;


#define MAX_ALLOC_QUEUE 1024

  typedef struct _alloc_data_ {
    int32_t type;
    int32_t insno;
    EVTBLK blk;
    MCHNBLK *chn;
    MEVENT mep;
    INSDS *ip;
    OPDS *ids;
  } ALLOC_DATA;

#define MAX_MESSAGE_STR 1024
  typedef struct _message_queue_t_ {
    int32_t attr;
    char str[MAX_MESSAGE_STR];
  } message_string_queue_t;

  /* Binary positive power function */
  static inline double intpow1(double x, int32_t n)
  {
    double ans = 1.;
    while (n!=0) {
      if (n&1) ans = ans * x;
      n >>= 1;
      x = x*x;
    }
    return ans;
  }

  /* Binary power function */
  static inline double intpow(MYFLT x, int32_t n)
  {
    if (n<0) {
      n = -n;
      x = 1./x;
    }
    return intpow1(x, n);
  }

  static inline int32_t byte_order(void){
    const int32_t one = 1;
    return (!*((char*) &one));
  }

  static inline int32_t isstrcod(MYFLT xx){
    int32_t sel = (byte_order()+1)&1;
#ifdef USE_DOUBLE
    union {
      double d;
      int32_t i[2];
    } z;
    z.d = xx;
    return ((z.i[sel]&0x7ff00000)==0x7ff00000);
#else
    union {
      float f;
      int32_t j;
    } z;
    z.f = xx;
    return ((z.j&0x7f800000) == 0x7f800000);
#endif
  }

  /** @name Opcode attributes */
  /**@{ */
  /**
   * Returns true if argument is a string code
   */
  static inline int32_t IsStringCode(MYFLT f){
    return isstrcod(f);
  }

  /**
   * Returns the number of input arguments for opcode 'p'.
   */
  static inline int32_t GetInputArgCnt(OPDS *p){
    return (int32_t) p->optext->t.inArgCount;
  }

  /**
   * Returns the name of input argument 'n' (counting from 0) for opcode 'p'.
   */
  static inline char *GetInputArgName(OPDS *p, uint32_t n){
    if ( n >=
         (uint32_t) p->optext->t.inArgCount)
      return (char*) NULL;
    return (char*) p->optext->t.inlist->arg[n];
  }

  /**
   * Returns the number of output arguments for opcode 'p'.
   */
  static inline int32_t GetOutputArgCnt(OPDS *p){
    return (int32_t) p->optext->t.outArgCount;
  }

  /**
   * Returns the name of output argument 'n' (counting from 0) for opcode 'p'.
   */
  static inline char *GetOutputArgName(OPDS *p, uint32_t n){
    if (n >= (uint32_t) p->optext->t.outArgCount)
      return (char*) NULL;
    return (char*) p->optext->t.outlist->arg[n];
  }

  /**
   * Returns the CS_TYPE for an opcode argument argPtr
   */
  static inline CS_TYPE* GetTypeForArg(void* argPtr) {
    char* ptr = (char*) argPtr;
    CS_TYPE* varType = *(CS_TYPE**)(ptr - CS_VAR_TYPE_OFFSET);
    return varType;
  }

  /**
   * Returns MIDI channel number (0 to 15) for the instrument instance
   * that called opcode 'p'.
   * In the case of score notes, -1 is returned.
   */
  static inline int32_t GetMidiChannelNumber(OPDS *p){
    MCHNBLK *chn = p->insdshead->m_chnbp;
    return chn != NULL ? chn->channel : -1;
  }

  /**
   * Returns MIDI note number (in the range 0 to 127) for opcode 'p'.
   * If the opcode was not called from a MIDI activated instrument
   * instance, the return value is undefined.
   */
  static inline int32_t GetMidiNoteNumber(OPDS *p){
    return (int32_t) p->insdshead->m_pitch;
  }

  /**
   * Returns MIDI velocity (in the range 0 to 127) for opcode 'p'.
   * If the opcode was not called from a MIDI activated instrument
   * instance, the return value is undefined.
   */
  static inline int32_t GetMidiVelocity(OPDS *p){
    return (int32_t) p->insdshead->m_veloc;
  }

  /**
   * Returns a pointer to the MIDI channel structure for the instrument
   * instance that called opcode 'p'.
   * In the case of score notes, NULL is returned.
   */
  static inline MCHNBLK *GetMidiChannel(OPDS *p){
    return p->insdshead->m_chnbp;
  }


  /**
   * Returns non-zero if the current note (owning opcode 'p') is releasing.
   */
  static inline int32_t GetReleaseFlag(void *p){
    return (int32_t) ((OPDS*) p)->insdshead->relesing;
  }

  /**
   * Returns the note-off time in seconds (measured from the beginning of
   * performance) of the current instrument instance, from which opcode 'p'
   * was called. The return value may be negative if the note has indefinite
   * duration.
   */
  static inline double GetOffTime(OPDS *p){
    return (double) p->insdshead->offtim;
  }

  /**
   * Returns the array of p-fields passed to the instrument instance
   * that owns opcode 'p', starting from p0. Only p1, p2, and p3 are
   * guaranteed to be available. p2 is measured in seconds from the
   * beginning of the current section.
   */
  static inline CS_VAR_MEM *GetPFields(void *p){
    return &(((OPDS*) p)->insdshead->p0);
  }

  /**
   * Returns the instrument number (p1) for opcode 'p'.
   */
  static inline int32_t GetInstrumentNumber(OPDS *p){
    return (int32_t) p->insdshead->p1.value;
  }

  /**
   * Returns the local ksmps of instrument/UDO containing opcode p.
   * This is an alternative to the macro CS_KSMPS.
   */
  static inline uint32_t GetLocalKsmps(OPDS *p){
    return (uint32_t) p->insdshead->ksmps;
  }

  /**
   * Returns the local sr of instrument/UDO containing opcode p.
   * This is an alternative to the macro CS_ESR.
   */
  static inline MYFLT GetLocalSr(OPDS *p){
    // FIXME: this needs to be adjusted once local sr is
    // implemented
    return p->insdshead->esr;
  }


  /**
   * Returns the local kr of instrument/UDO containing opcode p.
   * This is an alternative to the macro CS_EKR.
   */
  static inline MYFLT GetLocalKr(OPDS *p){
    return p->insdshead->ekr;
  }

  /**
   * Returns the local kcount of instrument/UDO containing opcode p.
   * This is an alternative to the macro CS_KCOUNTER.
   */
  static inline uint64_t GetLocalKcounter(OPDS *p){
    return p->insdshead->kcounter;
  }

  /**
   * Returns the opcode name for p.
   */
  static inline char *GetOpcodeName(OPDS *p){
    return p->optext->t.oentry->opname;
  }
  /**@}*/

  static inline char le_test(){
    union _le {
      char c[2];
      short s;
    } le = {{0x0001}};
    return le.c[0];
  }

  static inline char *byteswap(char *p, int32_t N){
    if (le_test()) {
      char tmp;
      int32_t j ;
      for(j = 0; j < N/2; j++) {
        tmp = p[j];
        p[j] = p[N - j - 1];
        p[N - j - 1] = tmp;
      }
    }
    return p;
  }

   /**
   * Functions used in Csound utilities
   * can be accessed via csound->GetUtility(csound);
   */  
  typedef struct _CSOUND_UTIL {
    int32_t (*AddUtility)(CSOUND *, const char *name,
                      int32_t (*UtilFunc)(CSOUND *, int32_t, char **));
    int32_t (*RunUtility)(CSOUND *, const char *name, int32_t argc, char **argv);
    char **(*ListUtilities)(CSOUND *);
    int32_t (*SetUtilityDescription)(CSOUND *, const char *utilName,
                                 const char *utilDesc);
    const char *(*GetUtilityDescription)(CSOUND *, const char *utilName);
    void (*SetUtilSr)(CSOUND *, MYFLT);
    void (*SetUtilNchnls)(CSOUND *, int32_t);
    void *(*SndinGetSetSA)(CSOUND *,
                              char *, void *, MYFLT *, MYFLT *, MYFLT *, int32_t);
    void *(*SndinGetSet)(CSOUND *, void *);
    int32_t (*Sndin)(CSOUND *, void *, MYFLT *, int32_t, void *);
  } CSOUND_UTIL;

  

#include "find_opcode.h"

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

    /** @name Attributes */
    /**@{ */
    /** Get number of output channels */
    uint32_t (*GetNchnls)(CSOUND *);
    /** Get number of input channels */
    uint32_t (*GetNchnls_i)(CSOUND *);
    /** Get max peak amp */
    MYFLT (*Get0dBFS) (CSOUND *);
    /** Get reference tuning */
    MYFLT (*GetA4)(CSOUND *);
    /** Get current tie flag */
    int32_t (*GetTieFlag)(CSOUND *);
    /** Get current reinit flag */
    int32_t (*GetReinitFlag)(CSOUND *);
    /** Get current compiled instrument list */
    INSTRTXT **(*GetInstrumentList)(CSOUND *);

    void *(*GetHostData)(CSOUND *);
    int64_t (*GetCurrentTimeSamples)(CSOUND *);
    long (*GetInputBufferSize)(CSOUND *);
    long (*GetOutputBufferSize)(CSOUND *);
    int32_t (*GetDebug)(CSOUND *);
    int32_t (*GetSizeOfMYFLT)(void);
    const OPARMS *(*GetOParms)(CSOUND *);
    const char *(*GetEnv)(CSOUND *, const char *name);
    MYFLT (*GetSystemSr)(CSOUND *, MYFLT );
    /**@}*/

    /** @name Software bus */
    /**@{ */
    int32_t (*GetChannelPtr)(CSOUND *, void **ptr, const char *name, int32_t mode);
    int32_t (*ListChannels)(CSOUND *, controlChannelInfo_t **list);
    /**@}*/

    /** @name Events and Score */
    /**@{ */
    int32_t (*CheckEvents)(CSOUND *);
    int32_t (*InsertScoreEvent)(CSOUND *, EVTBLK *, double);
    MYFLT (*GetScoreOffsetSeconds)(CSOUND *);
    void (*SetScoreOffsetSeconds)(CSOUND *, MYFLT);
    void (*RewindScore)(CSOUND *);
    void (*InputMessage)(CSOUND *, const char *message__);
    int (*ReadScore)(CSOUND *, const char*);
    /**@}*/

    /** @name Message printout */
    /**@{ */
    CS_PRINTF2 void (*Message)(CSOUND *, const char *fmt, ...);
    CS_PRINTF3 void (*MessageS)(CSOUND *, int32_t attr, const char *fmt, ...);
    void (*MessageV)(CSOUND *, int32_t attr, const char *format, va_list args);
    int32_t (*GetMessageLevel)(CSOUND *);
    void (*SetMessageLevel)(CSOUND *, int32_t messageLevel);
    void (*SetMessageCallback)(CSOUND *,
                               void (*csoundMessageCallback)
                               (CSOUND *,int32_t attr, const char *format,
                                va_list valist));
    /**@}*/

    /** @name Arguments and Types */
    /**@{ */
    char *(*GetString)(CSOUND *, MYFLT);
    int32 (*StringArg2Insno)(CSOUND *, void *p, int32_t is_string);
    char *(*StringArg2Name)(CSOUND *, char *, void *, const char *, int32_t);
    const CS_TYPE *(*GetType)(CSOUND *csound, const char *type);

    /**@}*/

    /** @name Memory allocation */
    /**@{ */
    void (*AuxAlloc)(CSOUND *, size_t nbytes, AUXCH *auxchp);
    int32_t (*AuxAllocAsync)(CSOUND *, size_t, AUXCH  *,
                         AUXASYNC *, aux_cb, void *);
    void *(*Malloc)(CSOUND *, size_t nbytes);
    void *(*Calloc)(CSOUND *, size_t nbytes);
    void *(*ReAlloc)(CSOUND *, void *oldp, size_t nbytes);
    char *(*Strdup)(CSOUND *, const char*);
    void (*Free)(CSOUND *, void *ptr);
    /**@}*/

    /** @name Function tables */
    /**@{ */
    int32_t (*FTCreate)(CSOUND *, FUNC **, const EVTBLK *, int32_t);
    int32_t (*FTAlloc)(CSOUND *, int32_t tableNum, int32_t len);
    int32_t (*FTDelete)(CSOUND *, int32_t tableNum);
    FUNC *(*FTFind)(CSOUND *, MYFLT *argp);
    void *(*GetNamedGens)(CSOUND *);
    /**@}*/

    /** @name Global and config variable manipulation */
    /**@{ */
    int32_t (*CreateGlobalVariable)(CSOUND *, const char *name, size_t nbytes);
    void *(*QueryGlobalVariable)(CSOUND *, const char *name);
    void *(*QueryGlobalVariableNoCheck)(CSOUND *, const char *name);
    int32_t (*DestroyGlobalVariable)(CSOUND *, const char *name);
    int32_t (*CreateConfigurationVariable)(CSOUND *, const char *name,
                                       void *p, int32_t type, int32_t flags,
                                       void *min, void *max,
                                       const char *shortDesc,
                                       const char *longDesc);
    int32_t (*SetConfigurationVariable)(CSOUND *, const char *name, void *value);
    int32_t (*ParseConfigurationVariable)(CSOUND *,
                                      const char *name, const char *value);
    csCfgVariable_t *(*QueryConfigurationVariable)(CSOUND *, const char *name);
    csCfgVariable_t **(*ListConfigurationVariables)(CSOUND *);
    int32_t (*DeleteConfigurationVariable)(CSOUND *, const char *name);
    const char *(*CfgErrorCodeToString)(int32_t errcode);
    /**@}*/

    /** @name FFT support */
    /**@{ */
    void *(*RealFFTSetup)(CSOUND *csound, int32_t FFTsize, int32_t d);
    void (*RealFFT)(CSOUND *csound,
                    void *p, MYFLT *sig);
    MYFLT (*GetInverseRealFFTScale)(CSOUND *, int32_t FFTsize);
    void (*ComplexFFT)(CSOUND *, MYFLT *buf, int32_t FFTsize);
    void (*InverseComplexFFT)(CSOUND *, MYFLT *buf, int32_t FFTsize);
    MYFLT (*GetInverseComplexFFTScale)(CSOUND *, int32_t FFTsize);
    void (*RealFFTMult)(CSOUND *, MYFLT *outbuf, MYFLT *buf1, MYFLT *buf2,
                        int32_t FFTsize, MYFLT scaleFac);
    void *(*DCTSetup)(CSOUND *csound,int32_t FFTsize, int32_t d);
    void (*DCT)(CSOUND *csound, void *p, MYFLT *sig);
    /**@}*/

    /** @name LPC support */
    /**@{ */
    MYFLT* (*AutoCorrelation)(CSOUND *, MYFLT*, MYFLT*, int32_t, MYFLT*, int32_t);
    void * (*LPsetup)(CSOUND *csound, int32_t N, int32_t M);
    void (*LPfree)(CSOUND *csound, void *);
    MYFLT* (*LPred)(CSOUND *, void *, MYFLT *);
    MYFLT* (*LPCeps)(CSOUND *, MYFLT *, MYFLT *, int32_t, int32_t);
    MYFLT* (*CepsLP)(CSOUND *, MYFLT *, MYFLT *, int32_t, int32_t);
    MYFLT (*LPrms)(CSOUND *, void *);
    /**@}*/

    /** @name PVOC-EX system */
    /**@{ */
    int32_t (*PVOC_CreateFile)(CSOUND *, const char *,
                           uint32, uint32, uint32,
                           uint32, int32, int32_t, int32_t,
                           float, float *, uint32);
    int32_t (*PVOC_OpenFile)(CSOUND *, const char *, void  *, void *);
    int32_t (*PVOC_CloseFile)(CSOUND *, int32_t);
    int32_t (*PVOC_PutFrames)(CSOUND *, int32_t, const float *, int32);
    int32_t (*PVOC_GetFrames)(CSOUND *, int32_t, float *, uint32);
    int32_t (*PVOC_FrameCount)(CSOUND *, int32_t);
    int32_t (*PVOC_fseek)(CSOUND *, int32_t, int32_t);
    const char *(*PVOC_ErrorString)(CSOUND *);
    int32_t (*PVOCEX_LoadFile)(CSOUND *, const char *, PVOCEX_MEMFILE *);
    /**@}*/

    /** @name Error messages */
    /**@{ */
    CS_NORETURN CS_PRINTF2 void (*Die)(CSOUND *, const char *msg, ...);
    CS_PRINTF2 int32_t (*InitError)(CSOUND *, const char *msg, ...);
    CS_PRINTF3 int32_t (*PerfError)(CSOUND *, OPDS *h,  const char *msg, ...);
    CS_PRINTF2 int32_t  (*FtError)(const FGDATA *, const char *, ...);
    CS_PRINTF2 void (*Warning)(CSOUND *, const char *msg, ...);
    CS_PRINTF2 void (*DebugMsg)(CSOUND *, const char *msg, ...);
    CS_NORETURN void (*LongJmp)(CSOUND *, int32_t);
    CS_PRINTF2 void (*ErrorMsg)(CSOUND *, const char *fmt, ...);
    void (*ErrMsgV)(CSOUND *, const char *hdr, const char *fmt, va_list);
    /**@}*/

    /** @name Random numbers */
    /**@{ */
    uint32_t (*GetRandomSeedFromTime)(void);
    void (*SeedRandMT)(CsoundRandMTState *p,
                       const uint32_t *initKey, uint32_t keyLength);
    uint32_t (*RandMT)(CsoundRandMTState *p);
    int32_t (*Rand31)(int32_t *seedVal);
    int32_t *(*RandSeed1)(CSOUND *);
    int32_t (*GetRandSeed)(CSOUND *, int32_t which);
    /**@}*/

    /** @name Threads and locks */
    /**@{ */
    void *(*CreateThread)(uintptr_t (*threadRoutine)(void *), void *userdata);
    uintptr_t (*JoinThread)(void *thread);
    void *(*CreateThreadLock)(void);
    void (*DestroyThreadLock)(void *lock);
    int32_t (*WaitThreadLock)(void *lock, size_t milliseconds);
    void (*NotifyThreadLock)(void *lock);
    void (*WaitThreadLockNoTimeout)(void *lock);
    void *(*Create_Mutex)(int32_t isRecursive);
    int32_t (*LockMutexNoWait)(void *mutex_);
    void (*LockMutex)(void *mutex_);
    void (*UnlockMutex)(void *mutex_);
    void (*DestroyMutex)(void *mutex_);
    void *(*CreateBarrier)(uint32_t max);
    int32_t (*DestroyBarrier)(void *);
    int32_t (*WaitBarrier)(void *);
    void *(*GetCurrentThreadID)(void);
    void (*Sleep)(size_t milliseconds);
    void (*InitTimerStruct)(RTCLOCK *);
    double (*GetRealTime)(RTCLOCK *);
    double (*GetCPUTime)(RTCLOCK *);
    /**@}*/

    /** @name Circular lock-free buffer */
    /**@{ */
    void *(*CreateCircularBuffer)(CSOUND *, int32_t, int32_t);
    int32_t (*ReadCircularBuffer)(CSOUND *, void *, void *, int32_t);
    int32_t (*WriteCircularBuffer)(CSOUND *, void *, const void *, int32_t);
    int32_t (*PeekCircularBuffer)(CSOUND *, void *, void *, int32_t);
    void (*FlushCircularBuffer)(CSOUND *, void *);
    void (*DestroyCircularBuffer)(CSOUND *, void *);
    /**@}*/

    /** @name File access */
    /**@{ */
    char *(*FindInputFile)(CSOUND *, const char *filename, const char *envList);
    char *(*FindOutputFile)(CSOUND *,
                            const char *filename, const char *envList);
    void *(*FileOpen)(CSOUND *, void *, int32_t, const char *, void *,
                      const char *, int32_t, int32_t); /* Rename FileOpen */
    void (*NotifyFileOpened)(CSOUND*, const char*, int32_t, int32_t, int32_t);
    int32_t (*FileClose)(CSOUND *, void *);
    const char *(*FileError)(CSOUND *, void *);
    void *(*FileOpenAsync)(CSOUND *, void *, int32_t, const char *, void *,
                           const char *, int32_t, int32_t, int32_t);
    uint32_t (*ReadAsync)(CSOUND *, void *, MYFLT *, int32_t);
    uint32_t (*WriteAsync)(CSOUND *, void *, MYFLT *, int32_t);
    int32_t  (*FSeekAsync)(CSOUND *, void *, int32_t, int32_t);
    void (*RewriteHeader)(CSOUND *csound, void *ofd);
    SNDMEMFILE *(*LoadSoundFile)(CSOUND *, const char *, void *);
    MEMFIL *(*LoadMemoryFile)(CSOUND *, const char *, int32_t,
                              int32_t (*callback)(CSOUND *, MEMFIL *));
    void (*FDRecord)(CSOUND *, FDCH *fdchp);
    void (*FDClose)(CSOUND *, FDCH *fdchp);
    void *(*CreateFileHandle)(CSOUND *, void *, int32_t, const char *);
    char *(*GetFileName)(void *);
    int32_t (*Type2CsfileType)(int32_t type, int32_t encoding);
    int32_t (*SndfileType2CsfileType)(int32_t type);
    char *(*Type2String)(int32_t type);
    char *(*GetStrFormat)(int32_t format);
    int32_t (*SndfileSampleSize)(int32_t format);
    /**@}*/

     /** @name Soundfile interface */
    /**@{ */   
    void *(*SndfileOpen)(CSOUND *csound, const char *path, int32_t mode,
                        SFLIB_INFO *sfinfo);
    void *(*SndfileOpenFd)(CSOUND *csound,
                          int32_t fd, int32_t mode, SFLIB_INFO *sfinfo,
                          int32_t close_desc);
    int32_t (*SndfileClose)(CSOUND *csound, void *);
    int64_t (*SndfileWrite)(CSOUND *, void *, MYFLT *, int64_t);
    int64_t (*SndfileRead)(CSOUND *, void *, MYFLT *, int64_t);
    int64_t (*SndfileWriteSamples)(CSOUND *, void *, MYFLT *, int64_t);
    int64_t (*SndfileReadSamples)(CSOUND *, void *, MYFLT *, int64_t);
    int64_t (*SndfileSeek)(CSOUND *, void *, int64_t, int32_t);
    int32_t (*SndfileSetString)(CSOUND *csound, void *sndfile, int32_t str_type, const char* str);
    const char *(*SndfileStrError)(CSOUND *csound, void *);
    int32_t (*SndfileCommand)(CSOUND *, void *, int32_t , void *, int32_t );
    /**@}*/    

    /** @name Generic callbacks */
    /**@{ */
    int32_t (*Set_KeyCallback)(CSOUND *, int32_t (*func)(void *, void *, uint32_t),
                           void *userData, uint32_t typeMask);
    void (*Remove_KeyCallback)(CSOUND *,
                               int32_t (*func)(void *, void *, uint32_t));
    int32_t (*RegisterResetCallback)(CSOUND *, void *userData,
                                 int32_t (*func)(CSOUND *, void *));
    /**@}*/

    /** @name Hash tables */
    /**@{ */
    CS_HASH_TABLE *(*CreateHashTable)(CSOUND *);
    void *(*GetHashTableValue)(CSOUND *, CS_HASH_TABLE *, char *);
    void (*SetHashTableValue)(CSOUND *, CS_HASH_TABLE *, char *, void *);
    void (*RemoveHashTableKey)(CSOUND *, CS_HASH_TABLE *, char *);
    void (*DestroyHashTable)(CSOUND *, CS_HASH_TABLE *);
    char *(*GetHashTableKey)(CSOUND *, CS_HASH_TABLE *, char *);
    CONS_CELL *(*GetHashTableKeys)(CSOUND *, CS_HASH_TABLE *);
    CONS_CELL *(*GetHashTableValues)(CSOUND *, CS_HASH_TABLE *);
    /**@}*/

    /** @name Plugin opcodes and discovery support */
    /**@{ */
    int32_t (*AppendOpcode)(CSOUND *, const char *opname, int32_t dsblksiz, int32_t flags,
                        const char *outypes, const char *intypes,
                        int32_t (*init)(CSOUND *, void *),
                        int32_t (*perf)(CSOUND *, void *),
                        int32_t (*deinit)(CSOUND *, void *));
    int32_t (*AppendOpcodes)(CSOUND *, const OENTRY *opcodeList, int32_t n);
    OENTRY* (*FindOpcode)(CSOUND*, char*, char* , char*);
    /**@}*/

    /** @name RT audio IO module support */
    /**@{ */
    void (*SetPlayopenCallback)(CSOUND *,
                                int32_t (*playopen__)(CSOUND *, const csRtAudioParams *parm));
    void (*SetRtplayCallback)(CSOUND *,
                              void (*rtplay__)(CSOUND *, const MYFLT *outBuf, int32_t nbytes));
    void (*SetRecopenCallback)(CSOUND *,
                               int32_t (*recopen__)(CSOUND *, const csRtAudioParams *parm));
    void (*SetRtrecordCallback)(CSOUND *,
                                int32_t (*rtrecord__)(CSOUND *, MYFLT *inBuf, int32_t nbytes));
    void (*SetRtcloseCallback)(CSOUND *, void (*rtclose__)(CSOUND *));
    void (*SetAudioDeviceListCallback)(CSOUND *csound,
                                       int32_t (*audiodevlist__)(CSOUND *, CS_AUDIODEVICE *list, int32_t isOutput));
    void **(*GetRtRecordUserData)(CSOUND *);
    void **(*GetRtPlayUserData)(CSOUND *);
    int32_t (*GetDitherMode)(CSOUND *);
    /**@}*/

    /** @name RT MIDI module support */
    /**@{ */
    void (*SetExternalMidiInOpenCallback)(CSOUND *,
                                          int32_t (*func)(CSOUND *, void **, const char *));
    void (*SetExternalMidiReadCallback)(CSOUND *,
                                        int32_t (*func)(CSOUND *, void *, unsigned char *, int32_t));
    void (*SetExternalMidiInCloseCallback)(CSOUND *,
                                           int32_t (*func)(CSOUND *, void *));
    void (*SetExternalMidiOutOpenCallback)(CSOUND *,
                                           int32_t (*func)(CSOUND *, void **, const char *));
    void (*SetExternalMidiWriteCallback)(CSOUND *,
                                         int32_t (*func)(CSOUND *, void *, const unsigned char *, int32_t));
    void (*SetExternalMidiOutCloseCallback)(CSOUND *,
                                            int32_t (*func)(CSOUND *, void *));
    void (*SetExternalMidiErrorStringCallback)(CSOUND *,
                                               const char *(*func)(int32_t));
    void (*SetMIDIDeviceListCallback)(CSOUND *csound,
                                      int32_t (*audiodevlist__)(CSOUND *, CS_MIDIDEVICE *list, int32_t isOutput));
    void (*ModuleListAdd)(CSOUND *, char *, char *);
    /**@}*/
    
    /** @name Displays & graphs support */
    /**@{ */
    void (*SetDisplay)(CSOUND *, WINDAT *, MYFLT *, int32, char *, int32_t, char *);
    void (*Display)(CSOUND *, WINDAT *);
    int32_t (*DeinitDisplay)(CSOUND *);
    void (*InitDisplay)(CSOUND *);
    int32_t (*SetIsGraphable)(CSOUND *, int32_t isGraphable);
    void (*SetMakeGraphCallback)(CSOUND *,
                                 void (*makeGraphCallback)(CSOUND *, WINDAT *p,
                                                           const char *name));
    void (*SetDrawGraphCallback)(CSOUND *,
                                 void (*drawGraphCallback)(CSOUND *, WINDAT *p));
    void (*SetKillGraphCallback)(CSOUND *,
                                 void (*killGraphCallback)(CSOUND *, WINDAT *p));
    void (*SetExitGraphCallback)(CSOUND *, int32_t (*exitGraphCallback)(CSOUND *));
    /**@}*/

    /** @name Miscellaneous */
    /**@{ */
    /* access functions used in csound utilities */
    const CSOUND_UTIL *(*GetUtility)(CSOUND *csound);
    /* Fast power of two function from a precomputed table */
    MYFLT (*Pow2)(CSOUND *, MYFLT a);
#if defined (__CUDACC__) || defined (__MACH__)
    char *(*LocalizeString)(const char *);
#else
    char *(*LocalizeString)(const char *) __attribute__ ((format_arg (1)));
#endif
    double (*Strtod)(char* nptr, char**);
    int32_t (*Sprintf)(char *str, const char *format, ...);
    int32_t (*Sscanf)(char *str, const char *format, ...);
    /**@}*/


    /** @name Placeholders
        To allow the API to grow while maintining backward binary compatibility. */
    /**@{ */
    SUBR dummyfn_2[13];
    /**@}*/
#ifdef __BUILDING_LIBCSOUND
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
    SUBR          first_callback_;
    channelCallback_t InputChannelCallback_;
    channelCallback_t OutputChannelCallback_;
    void          (*csoundMessageCallback_)(CSOUND *, int32_t attr,
                                            const char *format, va_list args);
    int32_t           (*csoundConfigureCallback_)(CSOUND *);
    void          (*csoundMakeGraphCallback_)(CSOUND *, WINDAT *windat,
                                              const char *name);
    void          (*csoundDrawGraphCallback_)(CSOUND *, WINDAT *windat);
    void          (*csoundKillGraphCallback_)(CSOUND *, WINDAT *windat);
    int32_t           (*csoundExitGraphCallback_)(CSOUND *);
    int32_t           (*csoundYieldCallback_)(CSOUND *);
    void          (*cscoreCallback_)(CSOUND *);
    void*         (*OpenSoundFileCallback_)(CSOUND*, const char*, int32_t, void*);
    FILE*         (*OpenFileCallback_)(CSOUND*, const char*, const char*);
    void          (*FileOpenCallback_)(CSOUND*, const char*, int32_t, int32_t, int32_t);
    SUBR          last_callback_;
    /* these are not saved on RESET */
    int32_t           (*playopen_callback)(CSOUND *, const csRtAudioParams *parm);
    void          (*rtplay_callback)(CSOUND *, const MYFLT *outBuf, int32_t nbytes);
    int32_t           (*recopen_callback)(CSOUND *, const csRtAudioParams *parm);
    int32_t           (*rtrecord_callback)(CSOUND *, MYFLT *inBuf, int32_t nbytes);
    void          (*rtclose_callback)(CSOUND *);
    int32_t           (*audio_dev_list_callback)(CSOUND *, CS_AUDIODEVICE *, int32_t);
    int32_t           (*midi_dev_list_callback)(CSOUND *, CS_MIDIDEVICE *, int32_t);
    int32_t           (*doCsoundCallback)(CSOUND *, void *, uint32_t);
    int32_t           (*csoundInternalYieldCallback_)(CSOUND *);
    /* end of callbacks */
    void          (*spinrecv)(CSOUND *);
    void          (*spoutran)(CSOUND *);
    int32_t           (*audrecv)(CSOUND *, MYFLT *, int32_t);
    void          (*audtran)(CSOUND *, const MYFLT *, int32_t);
    void          *hostdata;
    char          *orchname, *scorename;
    CORFIL        *orchstr, *scorestr;
    OPDS          *ids;             /* used by init loops */
    ENGINE_STATE  engineState;      /* current Engine State merged after
                                       compilation */
    INSTRTXT      *instr0;          /* instr0     */
    INSTRTXT      **dead_instr_pool;
    int32_t           dead_instr_no;
    TYPE_POOL*    typePool;
    uint32_t  ksmps;
    uint32_t      nchnls;
    int32_t           inchnls;
    uint64_t      kcounter, global_kcounter;
    MYFLT         esr;
    MYFLT         ekr;
    /** current time in seconds, inc. per kprd */
    int64_t       icurTime;   /* Current time in samples */
    double        curTime_inc;
    /** start time of current section    */
    double        timeOffs, beatOffs;
    /** current time in beats, inc per kprd */
    double        curBeat, curBeat_inc;
    /** beat time = 60 / tempo           */
    int64_t       ibeatTime;   /* Beat time in samples */
    EVTBLK        *currevent;
    INSDS         *curip;
    MYFLT         cpu_power_busy;
    char          *xfilename;
    int32_t           peakchunks;
    int32_t           keep_tmp;
    CS_HASH_TABLE *opcodes;
    int32         nrecs;
    FILE*         Linepipe;
    int32_t           Linefd;
    void          *csoundCallbacks_;
    FILE*         scfp;
    CORFIL        *scstr;
    FILE*         oscfp;
    MYFLT         maxamp[MAXCHNLS];
    MYFLT         smaxamp[MAXCHNLS];
    MYFLT         omaxamp[MAXCHNLS];
    uint32        maxpos[MAXCHNLS], smaxpos[MAXCHNLS], omaxpos[MAXCHNLS];
    FILE*         scorein;
    FILE*         scoreout;
    int32_t           *argoffspace;
    INSDS         *frstoff;
    int32_t           randSeed1;
    int32_t           randSeed2;
    CsoundRandMTState *csRandState;
    RTCLOCK       *csRtClock;
    int32_t           strsmax;
    char          **strsets;
    MYFLT         *spin;
    MYFLT         *spout;
    MYFLT         *spout_tmp;
    int32_t           nspin;
    int32_t           nspout;
    MYFLT         *auxspin;
    OPARMS        *oparms;
    /** reserve space for up to MIDIMAXPORTS MIDI devices */
    MCHNBLK       *m_chnbp[MIDIMAXPORTS*16];
    int32_t           dither_output;
    MYFLT         onedsr, sicvt;
    MYFLT         tpidsr, pidsr, mpidsr, mtpdsr;
    MYFLT         onedksmps;
    MYFLT         onedkr;
    MYFLT         kicvt;
    int32_t           reinitflag;
    int32_t           tieflag;
    MYFLT         e0dbfs, dbfs_to_float;
    double        A4;
    void          *rtRecord_userdata;
    void          *rtPlay_userdata;
    jmp_buf       exitjmp;
    SRTBLK        *frstbp;
    int32_t           sectcnt;
    int32_t           inerrcnt, synterrcnt, perferrcnt;
    INSDS         actanchor;
    int32         rngcnt[MAXCHNLS];
    int16         rngflg, multichan;
    void          *evtFuncChain;
    EVTNODE       *OrcTrigEvts;             /* List of events to be started */
    EVTNODE       *freeEvtNodes;
    int32_t           csoundIsScorePending_;
    int64_t       advanceCnt;
    int32_t           initonly;
    int32_t           evt_poll_cnt;
    int32_t           evt_poll_maxcnt;
    int32_t           Mforcdecs, Mxtroffs, MTrkend;
    OPCODINFO     *opcodeInfo;
    FUNC**        flist;
    int32_t           maxfnum;
    GEN           *gensub;
    int32_t           genmax;
    CS_HASH_TABLE *namedGlobals;
    CS_HASH_TABLE *cfgVariableDB;
    double        prvbt, curbt, nxtbt;
    double        curp2, nxtim;
    int64_t       cyclesRemaining;
    EVTBLK        evt;
    void          *memalloc_db;
    MGLOBAL       *midiGlobals;
    CS_HASH_TABLE *envVarDB;
    MEMFIL        *memfiles;
    PVOCEX_MEMFILE *pvx_memfiles;
    int32_t           FFT_max_size;
    void          *FFT_table_1;
    void          *FFT_table_2;
    /* statics from twarp.c should be TSEG* */
    void          *tseg, *tpsave;
    /* persistent macros */
    MACRO         *orc_macros;
    /* Statics from express.c */
    MYFLT         *gbloffbas;       /* was static in oload.c */
    void          *file_io_thread;
    int32_t           file_io_start;
    void          *file_io_threadlock;
    int32_t           realtime_audio_flag;
    void          *event_insert_thread;
    int32_t           event_insert_loop;
    void          *init_pass_threadlock;
    void          *API_lock;
    spin_lock_t   spoutlock, spinlock;
    spin_lock_t   memlock, spinlock1;
    char          *delayederrormessages;
    void          *printerrormessagesflag;
    struct sread__ {
      SRTBLK  *bp, *prvibp;           /* current srtblk,  prev w/same int(p1) */
      char    *sp, *nxp;              /* string pntrs into srtblk text        */
      int32_t     op;                     /* opcode of current event              */
      int32_t     warpin;                 /* input format sensor                  */
      int32_t     linpos;                 /* line position sensor                 */
      int32_t     lincnt;                 /* count of lines/section in scorefile  */
      MYFLT   prvp2 /* = -FL(1.0) */;     /* Last event time                  */
      MYFLT   clock_base /* = FL(0.0) */;
      MYFLT   warp_factor /* = FL(1.0) */;
      char    *curmem;
      char    *memend;                /* end of cur memblk                    */
      MACRO   *unused_ptr2;
      int32_t     last_name /* = -1 */;
      IN_STACK  *inputs, *str;
      int32_t     input_size, input_cnt;
      int32_t     unused_int3;
      int32_t     unused_int2;
      int32_t     linepos /* = -1 */;
      MARKED_SECTIONS names[30];
#define NAMELEN 40              /* array size of repeat macro names */
#define RPTDEPTH 40             /* size of repeat_n arrays (39 loop levels) */
      char    unused_char0[RPTDEPTH][NAMELEN];
      int32_t     unused_int4[RPTDEPTH];
      int32   unused_int7[RPTDEPTH];
      int32_t     unused_int5;
      MACRO   *unused_ptr0[RPTDEPTH];
      int32_t     unused_int6;
      /* Variable for repeat sections */
      char    unused_char1[NAMELEN];
      int32_t     unused_int8;
      int32   unused_int9;
      int32_t     unused_intA;
      MACRO   *unused_ptr1;
      int32_t     nocarry;
    } sread;
    struct onefileStatics__ {
      NAMELST *toremove;
      char    *orcname;
      char    *sconame;
      char    *midname;
      int32_t     midiSet;
      int32_t     csdlinecount;
    } onefileStatics;
#define LBUFSIZ   32768
    struct lineventStatics__ {
      char    *Linep, *Linebufend;
      int32_t     stdmode;
      EVTBLK  prve;
      char    *Linebuf;
      int32_t     linebufsiz;
      char *orchestra, *orchestrab;
      int32_t   oflag;
    } lineventStatics;
    struct musmonStatics__ {
      int32   srngcnt[MAXCHNLS], orngcnt[MAXCHNLS];
      int16   srngflg;
      int16   sectno;
      int32_t     lplayed;
      int32_t     segamps, sormsg;
      EVENT   **ep, **epend;      /* pointers for stepping through lplay list */
      EVENT   *lsect;
    } musmonStatics;
    struct libsndStatics__ {
      void       *outfile;
      void       *infile;
      char          *sfoutname;           /* soundout filename            */
      MYFLT         *inbuf;
      MYFLT         *outbuf;              /* contin sndio buffers         */
      MYFLT         *outbufp;             /* MYFLT pntr                   */
      uint32        inbufrem;
      uint32        outbufrem;            /* in monosamps                 */
                                          /* (see openin, iotranset)      */
      uint32_t  inbufsiz,  outbufsiz; /* alloc in sfopenin/out        */
      int32_t           isfopen;              /* (real set in sfopenin)       */
      int32_t           osfopen;              /* (real set in sfopenout)      */
      int32_t           pipdevin, pipdevout;  /* 0: file, 1: pipe, 2: rtaudio */
      uint32        nframes               /* = 1UL */;
      FILE          *pin, *pout;
      int32_t           dither;
    } libsndStatics;

    int32_t           warped;               /* rdscor.c */
    int32_t           sstrlen;
    char          *sstrbuf;
    int32_t           enableMsgAttr;        /* csound.c */
    int32_t           sampsNeeded;
    MYFLT         csoundScoreOffsetSeconds_;
    int32_t           inChar_;
    int32_t           isGraphable_;
    int32_t           delayr_stack_depth;   /* ugens6.c */
    void          *first_delayr;
    void          *last_delayr;
    int32         revlpsiz[6];
    int32         revlpsum;
    double        rndfrac;              /* aops.c */
    MYFLT         *logbase2;
    NAMES         *omacros, *smacros;
    void          *namedgen;            /* fgens.c */
    void          *open_files;          /* fileopen.c */
    void          *searchPathCache;
    CS_HASH_TABLE *sndmemfiles;
    void          *reset_list;
    void          *pvFileTable;         /* pvfileio.c */
    int32_t           pvNumFiles;
    int32_t           pvErrorCode;
    /* database for deferred loading of opcode plugin libraries */
    //    void          *pluginOpcodeFiles;
    int32_t           enableHostImplementedAudioIO;
    int32_t           enableHostImplementedMIDIIO;
    int32_t           hostRequestedBufferSize;
    /* engineStatus is sum of:
     *   1 (CS_STATE_PRE):  csoundPreCompile was called
     *   2 (CS_STATE_COMP): csoundCompile was called
     *   4 (CS_STATE_UTIL): csoundRunUtility was called
     *   8 (CS_STATE_CLN):  csoundCleanup needs to be called
     *  16 (CS_STATE_JMP):  csoundLongJmp was called
     */
    char          engineStatus;
    /* stdXX_assign_flags  can be {1,2,4,8} */
    char          stdin_assign_flg;
    char          stdout_assign_flg;
    char          orcname_mode;         /* 0: normal, 1: ignore, 2: fail */
    int32_t           use_only_orchfile;
    void          *csmodule_db;
    char          *dl_opcodes_oplibs;
    char          *SF_csd_licence;
    char          *SF_id_title;
    char          *SF_id_copyright;
    int32_t           SF_id_scopyright;
    char          *SF_id_software;
    char          *SF_id_artist;
    char          *SF_id_comment;
    char          *SF_id_date;
    void          *utility_db;
    int16         *isintab;             /* ugens3.c */
    void          *lprdaddr;            /* ugens5.c */
    int32_t           currentLPCSlot;
    int32_t           max_lpc_slot;
    CS_HASH_TABLE *chn_db;
    int32_t           opcodedirWasOK;
    int32_t           disable_csd_options;
    CsoundRandMTState randState_;
    int32_t           performState;
    int32_t           ugens4_rand_16;
    int32_t           ugens4_rand_15;
    void          *schedule_kicked;
    MYFLT         *disprep_fftcoefs;
    void          *winEPS_globals;
    OPARMS        oparms_;
    REMOT_BUF     SVrecvbuf;  /* RM: rt_evt input Communications buffer */
    void          *remoteGlobals;
    /* VL: pvs bus */
    int32_t            nchanif, nchanof;
    char           *chanif, *chanof;
    /* VL: internal yield callback */
    int32_t           multiThreadedComplete;
    THREADINFO    *multiThreadedThreadInfo;
    struct dag_t        *multiThreadedDag;
    void          *barrier1;
    void          *barrier2;
    /* Statics from cs_par_dispatch; */
    /* ********These are no longer used******** */
    void          *pointer1; //struct global_var_lock_t *global_var_lock_root;
    void          *pointer2; //struct global_var_lock_t **global_var_lock_cache;
    int32_t           int1; //global_var_lock_count;
    /* statics from cs_par_orc_semantic_analysis */
    struct instr_semantics_t *instCurr;
    struct instr_semantics_t *instRoot;
    int32_t           inInstr;
    int32_t           dag_changed;
    int32_t           dag_num_active;
    INSDS         **dag_task_map;
    volatile stateWithPadding    *dag_task_status;
    watchList     * volatile *dag_task_watch;
    watchList     *dag_wlmm;
    char          **dag_task_dep;
    int32_t           dag_task_max_size;
    uint32_t      tempStatus;    /* keeps track of which files are temps */
    int32_t           orcLineOffset; /* 1 less than 1st orch line in the CSD */
    int32_t           scoLineOffset; /* 1 less than 1st score line in the CSD */
    char*         csdname;
    /* original CSD name; do not free() */
    int32_t           parserNamedInstrFlag;
    int32_t           tran_nchnlsi;
    int32_t           scnt;         /* Count of strings */
    int32_t           strsiz;       /* length of current strings space */
    FUNC          *sinetable;   /* A useful table */
    int32_t           sinelength;   /* Size of table */
    MYFLT         *UNUSEDP;     /* pow2 table */
    MYFLT         *cpsocfrc;    /* cps conv table */
    CORFIL*       expanded_orc; /* output of preprocessor */
    CORFIL*       expanded_sco; /* output of preprocessor */
    char          *filedir[256];/* for location directory */
    void          *message_buffer;
    int32_t           jumpset;
    int32_t           info_message_request;
    int32_t           modules_loaded;
    MYFLT         _system_sr;
    void*         csdebug_data; /* debugger data */
    int32_t (*kperf)(CSOUND *); /* kperf function pointer, to switch between debug
                               and nodebug function */
    int32_t           score_parser;
    int32_t           print_version;
    int32_t           inZero;       /* flag compilation of instr0 */
    struct _message_queue **msg_queue;
    volatile long msg_queue_wget; /* Writer - Get index */
    volatile long msg_queue_wput; /* Writer - Put Index */
    volatile long msg_queue_rstart; /* Reader - start index */
    volatile long msg_queue_items;
    int32_t      aftouch;
    void     *directory;
    ALLOC_DATA *alloc_queue;
    volatile unsigned long alloc_queue_items;
    unsigned long alloc_queue_wp;
    spin_lock_t alloc_spinlock;
    EVTBLK *init_event;
    void (*csoundMessageStringCallback)(CSOUND *csound,
                                        int32_t attr,
                                        const char *str);
    char* message_string;
    volatile unsigned long message_string_queue_items;
    unsigned long message_string_queue_wp;
    message_string_queue_t *message_string_queue;
    int32_t io_initialised;
    char *op;
    int32_t  mode;
    char *opcodedir;
    char *score_srt;
    OSC_MESS osc_message_anchor;
    CORFIL *playscore;
    spin_lock_t osc_spinlock;
    CSOUND_UTIL csound_util;
    /*struct CSOUND_ **self;*/
    /**@}*/
#endif  /* __BUILDING_LIBCSOUND */
  };

  /*
   * Move the C++ guards to enclose the entire file,
   * in order to enable C++ to #include this file.
   */

#define LINKAGE_BUILTIN(name)                                           \
  int32_t name##_init(CSOUND *csound, OENTRY **ep)                      \
  {   (void) csound; *ep = name; return (long) (sizeof(name));  }

#define FLINKAGE_BUILTIN(name)                  \
  NGFENS* name##_init(CSOUND *csound)           \
  {   (void) csound; return name; }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* CSOUNDCORE_H */
