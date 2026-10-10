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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#if !defined(__BUILDING_LIBCSOUND) && !defined(CSOUND_CSDL_H)
#error "Csound plugins and host applications should not include csoundCore.h"
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
#include "csound_data_structures.h"
#include "coreDefs.h"
#include "soundfile.h"

#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus */

/* Source annotations, consumed by scripts/opcode_deprecations.py.
 * These expand to nothing: no runtime cost and no plugin ABI change.
 * Keep the descriptor beside its opcode registration. A frozen function must
 * also carry CSOUND_PRESERVE_LEGACY_BEHAVIOR immediately before its definition.
 * See docs/opcode-deprecation.md before changing historical behavior.
 */
#define CSOUND_DEPRECATED_OPCODE(name, replacement, policy, note)

/* STOP: known legacy behavior is retained for compatibility. Do not correct
 * historical output under this opcode name without a maintainer decision.
 * See the CSOUND_DEPRECATED_OPCODE descriptor in the same source file.
 */
#define CSOUND_PRESERVE_LEGACY_BEHAVIOR(name)

/** @name Arguments, opcodes, and instrument defs */
/**@{ */

  typedef struct arglst {
    int32_t count;
    char *arg[1];
  } ARGLST;

  typedef struct arg {
    int32_t type;
    void *argPtr;
    int32_t index;
    char *structPath;
    struct arg *next;
  } ARG;

  /**
   * Opcode entry structure.
   *
   * Array Type Format Contract:
   * ---------------------------
   * The outypes and intypes strings use EXTERNAL format for array types:
   *   - Primitive arrays: "k[]", "a[][]", "S[]"
   *   - UDT arrays: ":TypeName;[]"
   *
   * This format is human-readable and consistent with how opcodes are defined.
   * The split_args() function converts external format to internal format
   * (e.g., "k[]" -> "[k]") for runtime processing.
   */
  typedef struct oentry {
    char    *opname;
    size_t  dsblksiz;
    int32_t flags;
    char    *outypes;   /* Output types in EXTERNAL format (e.g., "ak[]") */
    char    *intypes;   /* Input types in EXTERNAL format (e.g., "ik[]") */
    SUBR    init;
    SUBR    perf;
    SUBR    deinit;
    void    *useropinfo; /* user opcode parameters */
    int32_t deprecated;  /* 1: deprecated, 2: renamed; see
                           local CSOUND_DEPRECATED_OPCODE descriptors and
                           docs/opcode-deprecation.md before behavior changes. */
  } OENTRY;


  /**
   *   holds OENTRYs for opcode overloads
   **/
#ifdef _MSC_VER
#  pragma warning(push)
/* This trailing array is extended by the allocation without changing the ABI. */
#  pragma warning(disable: 4200)
#endif
   typedef struct oentries {
      int32_t count;            /* Number of entries in table */
      OENTRY* entries[0];       /* Extended by count entries */
    } OENTRIES;
#ifdef _MSC_VER
#  pragma warning(pop)
#endif


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
    uint32_t        outArgCount;
  } TEXT;


/**
 *  Instrument definition structure
 */
typedef struct instr {
  struct op *nxtop;            /* Linked list of instr opcodes */
  TEXT t;                      /* Text of instrument (same in nxtop) */
  int32_t pmax, vmax, pextrab; /* Arg count, size of data for all
                                  opcodes in instr */
  CS_VAR_POOL *varPool;
  int16 muted;
  int32 opdstot;              /* Total size of opds structs in instr */
  cs_float *psetdata;            /* Used for pset opcode */
  struct insds *instance;     /* Chain of allocated instances of
                                 this instrument */
  struct insds *lst_instance; /* last allocated instance */
  struct insds *act_instance; /* Chain of free (inactive) instances */
  /* (pointer to next one is INSDS.nxtact) */
  struct instr *nxtinstxt; /* Next instrument in orch (num order) */
  int32_t active;          /* To count activations for control */
  int32_t pending_release; /* To count instruments in release phase */
  int32_t maxalloc;
  int32_t turnoff_mode; /* Opt turnoff instruments instances above maxalloc */
  cs_float cpuload;    /* % load this instrumemnt makes */
  struct opcodinfo *opcode_info; /* UDO info (when instrs are UDOs) */
  char *insname;                 /* instrument name */
  int32_t instcnt;               /* Count number of instances ever */
  int32_t isNew;                 /* is this a new definition */
  int32_t nocheckpcnt;           /* Control checks on pcnt */
  int32_t glbvarcnt;             /* global audio var count */
} INSTRTXT;

/**
 * Named instrument list structure
 */
typedef struct namedInstr {
  int32 instno;
  char *name;
  INSTRTXT *ip;
  struct namedInstr *next;
} INSTRNAME;

/**
 * A chain of TEXT structs. Note that this is identical with the first two
 * members of struct INSTRTEXT, and is so typecast at various points in code.
 */
typedef struct op {
  struct op *nxtop;
  TEXT t;
} OPTXT;

/**@}*/
/** @name File and memory lists */
/**@{ */

/**
 * File handle list struct
 */
typedef struct fdch {
  struct fdch *nxtchp;
  void *fd; /* handle returned by csound->FileOpen() */
} FDCH;

/**
 * AUXCH memory list struct
 */
typedef struct auxch {
  struct auxch *nxtchp;
  size_t size;
  void *auxp, *endp;
} AUXCH;

/**  this callback is used to notify the
     availability of new storage in AUXCH *.
     It can be used to swap the old storage
     for the new one and return it for deallocation.
*/
typedef AUXCH *(*aux_cb)(CSOUND *, void *, AUXCH *);

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


/**@}*/
/** @name Standard data type structures */
/**@{ */

 /**
      OCTAVE data
  */
  typedef struct {
    cs_float   *begp, *curp, *endp, feedback[6];
    int32    scount;
  } OCTDAT;


  /**
      DOWNSAMP data
  */
  typedef struct {
    int32    npts, nocts, nsamps;
    cs_float   lofrq, hifrq, looct, srate;
    OCTDAT  octdata[MAXOCTS];
    AUXCH   auxch;
  } DOWNDAT;

  /**
   * Type defitnion for wsigs
   */
  typedef struct {
    uint32_t   ktimstamp, ktimprd;
    int32    npts, nfreqs, dbout;
    DOWNDAT *downsrcp;
    AUXCH   auxch;
  } SPECDAT;


  /**
   * Type definition for arrays
   */
  struct cs_array_storage;
  struct arraydat {
    int32_t      dimensions; /* number of array dimensions */
    int32_t*     sizes;  /* size of each dimensions */
    int32_t      arrayMemberSize; /* size of each item */
    const struct cstype* arrayType; /* type of array */
    cs_float*   data; /* data */
    size_t   allocated; /* size of allocated data */
    /* Opaque ownership sidecar for structured arrays. Appending it preserves
       existing member offsets but changes sizeof(ARRAYDAT), so external code
       embedding the struct must rebuild against this header. Direct ARRAYDAT
       constructors must initialize the field to NULL; ownership changes must
       go through the array type callbacks rather than copying this pointer. */
    struct cs_array_storage* storage;
  };

  /**
   * OPCODE REF type
   */
  typedef struct opcodeRef {
    struct oentries *entries;
    int32_t readonly;
  }  OPCODEREF;

  /**
   * OPCODE type
   */
  typedef struct opcodeObj {
    struct opds *dataspace; // opcode dataspace
    size_t size;            // dataspace size
    cs_float **outargp;        // ptr to the first output arg
    cs_float **inargp;         // ptr to first input arg
    int32_t udo_flag;       // set if opcode is UDO
    int32_t readonly;       // readonly flag
  } OPCODEOBJ;

  /**
   *  Type definition for instr definition ref
   */
  typedef struct instrRef {
    INSTRTXT *instr;
    int32_t   readonly;
  } INSTREF;

#define MAX_STRINGDAT_SIZE 0xFFFFFFFF
  /*
   * Type definition for string data
   */
  struct stringdat {
    char *data;         // null-terminated string
    size_t size;        // total allocated size
    int32_t refcount;   // reference count for shared buffers (0 = unmanaged)
  };

  /**
   * Type definition for complex numbers
   */
  typedef struct complexdat {
    cs_float real;
    cs_float imag;
    int32_t isPolar;
  } COMPLEXDAT;

  /**
  * Type definition for instr instance ref
  */
  typedef struct instanceref {
    struct insds *instance;
    int32_t   readonly;
  } INSTANCEREF;

 /**@}*/
  /** @name Event data */
  /**@{ */

  /**
   * MIDI data structures
   */
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
    cs_float   keylst[1];
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
    cs_float   polyaft[128];
    /** ... with GS vib_rate, stored in c128-c135 */
    cs_float   ctl_val[136];
    /** program change to instr number (<=0: ignore) */
    int16   pgm2ins[128];
    /** channel pressure (0-127) */
    cs_float   aftouch;
    /** pitch bend (-1 to 1) */
    cs_float   pchbend;
    /** pitch bend sensitivity in semitones */
    cs_float   pbensens;
    /** number of held (sustaining) notes */
    int16   ksuscnt;
    /** current state of sustain pedal (0: off) */
    int16   sustaining;
    int32_t dpmsb;
    int32_t dplsb;
    int32_t  datenabl;
    /** chain of dpgm keylists */
    DKLST   *klists;
    /** drumset params         */
    DPARM   *dparms;
  } MCHNBLK;

  typedef struct mevent {
    int16   type;
    int16   chan;
    int16   dat1;
    int16   dat2;
  } MEVENT;

  /**
   * This struct holds the data for one score event.
   */
  typedef struct event {
    /** String argument(s) (NULL if none) */
    int32_t     scnt;
    char    *strarg;
    /* INSDS instance pointer */
    void *pinstance;
    /* suppress ties, add new instance for event */
    int32_t suppress_tie;
    /** Event type */
    char    opcod;
    /** Number of p-fields */
    int32_t  pcnt;
    /** Event start time */
    cs_float   p2orig;
    /** Length */
    cs_float   p3orig;
    /** All p-fields for this event (SSTRCOD: string argument) */
    cs_float   *p; // dynamically-allocated
  } EVTBLK;


    /**@}*/
  /** @name Instrument and Opcode instances */
  /**@{ */

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
    cs_double   offbet;
    /* Time to turn off event, in seconds (negative on indef/tie) */
    cs_double   offtim;
    /* pointer to Csound engine and API for externals */
    CSOUND  *csound;
    uint64_t kcounter;
    cs_float    esr, sicvt, pidsr;                  /* local sr */
    cs_float    onedsr;
    int32_t  in_cvt, out_cvt; /* resampling converter modes for in and out */
    uint32_t ksmps;     /* Instrument copy of ksmps */
    cs_float    ekr;                /* and of rates */

    cs_float    onedksmps, onedkr, kicvt;
    struct opds  *pds;          /* Used for jumping */
    cs_float    scratchpad[4];      /* Persistent data */

    /* user defined opcode I/O buffers */
    void    *opcod_iobufs;
    void    *opcod_deact, *subins_deact;
    uint32_t ksmps_offset; /* ksmps offset for sample accuracy */
    uint32_t no_end;      /* samps left at the end for sample accuracy
                             (calculated) */
    uint32_t ksmps_no_end; /* samps left at the end for sample accuracy
                              (used by opcodes) */
    cs_float   *spin;         /* offset into csound->spin */
    cs_float   *spout;        /* offset into csound->spout, or local spout */
    int32_t  init_done;
    /* Incremented when this instance starts an init or reinit pass. */
    uint64_t init_pass;
    /* Init work can be nested or queued more than once. Only
       instance_init_begin/finish update init_running, under
       async_ref_spinlock. A terminal turnoff remains sticky until the
       outermost finish publishes this instance to the performance-thread
       handoff. free_pending transfers unlinked deletion to that handoff, and
       RECLAIM records that deinit is complete while an async reader exits. */
    volatile int32_t init_running;
    volatile int32_t turnoff_pending;
    volatile int32_t free_pending;
    struct insds *init_turnoff_next;
    int32_t  tieflag;
    int32_t  reinitflag;
    cs_float    retval;
    cs_float   *lclbas;  /* base for variable memory pool */
    char    *strarg;       /* string argument */
    int32_t  linked;  /* linked to instrtxt->act_instance */
    uint64_t instance_id; /* instance id number */
    /* Background users that must finish before this instance is reused or
       reclaimed. Increment only while a lock proves the INSDS is alive. Every
       reuse and free path must reject a nonzero count, and a borrower must not
       access the INSDS after its final decrement. */
    volatile int32_t async_ref_count;
    /* Private named storage. Use Create/QueryInstanceVariable to access it. */
    void *instance_variables;
    /* Copy of required p-field values for quick access */
    CS_VAR_MEM  p0;
    CS_VAR_MEM  p1;
    CS_VAR_MEM  p2;
    CS_VAR_MEM  p3;

  } INSDS;

  /**
   * This struct holds the info for one opcode instance in a concrete
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

  /**
   * label list struct
   */
  typedef struct lblblk {
    OPDS    h;
    OPDS    *prvi;
    OPDS    *prvp;
    OPDS    *prvd;
  } LBLBLK;

  /**@}*/
  /** @name Function tables and GENS */
  /**@{ */

  /**
   * Data for GEN01
   */
  typedef struct {
    cs_float   gen01;
    cs_float   ifilno;
    cs_float   iskptim;
    cs_float   iformat;
    cs_float   channel;
    cs_float   sample_rate;
    char    strarg[SSTRSIZ];
    int32_t deferred_length;
    int32_t deferred_guardreq;
  } GEN01ARGS;


  /**
   * Function table data structure
   */
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
    cs_float   lodiv;
    /** LOFACT * (table_sr / orch_sr), cpscvt = cvtbas / base_freq */
    cs_float   cvtbas, cpscvt;
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
    cs_float   sr;
    /** args  */
    cs_float *args;
    /** arg count */
    int32_t argcnt;
    /** GEN01 parameters */
    GEN01ARGS gen01args;
    /** table data (flen + 1 cs_float values) */
    cs_float   *ftable;
  } FUNC;

  /**
   * Function table event data
   */
  typedef struct {
    CSOUND  *csound;
    int32   flen;
    int32_t fno, guardreq;
    EVTBLK  e;
  } FGDATA;

  /*
   * GEN list structure
   */
  typedef struct {
    char    *name;
    int32_t (*fn)(FGDATA *, FUNC *);
  } NGFENS;

  typedef int32_t (*GEN)(FGDATA *, FUNC *);

  /**@}*/
  /** @name Memory files */
  /**@{ */

  /**
   * Memory file data structure
   */
  typedef struct MEMFIL {
    char    filename[256];      /* Made larger RWD */
    char    *beginp;
    char    *endp;
    int32    length;
    struct MEMFIL *next;
  } MEMFIL;

  /**
   * Sound memory file data structure
   */
  typedef struct SNDMEMFILE_ {
    /** file ID (short name)          */
    char            *name;
    struct SNDMEMFILE_ *nxt;
    /** full path filename            */
    char            *fullName;
    /** file length in sample frames  */
    size_t          nFrames;
    /** sample rate in Hz             */
    cs_double          sampleRate;
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
    cs_double          startOffs;
    /** loop start (sample frames)    */
    cs_double          loopStart;
    /** loop end (sample frames)      */
    cs_double          loopEnd;
    /** base frequency (in Hz)        */
    cs_double          baseFreq;
    /** amplitude scale factor        */
    cs_double          scaleFac;
    /** interleaved sample data       */
    cs_float           data[1];
  } SNDMEMFILE;

/**
 * PVOCEX memory file data structure
 */
typedef struct pvx_memfile_ {
  char *filename;
  struct pvx_memfile_ *nxt;
  float *data;
  uint32 nframes;
  int32_t format;
  int32_t fftsize;
  int32_t overlap;
  int32_t winsize;
  int32_t wintype;
  int32_t chans;
  cs_float srate;
} PVOCEX_MEMFILE;

/**@}*/
/** @name FFT function interface */
/**@{ */

/**
 * New FFT interface
 */
typedef struct _FFT_SETUP {
  int32_t N, M;
  void *setup;
  cs_float *buffer;
  int32_t lib;
  int32_t d;
  int32_t p2;
} CSOUND_FFT_SETUP;

/**@}*/
/** @name Macros to access INSDS/OPDS data from opcodes */
/**@{ */

#define CS_KSMPS (p->h.insdshead->ksmps)
#define CS_KCNT (p->h.insdshead->kcounter)
#define CS_EKR (p->h.insdshead->ekr)
#define CS_ONEDKSMPS (p->h.insdshead->onedksmps)
#define CS_ONEDKR (p->h.insdshead->onedkr)
#define CS_KICVT (p->h.insdshead->kicvt)
#define CS_ONEDDBFS (FL(1.0 / csound->Get0dBFS(csound)))
#define CS_ESR (p->h.insdshead->esr)
#define CS_ONEDSR (p->h.insdshead->onedsr)
#define CS_SICVT (p->h.insdshead->sicvt)
#define CS_TPIDSR (2. * p->h.insdshead->pidsr)
#define CS_PIDSR (p->h.insdshead->pidsr)
#define CS_MPIDSR (-p->h.insdshead->pidsr)
#define CS_MTPIDSR (-2. * p->h.insdshead->pidsr)
#define CS_PDS (p->h.insdshead->pds)
#define CS_SPIN (p->h.insdshead->spin)
#define CS_SPOUT (p->h.insdshead->spout)
#define ORTXT h.optext->t
#define INOCOUNT ORTXT.inArgCount
#define OUTOCOUNT ORTXT.outArgCount
#define CURTIME (((cs_double)csound->icurTimeSamples) / ((cs_double)csound->esr))
#define CURTIME_inc (((cs_double)csound->ksmps) / ((cs_double)csound->esr))

/**@}*/
/** @name Macros to check for arg types */
/**@{ */

#define IS_ASIG_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "a"))
#define IS_STR_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "S"))
#define IS_KSIG_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "k"))
#define IS_INIT_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "i"))
#define IS_FSIG_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "f"))
#define IS_ARRAY_ARG(x) (GetTypeForArg(x) == csound->GetType(csound, "["))

/**@}*/
/** @name Utility inline functions */
/**@{ */

/**
 * Phase modulo-1 for oscillators
 */
static inline cs_float PHMOD1(cs_float p) {
  return p < 0 ? p - (int64_t) (p-1) : p - (uint64_t)p;
}

/**
 * Binary positive power function
 */
static inline cs_double intpow1(cs_double x, int32_t n) {
  cs_double ans = 1.;
  while (n != 0) {
    if (n & 1)
      ans = ans * x;
    n >>= 1;
    x = x * x;
  }
  return ans;
}

  /**
 * Binary power function
 */
static inline cs_double intpow(cs_float x, int32_t n) {
  if (n < 0) {
    n = -n;
    x = 1. / x;
  }
  return intpow1(x, n);
}

/**
 * Byte order check
 */
static inline int32_t byte_order(void) {
  const int32_t one = 1;
  return (!*((char *)&one));
}

  /**
   * checks for string code in p-fields
   */
  static inline int32_t isstrcod(cs_float xx){
#ifdef USE_DOUBLE
    int32_t sel = (byte_order()+1)&1;
    union {
      cs_double d;
      int32_t i[2];
    } z;
    z.d = xx;
    return ((z.i[sel]&0x7ff80000)==0x7ff80000); // was 0x7ff00000 - failing on clang 17!
#else
  union {
    float f;
    int32_t j;
  } z;
  z.f = xx;
  return ((z.j & 0x7f800000) == 0x7f800000);
#endif
}

/**@}*/
/** @name Opcode attributes */
/**@{ */
/**
 * Returns true if argument is a string code
 */
static inline int32_t IsStringCode(cs_float f) {
  return isstrcod(f);
}

/**
 * Returns the number of input arguments for opcode 'p'.
 */
static inline int32_t GetInputArgCnt(OPDS *p) {
  return (int32_t)p->optext->t.inArgCount;
}

/**
 * Returns the name of input argument 'n' (counting from 0) for opcode 'p'.
 */
static inline char *GetInputArgName(OPDS *p, uint32_t n) {
  if (n >= (uint32_t)p->optext->t.inArgCount)
    return (char *)NULL;
  return (char *)p->optext->t.inlist->arg[n];
}

/**
 * Returns the number of output arguments for opcode 'p'.
 */
static inline int32_t GetOutputArgCnt(OPDS *p) {
  return (int32_t)p->optext->t.outArgCount;
}

/**
 * Returns the name of output argument 'n' (counting from 0) for opcode 'p'.
 */
static inline char *GetOutputArgName(OPDS *p, uint32_t n) {
  if (n >= (uint32_t)p->optext->t.outArgCount)
    return (char *)NULL;
  return (char *)p->optext->t.outlist->arg[n];
}

/**
 * Returns the CS_TYPE for an opcode argument argPtr
 */
static inline const CS_TYPE *GetTypeForArg(const void *argPtr) {
  const char *ptr = (const char *)argPtr;
  const CS_TYPE *varType = *(const CS_TYPE *const *)(ptr - CS_VAR_TYPE_OFFSET);
  return varType;
}

/**
 * Returns MIDI channel number (0 to 15) for the instrument instance
 * that called opcode 'p'.
 * In the case of score notes, -1 is returned.
 */
static inline int32_t GetMidiChannelNumber(OPDS *p) {
  MCHNBLK *chn = p->insdshead->m_chnbp;
  return chn != NULL ? chn->channel : -1;
}

/**
 * Returns MIDI note number (in the range 0 to 127) for opcode 'p'.
 * If the opcode was not called from a MIDI activated instrument
 * instance, the return value is undefined.
 */
static inline int32_t GetMidiNoteNumber(OPDS *p) {
  return (int32_t)p->insdshead->m_pitch;
}

/**
 * Returns MIDI velocity (in the range 0 to 127) for opcode 'p'.
 * If the opcode was not called from a MIDI activated instrument
 * instance, the return value is undefined.
 */
static inline int32_t GetMidiVelocity(OPDS *p) {
  return (int32_t)p->insdshead->m_veloc;
}

/**
 * Returns a pointer to the MIDI channel structure for the instrument
 * instance that called opcode 'p'.
 * In the case of score notes, NULL is returned.
 */
static inline MCHNBLK *GetMidiChannel(OPDS *p) {
  return p->insdshead->m_chnbp;
}

/**
 * Returns non-zero if the current note (owning opcode 'p') is releasing.
 */
static inline int32_t GetReleaseFlag(void *p) {
  return (int32_t)((OPDS *)p)->insdshead->relesing;
}

/**
 * Returns the note-off time in seconds (measured from the beginning of
 * performance) of the current instrument instance, from which opcode 'p'
 * was called. The return value may be negative if the note has indefinite
 * duration.
 */
static inline cs_double GetOffTime(OPDS *p) {
  return (cs_double)p->insdshead->offtim;
}

/**
 * Returns the array of p-fields passed to the instrument instance
 * that owns opcode 'p', starting from p0. Only p1, p2, and p3 are
 * guaranteed to be available. p2 is measured in seconds from the
 * beginning of the current section.
 */
static inline CS_VAR_MEM *GetPFields(void *p) {
  return &(((OPDS *)p)->insdshead->p0);
}

/**
 * Returns the instrument number (p1) for opcode 'p'.
 */
static inline int32_t GetInstrumentNumber(OPDS *p) {
  return (int32_t)p->insdshead->p1.value;
}

/**
 * Returns the local ksmps of instrument/UDO containing opcode p.
 * This is an alternative to the macro CS_KSMPS.
 *
 */
static inline uint32_t GetLocalKsmps(OPDS *p) {
  return (uint32_t)  p->insdshead->ksmps;
}

/**
 * Returns the number of samples left at the
 * end of the ksmps block in early sample-accurate
 * exit.
 */
static inline uint32_t GetEarlySmps(OPDS *p) {
  return (uint32_t) p->insdshead->ksmps_no_end;
}

/**
 * Returns the sample-accurate offset at the start
 * of the ksmps block.
 */
static inline uint32_t GetKsmpsOffset(OPDS *p) {
  return (uint32_t) p->insdshead->ksmps_offset;
}

/**
 * Returns the local sr of instrument/UDO containing opcode p.
 * This is an alternative to the macro CS_ESR.
 */
static inline cs_float GetLocalSr(OPDS *p) {
  return p->insdshead->esr;
}

/**
 * Returns the local kr of instrument/UDO containing opcode p.
 * This is an alternative to the macro CS_EKR.
 */
static inline cs_float GetLocalKr(OPDS *p) {
  return p->insdshead->ekr;
}

/**
 * Returns the local kcount of instrument/UDO containing opcode p.
 * This is an alternative to the macro CS_KCOUNTER.
 */
static inline uint64_t GetLocalKcounter(OPDS *p) {
  return p->insdshead->kcounter;
}

/**
 * Returns the opcode name for p.
 */
static inline char *GetOpcodeName(OPDS *p) {
  return p->optext->t.oentry->opname;
}

/**
 * Returns the event type (0 for score/realtine,
 *   1 for MIDI)
 */
static inline int32_t GetEventType(OPDS *p) {
   return p->insdshead->m_chnbp == NULL ? 0 : 1;
}

/**@}*/

static inline char le_test() {
  union _le {
    char c[2];
    short s;
  } le = {{0x0001}};
  return le.c[0];
}

static inline char *byteswap(char *p, int32_t N) {
  if (le_test()) {
    char tmp;
    int32_t j;
    for (j = 0; j < N / 2; j++) {
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
  void (*SetUtilSr)(CSOUND *, cs_float);
  void (*SetUtilNchnls)(CSOUND *, int32_t);
  void *(*SndinGetSetSA)(CSOUND *, char *, void *, cs_float *, cs_float *, cs_float *,
                         int32_t);
  void *(*SndinGetSet)(CSOUND *, void *);
  int32_t (*Sndin)(CSOUND *, void *, cs_float *, int32_t, void *);
} CSOUND_UTIL;

/* The definitions and declarations in this header
   are not part of the API and thus not
   available externally to plugins
*/
#ifdef __BUILDING_LIBCSOUND
#include "cs_internal.h"
#endif

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
#include "csound_structs.h"

struct CSOUND_ {

  /** @name Attributes */
  /**@{ */
  /** Get number of output channels */
  uint32_t (*GetNchnls)(CSOUND *);
  /** Get number of input channels */
  uint32_t (*GetNchnls_i)(CSOUND *);
  /** Get max peak amp */
  cs_float (*Get0dBFS)(CSOUND *);
  /** Get reference tuning */
  cs_float (*GetA4)(CSOUND *);
  /** Get current tie flag */
  int32_t (*GetTieFlag)(CSOUND *);
  /** Get current reinit flag */
  int32_t (*GetReinitFlag)(CSOUND *);
  /** Get current compiled instrument list */
  INSTRTXT **(*GetInstrumentList)(CSOUND *);
  /** Get the max number of strsets */
  int32_t (*GetStrsetsMax)(CSOUND *);
  /** Get a string from Strsets */
  const char *(*GetStrsets)(CSOUND *, int32_t);

  void *(*GetHostData)(CSOUND *);
  int64_t (*GetCurrentTimeSamples)(CSOUND *);
  long (*GetInputBufferSize)(CSOUND *);
  long (*GetOutputBufferSize)(CSOUND *);
  int32_t (*GetDebug)(CSOUND *);
  union {
    int32_t (*GetSizeOfCsFloat)(void);
    int32_t (*GetSizeOfMYFLT)(void); /* CS7 compatibility name. */
  };
  const OPARMS *(*GetOParms)(CSOUND *);
  const char *(*GetEnv)(CSOUND *, const char *name);
  cs_float (*GetSystemSr)(CSOUND *, cs_float);
  /* Get engine sampling rate */
  cs_float   (*GetEngineSr) (CSOUND *csound);
  /* Get engine control rate */
  cs_float   (*GetEngineKr) (CSOUND *csound);
  /* Get engine kcounter value */
  uint64_t (*GetEngineKcounter) (CSOUND *csound);
  /**@}*/

  /** @name Software bus */
  /**@{ */
  int32_t (*GetChannelPtr)(CSOUND *, void **ptr, const char *name,
                           int32_t mode);
  int32_t (*ListChannels)(CSOUND *, controlChannelInfo_t **list);
  /**@}*/

  /** @name Events and Score */
  /**@{ */
  void (*Event)(CSOUND *, int32_t, const cs_float *, int32_t);
  cs_float (*GetScoreOffsetSeconds)(CSOUND *);
  void (*SetScoreOffsetSeconds)(CSOUND *, cs_float);
  void (*RewindScore)(CSOUND *);
  void (*InputMessage)(CSOUND *, const char *message__);
  int32_t (*ReadScore)(CSOUND *, const char *);
  /**@}*/

  /** @name Message printout */
  /**@{ */
  CS_PRINTF2 void (*Message)(CSOUND *, const char *fmt, ...);
  CS_PRINTF3 void (*MessageS)(CSOUND *, int32_t attr, const char *fmt, ...);
  void (*MessageV)(CSOUND *, int32_t attr, const char *format, va_list args);
  int32_t (*GetMessageLevel)(CSOUND *);
  void (*SetMessageLevel)(CSOUND *, int32_t messageLevel);
  void (*SetMessageCallback)(CSOUND *, void (*csoundMessageCallback)(
                                           CSOUND *, int32_t attr,
                                           const char *format, va_list valist));
  /**@}*/

  /** @name Arguments and Types */
  /**@{ */
  char *(*GetArgString)(CSOUND *, cs_float);
  int32 (*StringArg2Insno)(CSOUND *, void *p, int32_t is_string);
  char *(*StringArg2Name)(CSOUND *, char *, void *, const char *, int32_t);
  const CS_TYPE *(*GetType)(CSOUND *csound, const char *type);
  TYPE_POOL *(*GetTypePool)(CSOUND *csound);
  int32_t (*AddVariableType)(CSOUND *csound, TYPE_POOL *pool,
                             const CS_TYPE *typeInstance);

  /**@}*/

  /** @name Memory allocation */
  /**@{ */
  void (*AuxAlloc)(CSOUND *, size_t nbytes, AUXCH *auxchp);
  void (*AuxAllocAligned)(CSOUND *, size_t nbytes, size_t align, AUXCH *auxchp);
  int32_t (*AuxAllocAsync)(CSOUND *, size_t, AUXCH *, AUXASYNC *, aux_cb,
                           void *);
  void *(*Malloc)(CSOUND *, size_t nbytes);
  void *(*Calloc)(CSOUND *, size_t nbytes);
  void *(*CallocAligned)(CSOUND *, size_t nbytes, size_t align);
  void *(*ReAlloc)(CSOUND *, void *oldp, size_t nbytes);
  char *(*Strdup)(CSOUND *, const char *);
  void (*Free)(CSOUND *, void *ptr);
  /**@}*/

  /** @name Function tables */
  /**@{ */
  int32_t (*FTCreate)(CSOUND *, FUNC **, const EVTBLK *, int32_t);
  int32_t (*FTAlloc)(CSOUND *, int32_t tableNum, int32_t len);
  int32_t (*FTDelete)(CSOUND *, int32_t tableNum);
  FUNC *(*FTFind)(CSOUND *, cs_float *argp);
  void *(*GetNamedGens)(CSOUND *);
  int32_t (*GetTableArgs)(CSOUND *, cs_float **, int32_t);
  /**@}*/

  /** @name Instance variables */
  /**@{ */
  /** Create zero-filled named storage local to an instrument instance. */
  int32_t (*CreateInstanceVariable)(CSOUND *, INSDS *owner,
                                     const char *name, size_t nbytes);
  /** Return instance storage, or NULL for a missing name or invalid owner. */
  void *(*QueryInstanceVariable)(CSOUND *, const INSDS *owner, const char *name);
  /**@}*/

  /** @name Global and config variable manipulation */
  /**@{ */
  int32_t (*CreateGlobalVariable)(CSOUND *, const char *name, size_t nbytes);
  void *(*QueryGlobalVariable)(CSOUND *, const char *name);
  void *(*QueryGlobalVariableNoCheck)(CSOUND *, const char *name);
  int32_t (*DestroyGlobalVariable)(CSOUND *, const char *name);
  int32_t (*CreateConfigurationVariable)(CSOUND *, const char *name, void *p,
                                         int32_t type, int32_t flags, void *min,
                                         void *max, const char *shortDesc,
                                         const char *longDesc);
  int32_t (*SetConfigurationVariable)(CSOUND *, const char *name, void *value);
  int32_t (*ParseConfigurationVariable)(CSOUND *, const char *name,
                                        const char *value);
  csCfgVariable_t *(*QueryConfigurationVariable)(CSOUND *, const char *name);
  csCfgVariable_t **(*ListConfigurationVariables)(CSOUND *);
  int32_t (*DeleteConfigurationVariable)(CSOUND *, const char *name);
  const char *(*CfgErrorCodeToString)(int32_t errcode);
  /**@}*/

  /** @name FFT support */
  /**@{ */
  void *(*RealFFTSetup)(CSOUND *csound, int32_t FFTsize, int32_t d);
  void (*RealFFT)(CSOUND *csound, void *p, cs_float *sig);
  cs_float (*GetInverseRealFFTScale)(CSOUND *, int32_t FFTsize);
  void (*ComplexFFT)(CSOUND *, cs_float *buf, int32_t FFTsize);
  void (*InverseComplexFFT)(CSOUND *, cs_float *buf, int32_t FFTsize);
  cs_float (*GetInverseComplexFFTScale)(CSOUND *, int32_t FFTsize);
  void (*RealFFTMult)(CSOUND *, cs_float *outbuf, cs_float *buf1, cs_float *buf2,
                      int32_t FFTsize, cs_float scaleFac);
  void *(*DCTSetup)(CSOUND *csound, int32_t FFTsize, int32_t d);
  void (*DCT)(CSOUND *csound, void *p, cs_float *sig);
  /**@}*/

  /** @name LPC support */
  /**@{ */
  cs_float *(*AutoCorrelation)(CSOUND *, cs_float *, cs_float *, int32_t, cs_float *,
                            int32_t);
  void *(*LPsetup)(CSOUND *csound, int32_t N, int32_t M);
  void (*LPfree)(CSOUND *csound, void *);
  cs_float *(*LPred)(CSOUND *, void *, cs_float *);
  cs_float *(*LPCeps)(CSOUND *, cs_float *, cs_float *, int32_t, int32_t);
  cs_float *(*CepsLP)(CSOUND *, cs_float *, cs_float *, int32_t, int32_t);
  cs_float (*LPrms)(CSOUND *, void *);
  /**@}*/

  /** @name PVOC-EX system */
  /**@{ */
  int32_t (*PVOC_CreateFile)(CSOUND *, const char *, uint32, uint32, uint32,
                             uint32, int32, int32_t, int32_t, float, float *,
                             uint32);
  int32_t (*PVOC_OpenFile)(CSOUND *, const char *, void *, void *);
  int32_t (*PVOC_CloseFile)(CSOUND *, int32_t);
  int32_t (*PVOC_PutFrames)(CSOUND *, int32_t, const float *, int32);
  int32_t (*PVOC_GetFrames)(CSOUND *, int32_t, float *, uint32);
  int32_t (*PVOC_FrameCount)(CSOUND *, int32_t);
  int32_t (*PVOC_fseek)(CSOUND *, int32_t, int32_t);
  const char *(*PVOC_ErrorString)(CSOUND *);
  int32_t (*PVOCEX_LoadFile)(CSOUND *, const char *, PVOCEX_MEMFILE *);
  /**@}*/

  /** @name Error messages
   * These functions do not append a newline to the supplied message.
   * Include line endings in complete diagnostics.
   */
  /**@{ */
  CS_NORETURN CS_PRINTF2 void (*Die)(CSOUND *, const char *msg, ...);
  CS_PRINTF2 int32_t (*InitError)(CSOUND *, const char *msg, ...);
  CS_PRINTF3 int32_t (*PerfError)(CSOUND *, OPDS *h, const char *msg, ...);
  CS_PRINTF2 int32_t (*FtError)(const FGDATA *, const char *, ...);
  CS_PRINTF2 void (*Warning)(CSOUND *, const char *msg, ...);
  CS_PRINTF2 void (*DebugMsg)(CSOUND *, const char *msg, ...);
  CS_NORETURN void (*LongJmp)(CSOUND *, int32_t);
  CS_PRINTF2 void (*ErrorMsg)(CSOUND *, const char *fmt, ...);
  void (*ErrMsgV)(CSOUND *, const char *hdr, const char *fmt, va_list);
  /**@}*/

  /** @name Random numbers */
  /**@{ */
  uint32_t (*GetRandomSeedFromTime)(void);
  void (*SeedRandMT)(CsoundRandMTState *p, const uint32_t *initKey,
                     uint32_t keyLength);
  uint32_t (*RandMT)(CsoundRandMTState *p);
  int32_t (*Rand31)(int32_t *seedVal);
  int32_t *(*RandSeed31)(CSOUND *);
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
  cs_double (*GetRealTime)(RTCLOCK *);
  cs_double (*GetCPUTime)(RTCLOCK *);
  /**@}*/

  /** @name Circular lock-free buffer */
  /**@{ */
  void *(*CreateCircularBuffer)(CSOUND *, int32_t, int32_t);
  int32_t (*ReadCircularBuffer)(CSOUND *, void *, void *, int32_t);
  int32_t (*WriteCircularBuffer)(CSOUND *, void *, const void *, int32_t);
  int32_t (*CheckCircularBuffer)(CSOUND *, void *, int32_t);
  int32_t (*PeekCircularBuffer)(CSOUND *, void *, void *, int32_t);
  int32_t (*GetSizeCircularBuffer)(CSOUND *, void *);
  int32_t (*GetElementSizeCircularBuffer)(CSOUND *, void *);  
  void (*FlushCircularBuffer)(CSOUND *, void *);
  void (*DestroyCircularBuffer)(CSOUND *, void *);
  /**@}*/

  /** @name File access */
  /**@{ */
  char *(*FindInputFile)(CSOUND *, const char *filename, const char *envList);
  char *(*FindOutputFile)(CSOUND *, const char *filename, const char *envList);
  void *(*FileOpen)(CSOUND *, void *, int32_t, const char *, void *,
                    const char *, int32_t, int32_t); /* Rename FileOpen */
  void (*NotifyFileOpened)(CSOUND *, const char *, int32_t, int32_t, int32_t);
  /* closeFlags is CSFILE_CLOSE_SYNC or CSFILE_CLOSE_DEFER. */
  int32_t (*FileClose)(CSOUND *, void *, uint32_t closeFlags);
  const char *(*FileError)(CSOUND *, void *);
  void *(*FileOpenAsync)(CSOUND *, void *, int32_t, const char *, void *,
                         const char *, int32_t, int32_t, int32_t);
  uint32_t (*ReadAsync)(CSOUND *, void *, cs_float *, int32_t);
  uint32_t (*WriteAsync)(CSOUND *, void *, cs_float *, int32_t);
  int32_t (*FSeekAsync)(CSOUND *, void *, int32_t, int32_t);
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
  void *(*SndfileOpenFd)(CSOUND *csound, int32_t fd, int32_t mode,
                         SFLIB_INFO *sfinfo, int32_t close_desc);
  int32_t (*SndfileClose)(CSOUND *csound, void *);
  int64_t (*SndfileWrite)(CSOUND *, void *, cs_float *, int64_t);
  int64_t (*SndfileRead)(CSOUND *, void *, cs_float *, int64_t);
  int64_t (*SndfileWriteSamples)(CSOUND *, void *, cs_float *, int64_t);
  int64_t (*SndfileReadSamples)(CSOUND *, void *, cs_float *, int64_t);
  int64_t (*SndfileSeek)(CSOUND *, void *, int64_t, int32_t);
  int32_t (*SndfileSetString)(CSOUND *csound, void *sndfile, int32_t str_type,
                              const char *str);
  const char *(*SndfileStrError)(CSOUND *csound, void *);
  int32_t (*SndfileCommand)(CSOUND *, void *, int32_t, void *, int32_t);
  /**@}*/

  /** @name Generic callbacks */
  /**@{ */
  int32_t (*Set_KeyCallback)(CSOUND *,
                             int32_t (*func)(void *, void *, uint32_t),
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
    int32_t (*AppendOpcode)(CSOUND *, const char *opname, size_t dsblksiz, int32_t flags,
                            const char *outypes, const char *intypes,
                            int32_t (*init)(CSOUND *, void *),
                            int32_t (*perf)(CSOUND *, void *),
                            int32_t (*deinit)(CSOUND *, void *));
    int32_t (*AppendOpcodes)(CSOUND *, const OENTRY *opcodeList, int32_t n);
    const OENTRY* (*FindOpcode)(CSOUND*, int32_t exact, char*, char* , char*);
    /**@}*/

  /** @name RT audio IO module support */
  /**@{ */
  void (*SetPlayopenCallback)(
      CSOUND *, int32_t (*playopen__)(CSOUND *, const csRtAudioParams *parm));
  void (*SetRtplayCallback)(CSOUND *,
                            void (*rtplay__)(CSOUND *, const cs_float *outBuf,
                                             int32_t nbytes));
  void (*SetRecopenCallback)(CSOUND *,
                             int32_t (*recopen__)(CSOUND *,
                                                  const csRtAudioParams *parm));
  void (*SetRtrecordCallback)(CSOUND *,
                              int32_t (*rtrecord__)(CSOUND *, cs_float *inBuf,
                                                    int32_t nbytes));
  void (*SetRtcloseCallback)(CSOUND *, void (*rtclose__)(CSOUND *));
  void (*SetAudioDeviceListCallback)(
      CSOUND *csound, int32_t (*audiodevlist__)(CSOUND *, CS_AUDIODEVICE *list,
                                                int32_t isOutput));
  void **(*GetRtRecordUserData)(CSOUND *);
  void **(*GetRtPlayUserData)(CSOUND *);
  int32_t (*GetDitherMode)(CSOUND *);
  /**@}*/

  /** @name RT MIDI module support */
  /**@{ */
  void (*SetExternalMidiInOpenCallback)(CSOUND *,
                                        int32_t (*func)(CSOUND *, void **,
                                                        const char *));
  void (*SetExternalMidiReadCallback)(
      CSOUND *, int32_t (*func)(CSOUND *, void *, unsigned char *, int32_t));
  void (*SetExternalMidiInCloseCallback)(CSOUND *,
                                         int32_t (*func)(CSOUND *, void *));
  void (*SetExternalMidiOutOpenCallback)(CSOUND *,
                                         int32_t (*func)(CSOUND *, void **,
                                                         const char *));
  void (*SetExternalMidiWriteCallback)(CSOUND *,
                                       int32_t (*func)(CSOUND *, void *,
                                                       const unsigned char *,
                                                       int32_t));
  void (*SetExternalMidiOutCloseCallback)(CSOUND *,
                                          int32_t (*func)(CSOUND *, void *));
  void (*SetExternalMidiErrorStringCallback)(CSOUND *,
                                             const char *(*func)(int32_t));
  void (*SetMIDIDeviceListCallback)(
      CSOUND *csound, int32_t (*audiodevlist__)(CSOUND *, CS_MIDIDEVICE *list,
                                                int32_t isOutput));
  void (*ModuleListAdd)(CSOUND *, char *, char *);
  /**@}*/
  /** @name MIDI message output support */
  /**@{ */
  /* Send MIDI message to output */
  void (*SendMidiMsg) (CSOUND *csound, int32_t status,
                       int32_t data1, int32_t data2,
                       int32_t port);
  /* Retrieve MIDI out port for last msg sent */
  int32_t (*GetMidiOutPort) (CSOUND *csound);
  /**@}*/
  /** @name Displays & graphs support */
  /**@{ */
  void (*SetDisplay)(CSOUND *, WINDAT *, cs_float *, int32, char *, int32_t,
                     char *);
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
  void (*SetExitGraphCallback)(CSOUND *,
                               int32_t (*exitGraphCallback)(CSOUND *));
  /**@}*/

  /** @name Miscellaneous */
  /**@{ */
  /* access functions used in csound utilities */
  const CSOUND_UTIL *(*GetUtility)(CSOUND *csound);
  /* Fast power of two function from a precomputed table */
  cs_float (*Pow2)(CSOUND *, cs_float a);
  /* String localisation; preserve printf format checking through Str(). */
#if defined(__CUDACC__)
  char *(*LocalizeString)(const char *);
#else
  char *(*LocalizeString)(const char *)__attribute__((format_arg(1)));
#endif
  /* String conversion */
  cs_double (*Strtod)(char *nptr, char **);
  /* String formatted printing */
  int32_t (*Sprintf)(char *str, const char *format, ...);
  /* String formatted scanning */
  int32_t (*Sscanf)(char *str, const char *format, ...);
  /* Set opcode as deprecated */
  int32_t (*Deprecate)(CSOUND *csound, char *name,
                       char *o, char *i, int32_t deprec);
  /**@}*/
  /** @name Arrays & Structs */
  /**@{ */
  int32_t (*ArrayPrepareWrite)(CSOUND *, ARRAYDAT *, INSDS *, int32_t);
  int32_t (*ArrayPrepareOpcodeWrite)(CSOUND *, ARRAYDAT *, OPDS *, int32_t,
                                     const char *);
  int32_t (*ArrayEnsureCapacity)(CSOUND *, ARRAYDAT *, size_t, INSDS *);
  const CS_TYPE *(*RegisterStruct)(CSOUND *, const char *,
                                    const CSOUND_STRUCT_MEMBER *, size_t);
  /**@}*/

  /** @name Placeholders
      To allow the API to grow while maintaining backward binary compatibility.
   */
  /**@{ */
  SUBR dummyfn_2[45];
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

  /* -------- Callback pointers preserved by csoundReset() (between the two markers) -------- */
  SUBR first_callback_;                                      /* start marker of the callback block preserved by csoundReset() */
  channelCallback_t InputChannelCallback_;                   /* software bus input channel callback */
  channelCallback_t OutputChannelCallback_;                  /* software bus output channel callback */
  void (*csoundMessageCallback_)(CSOUND *, int32_t attr, const char *format, va_list args); /* host message callback */
  void (*csoundMakeGraphCallback_)(CSOUND *, WINDAT *windat, const char *name); /* host callback that creates a graph window */
  void (*csoundDrawGraphCallback_)(CSOUND *, WINDAT *windat); /* host callback that draws a graph window */
  void (*csoundKillGraphCallback_)(CSOUND *, WINDAT *windat); /* host callback that closes a graph window */
  int32_t (*csoundExitGraphCallback_)(CSOUND *);             /* host callback returning non-zero while graphs are open */
  void *(*OpenSoundFileCallback_)(CSOUND *, const char *, int32_t, void *); /* host callback that opens a sound file */
  FILE *(*OpenFileCallback_)(CSOUND *, const char *, const char *); /* host callback that opens a generic file */
  void (*FileOpenCallback_)(CSOUND *, const char *, int32_t, int32_t, int32_t); /* host callback notified when a file is opened */
  SUBR last_callback_;                                       /* end marker of the callback block preserved by csoundReset() */

  /* -------- Callback pointers not preserved by csoundReset() -------- */
  int32_t (*playopen_callback)(CSOUND *, const csRtAudioParams *parm); /* host audio output open callback */
  void (*rtplay_callback)(CSOUND *, const cs_float *outBuf, int32_t nbytes); /* host audio output write callback */
  int32_t (*recopen_callback)(CSOUND *, const csRtAudioParams *parm); /* host audio input open callback */
  int32_t (*rtrecord_callback)(CSOUND *, cs_float *inBuf, int32_t nbytes); /* host audio input read callback */
  void (*rtclose_callback)(CSOUND *);                        /* host audio close callback */
  int32_t (*audio_dev_list_callback)(CSOUND *, CS_AUDIODEVICE *, int32_t); /* host callback that enumerates audio devices */
  int32_t (*midi_dev_list_callback)(CSOUND *, CS_MIDIDEVICE *, int32_t); /* host callback that enumerates MIDI devices */
  int32_t (*doCsoundCallback)(CSOUND *, void *, uint32_t);   /* dispatches a registered keyboard/event callback */
  int32_t (*kperf)(CSOUND *);                                /* active k-cycle performance function */
  void (*csoundMessageStringCallback)(CSOUND *csound, int32_t attr, const char *str); /* host callback receiving preformatted messages */
  void (*spinrecv)(CSOUND *);                                /* receives an input audio block */
  void (*spoutran)(CSOUND *);                                /* randomises the output audio block */
  int32_t (*audrecv)(CSOUND *, cs_float *, int32_t);         /* copies audio input samples */
  void (*audtran)(CSOUND *, const cs_float *, int32_t);      /* copies audio output samples */
  void (*debug_cb)(CSOUND *, void *);                        /* per-k-cycle debugger callback */

  /* -------- Engine state, rates and timing -------- */
  void *hostdata;                                            /* opaque host-provided user data */
  ENGINE_STATE engineState;                                  /* engine state merged after compilation */
  char engineStatus;                                         /* engine lifecycle state bitmask */
  uint32_t ksmps;                                            /* samples per control period */
  uint32_t nchnls;                                           /* number of output channels */
  int32_t inchnls;                                           /* number of input channels */
  uint64_t kcounter;                                         /* control-period counter for the current performance */
  uint64_t global_kcounter;                                  /* global control-period counter */
  cs_float esr;                                              /* engine sample rate */
  cs_float ekr;                                              /* engine control rate */
  cs_float onedsr;                                           /* 1 / esr */
  cs_float onedksmps;                                        /* 1 / ksmps */
  cs_float onedkr;                                           /* 1 / ekr */
  cs_float sicvt;                                            /* samples-to-increment conversion factor */
  cs_float kicvt;                                            /* control-to-increment conversion factor */
  cs_float tpidsr;                                           /* 2*pi / esr */
  cs_float pidsr;                                            /* pi / esr */
  cs_float mpidsr;                                           /* -pi / esr */
  cs_float mtpdsr;                                           /* -2*pi / esr */
  cs_float e0dbfs;                                           /* amplitude corresponding to 0 dBFS */
  cs_float dbfs_to_float;                                    /* 1 / e0dbfs */
  cs_double A4;                                              /* reference tuning frequency */
  cs_float _system_sr;                                       /* host/system audio sample rate */
  cs_float csoundScoreOffsetSeconds_;                        /* score time offset in seconds */
  int32_t reinitflag;                                        /* current reinit flag */
  int32_t tieflag;                                           /* current tie flag */
  int64_t icurTimeSamples;                                   /* current time in samples */
  cs_double curTime_inc;                                     /* time increment per control period */
  cs_double timeOffs;                                        /* start time offset of the current section */
  cs_double beatOffs;                                        /* start beat offset of the current section */
  cs_double curBeat;                                         /* current time in beats */
  cs_double curBeat_inc;                                     /* beat increment per control period */
  int64_t ibeatTime;                                         /* beat time in samples */
  cs_double prvbt;                                           /* previous beat time */
  cs_double curbt;                                           /* current beat time */
  cs_double nxtbt;                                           /* next beat time */
  cs_double curp2;                                           /* current event p2 time */
  cs_double nxtim;                                           /* next score event time */
  int64_t cyclesRemaining;                                   /* performance cycles remaining */
  int64_t advanceCnt;                                        /* events to process before advancing time */
  int32_t sampsNeeded;                                       /* samples needed for the next control period */
  int32_t initonly;                                          /* initialisation-only run flag */
  cs_float cpu_power_busy;                                   /* measured CPU load */
  jmp_buf exitjmp;                                           /* setjmp target for fatal-error unwinding */

  /* -------- Instruments, opcodes and modules -------- */
  OPDS *ids;                                                 /* current init opcode chain */
  INSDS *curip;                                              /* currently running instrument instance */
  INSTRTXT *instr0;                                          /* compiled global instrument 0 */
  INSTRTXT **dead_instr_pool;                                /* pool of deactivated instrument definitions */
  int32_t dead_instr_no;                                     /* number of pooled deactivated instruments */
  INSDS *frstoff;                                            /* first deactivated instrument instance */
  INSDS actanchor;                                           /* anchor of the active instrument instance list */
  TYPE_POOL *typePool;                                       /* variable type pool */
  CS_HASH_TABLE *opcodes;                                    /* opcode hash table */
  OPCODINFO *opcodeInfo;                                     /* opcode metadata chain */
  int32 nrecs;                                               /* number of recorded opcode definitions */
  void *csmodule_db;                                         /* loaded plugin module database */
  char *dl_opcodes_oplibs;                                   /* opcode libraries requested for loading */
  int32_t opcodedirWasOK;                                    /* plugin directory scan status */
  void *utility_db;                                          /* utility database */
  int32_t modules_loaded;                                    /* number of loaded modules */
  int32_t default_modules_loaded;                            /* whether the default plugin directories were scanned */
  int32_t use_only_orchfile;                                 /* ignore the score file */
  char orcname_mode;                                         /* orchestra name handling mode */

  /* -------- Function tables, generators and lookup tables -------- */
  FUNC **flist;                                              /* function table list */
  int32_t maxfnum;                                           /* highest allocated function table number */
  GEN *gensub;                                               /* user-defined gen routine chain */
  int32_t genmax;                                            /* highest generator number */
  void *namedgen;                                            /* named generator table */
  int32_t genlabs;                                           /* number of gen slots */
  FUNC *sinetable;                                           /* built-in sine table */
  int32_t sinelength;                                        /* sine table length */
  cs_float *cpsocfrc;                                        /* cps/oct conversion lookup table */
  cs_float *logbase2;                                        /* log2 lookup table */
  int16 *isintab;                                            /* is-integer lookup table */
  int32_t FFT_max_size;                                      /* largest FFT size */
  void *FFT_table_1;                                         /* cached FFT setup table 1 */
  void *FFT_table_2;                                         /* cached FFT setup table 2 */
  void *tseg;                                                /* time-warping segment pointer */
  void *tpsave;                                              /* time-warping saved segment pointer */
  int32 revlpsum;                                            /* reverse low-pass accumulation */

  /* -------- Score, events and parsing -------- */
  char *orchname;                                            /* orchestra file name */
  char *scorename;                                           /* score file name */
  int32_t commandLineArgCount;                               /* number of command-line arguments */
  char **commandLineArgs;                                    /* command-line arguments */
  CORFIL *orchstr;                                           /* orchestra source corfile */
  CORFIL *scorestr;                                          /* score source corfile */
  CORFIL *scstr;                                             /* score reader corfile */
  CORFIL *expanded_orc;                                      /* preprocessed orchestra source */
  CORFIL *expanded_sco;                                      /* preprocessed score source */
  CORFIL *playscore;                                         /* realtime score playback corfile */
  FILE *scoreout;                                            /* score output file */
  EVTBLK *currevent;                                         /* current event block */
  EVTBLK evt;                                                /* working event block */
  EVTBLK *init_event;                                        /* event used during initialisation */
  SRTBLK *frstbp;                                            /* first score block */
  NAMES *omacros;                                            /* orchestra macro table */
  NAMES *smacros;                                            /* score macro table */
  MACRO *orc_macros;                                         /* persistent orchestra macros */
  int32_t sectcnt;                                           /* score section count */
  int32_t inerrcnt;                                          /* initialisation error count */
  int32_t synterrcnt;                                        /* syntax error count */
  int32_t perferrcnt;                                        /* performance error count */
  int32_t total_assert_cnt;                                  /* total number of unit-test assertions */
  int32_t warped;                                            /* score warping active flag */
  int32_t jumpset;                                           /* setjmp target is active */
  int32_t inZero;                                            /* compiling or running global instrument 0 */
  int32_t orcLineOffset;                                     /* lines before the orchestra in the CSD */
  int32_t scoLineOffset;                                     /* lines before the score in the CSD */
  char *csdname;                                             /* original CSD file name (not freed) */
  char *xfilename;                                           /* CSD file name */
  int32_t score_parser;                                      /* selected score parser */
  char *score_srt;                                           /* sorted score text */
  char *op;                                                  /* current command-line option */
  int32_t mode;                                              /* current operating mode */
  int32_t print_version;                                     /* print version on startup flag */
  char *opcodedir;                                           /* opcode search directory */
  int32_t aftouch;                                           /* default MIDI touch value */
  int32_t keep_tmp;                                          /* keep temporary files */
  uint32_t tempStatus;                                       /* temporary-file tracking status */
  void *evtFuncChain;                                        /* realtime event function chain */
  EVTNODE *OrcTrigEvts;                                      /* events triggered by the orchestra awaiting start */
  EVTNODE *freeEvtNodes;                                     /* free list of event nodes */
  int32_t csoundIsScorePending_;                             /* score events pending flag */
  int32_t Mforcdecs;                                         /* score force-decrement flag */
  int32_t Mxtroffs;                                          /* score turn-off offset flag */
  int32_t MTrkend;                                           /* score end-of-track flag */
  int32 rngcnt[MAXCHNLS];                                    /* per-channel random-number counters */
  int16 rngflg;                                              /* per-channel random-number flags */
  int16 multichan;                                           /* multichannel score playback flag */

  /* -------- Global variables, configuration and strings -------- */
  CS_HASH_TABLE *namedGlobals;                               /* named global variable table */
  CS_HASH_TABLE *cfgVariableDB;                              /* configuration variable table */
  CS_HASH_TABLE *envVarDB;                                   /* environment variable table */
  CS_HASH_TABLE *chn_db;                                     /* channel database */
  int32_t strsmax;                                           /* maximum number of strset variables */
  char **strsets;                                            /* strset strings */
  int32_t scnt;                                              /* string count */
  int32_t strsiz;                                            /* current string space size */
  int32_t sstrlen;                                           /* string buffer length */
  char *sstrbuf;                                             /* string buffer */
  OPARMS *oparms;                                            /* active option set */
  OPARMS oparms_;                                            /* option storage */
  int32_t disable_csd_options;                               /* ignore options embedded in the CSD */
  int32_t options_checked;                                   /* command-line options checked flag */
  int32_t inChar_;                                           /* next input character */
  int32_t enableMsgAttr;                                     /* message attributes enabled */
  int32_t info_message_request;                              /* information messages requested */

  /* -------- Realtime audio I/O and buffers -------- */
  cs_float *spin;                                            /* input channel buffer */
  cs_float *spout;                                           /* output channel buffer */
  cs_float *spout_tmp;                                       /* scratch output channel buffer */
  int32_t nspin;                                             /* number of channels in spin */
  int32_t nspout;                                            /* number of channels in spout */
  cs_float *auxspin;                                         /* auxiliary input buffer */
  void *rtRecord_userdata;                                   /* host audio input callback user data */
  void *rtPlay_userdata;                                     /* host audio output callback user data */
  int32_t realtime_audio_flag;                               /* realtime audio active flag */
  int32_t dither_output;                                     /* dither output flag */
  int32_t enableHostImplementedAudioIO;                      /* host provides audio I/O */
  int32_t enableHostImplementedMIDIIO;                       /* host provides MIDI I/O */
  int32_t hostRequestedBufferSize;                           /* host-requested audio buffer size */
  char stdin_assign_flg;                                     /* standard input redirection flag */
  char stdout_assign_flg;                                    /* standard output redirection flag */
  int32_t io_initialised;                                    /* I/O subsystem initialised */
  REMOT_BUF SVrecvbuf;                                       /* realtime event input communications buffer */
  void *remoteGlobals;                                       /* remote control globals */

  /* -------- MIDI -------- */
  MCHNBLK *m_chnbp[MIDIMAXPORTS * 16];                       /* MIDI channel blocks */
  MGLOBAL *midiGlobals;                                      /* MIDI global state */
  int32_t midi_clock_pulse;                                  /* MIDI clock pulse flag */
  int32_t midi_start;                                        /* MIDI start message flag */
  int32_t midi_continue;                                     /* MIDI continue message flag */
  int32_t midi_stop;                                         /* MIDI stop message flag */
  int32_t midiout_port;                                      /* current MIDI output port */

  /* -------- File I/O -------- */
  FILE *Linepipe;                                            /* line input pipe */
  int32_t Linefd;                                            /* line input file descriptor */
  void *file_io_thread;                                      /* asynchronous file I/O thread */
  int32_t file_io_start;                                     /* asynchronous file I/O running flag */
  void *file_io_threadlock;                                  /* asynchronous file I/O lock */
  uint64_t file_io_pass;                                     /* asynchronous file I/O generation counter */
  void *open_files;                                          /* list of open files */
  void *retired_files;                                       /* files awaiting asynchronous reclamation */
  void *pvFileTable;                                         /* PVOC file table */
  int32_t pvNumFiles;                                        /* number of open PVOC files */
  int32_t pvErrorCode;                                       /* last PVOC error code */
  MEMFIL *memfiles;                                          /* memory file list */
  PVOCEX_MEMFILE *pvx_memfiles;                              /* PVOC-EX memory files */
  CS_HASH_TABLE *sndmemfiles;                                /* sound memory file hash table */
  void *diskin2_async_state;                                 /* diskin2 asynchronous read state */
  char *filedir[256];                                        /* file search directory list */

  /* -------- Memory management -------- */
  void *memalloc_db;                                         /* memory allocation database */
  ALLOC_DATA *alloc_queue;                                   /* queue of allocations pending free */
  volatile unsigned long alloc_queue_items;                  /* number of entries in the allocation queue */
  unsigned long alloc_queue_active;                          /* allocations currently active */
  unsigned long alloc_queue_wp;                              /* allocation queue write pointer */
  void *searchPathCache;                                     /* cached file search paths */
  void *reset_list;                                          /* registered reset callback list */
  void *csoundCallbacks_;                                    /* registered event callback list */

  /* -------- Locks and synchronisation -------- */
  void *API_lock;                                            /* serialises public API entry points */
  void *array_storage_lock;                                  /* structured array storage lock */
  spin_lock_t array_storage_spinlock;                        /* structured array sidecar spinlock */
  spin_lock_t spoutlock;                                     /* output buffer spinlock */
  spin_lock_t spinlock;                                      /* input buffer spinlock */
  spin_lock_t memlock;                                       /* memory database spinlock */
  spin_lock_t spinlock1;                                     /* general-purpose spinlock */
  spin_lock_t open_files_lock;                               /* open file list spinlock */
  spin_lock_t alloc_queue_spinlock;                          /* allocation queue spinlock */
  spin_lock_t alloc_spinlock;                                /* allocation spinlock */
  spin_lock_t instance_spinlock;                             /* instrument instance registry spinlock */
  spin_lock_t async_ref_spinlock;                            /* asynchronous reference registry spinlock */
  spin_lock_t rt_event_spinlock;                             /* realtime event registry spinlock */
  spin_lock_t diskin2_async_lock;                            /* diskin2 asynchronous read lock */
  spin_lock_t osc_spinlock;                                  /* OSC message spinlock */
  int32_t realtime_locks_initialized;                        /* realtime locks initialised flag */

  /* -------- Threads, parallel execution and DAG -------- */
  void *event_insert_thread;                                 /* realtime event insertion thread */
  int32_t event_insert_loop;                                 /* realtime event insertion loop flag */
  void *init_pass_threadlock;                                /* initialisation pass lock */
  int32_t multiThreadedComplete;                             /* parallel performance complete flag */
  THREADINFO *multiThreadedThreadInfo;                       /* parallel worker thread information */
  struct dag_t *multiThreadedDag;                            /* parallel execution graph */
  void *barrier1;                                            /* parallel execution barrier 1 */
  void *barrier2;                                            /* parallel execution barrier 2 */
  struct instr_semantics_t *instCurr;                        /* instrument currently under semantic analysis */
  struct instr_semantics_t *instRoot;                        /* root of the instrument semantic tree */
  int32_t inInstr;                                           /* inside an instrument during semantic analysis */
  int32_t dag_changed;                                       /* execution graph changed flag */
  int32_t dag_num_active;                                    /* number of active graph nodes */
  INSDS **dag_task_map;                                      /* graph node to instrument instance map */
  volatile stateWithPadding *dag_task_status;                /* graph node status array */
  watchList *volatile *dag_task_watch;                       /* graph node watch lists */
  watchList *dag_wlmm;                                       /* graph watch-list free list */
  char **dag_task_dep;                                       /* graph task dependencies */
  int32_t dag_task_max_size;                                 /* maximum graph size */
  INSDS *init_turnoff_pending;                               /* instances pending turn-off at initialisation */
  int32_t parflag;                                           /* parallel performance active flag */
  int32_t *taskflag;                                         /* per-task graph status flags */

  /* -------- Message output -------- */
  void *message_buffer;                                      /* buffered message storage */
  char *message_string;                                      /* reusable message string */
  volatile unsigned long message_string_queue_items;         /* pending entries in the message string queue */
  unsigned long message_string_queue_wp;                     /* message string queue write pointer */
  message_string_queue_t *message_string_queue;              /* message string queue */
  struct _message_queue **msg_queue;                         /* realtime message queue */
  volatile long msg_queue_wget;                              /* realtime message queue read index */
  volatile long msg_queue_wput;                              /* realtime message queue write index */
  volatile long msg_queue_rstart;                            /* realtime message queue start index */
  volatile long msg_queue_items;                             /* realtime message queue entry count */
  char *delayederrormessages;                                /* queued error messages */
  void *printerrormessagesflag;                              /* deferred error printing flag */
  OSC_MESS osc_message_anchor;                               /* anchored OSC message storage */

  /* -------- Random number generators -------- */
  int32_t randSeed1;                                         /* legacy random seed 1 */
  int32_t randSeed2;                                         /* legacy random seed 2 */
  CsoundRandMTState *csRandState;                            /* active Mersenne-Twister state */
  CsoundRandMTState randState_;                              /* Mersenne-Twister state storage */
  int32_t ugens4_rand_16;                                    /* ugens4 16-bit random state */
  int32_t ugens4_rand_15;                                    /* ugens4 15-bit random state */
  cs_double rndfrac;                                         /* random fraction accumulator */

  /* -------- Display and graphs -------- */
  int32_t isGraphable_;                                      /* graph display enabled */
  cs_float *disprep_fftcoefs;                                /* FFT coefficients for display */
  void *winEPS_globals;                                      /* winEPS display globals */

  /* -------- Sound file metadata and analysis -------- */
  cs_float maxamp[MAXCHNLS];                                 /* maximum output amplitude per channel */
  cs_float smaxamp[MAXCHNLS];                                /* maximum input amplitude per channel */
  cs_float omaxamp[MAXCHNLS];                                /* previous maximum output amplitude per channel */
  uint32 maxpos[MAXCHNLS];                                   /* sample position of maximum output */
  uint32 smaxpos[MAXCHNLS];                                  /* sample position of maximum input */
  uint32 omaxpos[MAXCHNLS];                                  /* sample position of previous maximum output */
  int32_t peakchunks;                                        /* peak analysis chunk count */
  char *SF_csd_licence;                                      /* sound file licence metadata */
  char *SF_id_title;                                         /* sound file title metadata */
  char *SF_id_copyright;                                     /* sound file copyright metadata */
  int32_t SF_id_scopyright;                                  /* sound file copyright flag */
  char *SF_id_software;                                      /* sound file software metadata */
  char *SF_id_artist;                                        /* sound file artist metadata */
  char *SF_id_comment;                                       /* sound file comment metadata */
  char *SF_id_date;                                          /* sound file date metadata */

  /* -------- Sub-module static state and API bookkeeping -------- */
  struct sread__ sread;                                      /* score file reader static state */
  struct onefileStatics__ onefileStatics;                    /* single-file opcode static state */
  struct lineventStatics__ lineventStatics;                  /* line event static state */
  struct musmonStatics__ musmonStatics;                      /* score playback static state */
  struct libsndStatics__ libsndStatics;                      /* libsndfile static state */
  RTCLOCK *csRtClock;                                        /* realtime clock */
  void *csdebug_data;                                        /* debugger data */
  void *debug_cb_data;                                       /* debugger callback user data */
  CSOUND_UTIL csound_util;                                   /* utility access table */
  uint64_t instance_count;                                   /* number of live CSOUND instances */
  readlineCallback_t readlineCallback;                       /* host readline callback */
  void *readlineUserData;                                    /* readline callback user data */
  uint32_t readlineRequestId;                                /* readline request identifier */
  /*struct CSOUND_ **self;*/
  /**@}*/
#endif /* __BUILDING_LIBCSOUND */
};

/*
 * Move the C++ guards to enclose the entire file,
 * in order to enable C++ to #include this file.
 */

#define LINKAGE_BUILTIN(name)                                                  \
  int32_t name##_init(CSOUND *csound, OENTRY **ep) {                           \
    (void)csound;                                                              \
    *ep = name;                                                                \
    return (long)(sizeof(name));                                               \
  }

#define FLINKAGE_BUILTIN(name)                                                 \
  NGFENS *name##_init(CSOUND *csound) {                                        \
    (void)csound;                                                              \
    return name;                                                               \
  }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CSOUNDCORE_H */
