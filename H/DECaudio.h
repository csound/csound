/*
    DECaudio.h:

    Copyright (C) 1991 Dan Ellis

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

/*******************************************************\
*       DECaudio.h                                      *
*  all the header files needed to drive DEC's LoFi      *
*  collected by dpwe 05oct90                            *
\*******************************************************/

#include <time.h>

/******* was LoFi.h ***********/

#define RPHYSICAL       0
#define RRAM    1
#define RROM    2
#define RCSR    3
#define RCODEC0 4
#define RCODEC1 5
#define ROPTION 6
#define RHOST   7

#define FHS     0
#define FCA     1
#define FEA     2
#define FED     3
#define FMD     4
#define FIE     5
#define FGC     6

extern  struct lofi_reg *LoFiOpen();
extern  unsigned long *LoFiMap();
extern  int     LoFiRead();
extern  int     LoFiWrite();
extern  void    LoFiPrintCSR();
extern  void    LoFiSetCSR();
extern  void    LoFiTestRam();
extern  void    LoFiPrintEvent();
extern  void    LoFiReadBlock();
extern  void    LoFiWriteBlock();


/****** was LoFiMap.h *******/

#define RPHYSICAL       0
#define RRAM    1
#define RROM    2
#define RCSR    3
#define RCODEC0 4
#define RCODEC1 5
#define ROPTION 6
#define RHOST   7

#define FHS     0
#define FCA     1
#define FEA     2
#define FED     3
#define FMD     4
#define FIE     5
#define FGC     6

extern  struct lofi_reg *LoFiOpen();
extern  void    LoFiClose();    /* added dpwe 08sep90 */
extern  unsigned long *LoFiMap();
extern  int     LoFiRead();
extern  int     LoFiWrite();
extern  void    LoFiPrintCSR();
extern  void    LoFiSetCSR();
extern  void    LoFiTestRam();
extern  void    LoFiPrintEvent();
extern  void    LoFiReadBlock();
extern  void    LoFiWriteBlock();


/******** was lofi.h ********/


/************************************************************************
 *                                                                      *
 *                      Copyright (c) 1990 by                           *
 *              Digital Equipment Corporation, Maynard, MA              *
 *                      All rights reserved.                            *
 *                                                                      *
 *   This software is furnished under a license and may be used and     *
 *   copied  only  in accordance with the terms of such license and     *
 *   with the  inclusion  of  the  above  copyright  notice.   This     *
 *   software  or  any  other copies thereof may not be provided or     *
 *   otherwise made available to any other person.  No title to and     *
 *   ownership of the software is hereby transferred.                   *
 *                                                                      *
 *   The information in this software is subject to change  without     *
 *   notice  and should not be construed as a commitment by Digital     *
 *   Equipment Corporation.                                             *
 *                                                                      *
 *   Digital assumes no responsibility for the use  or  reliability     *
 *   of its software on equipment which is not supplied by Digital.     *
 *                                                                      *
 ************************************************************************/

/************************************************************************
 * Modification History
 *
 *
 *      7/17/90
 *         prototype lofi driver hacked by Rich Hyde
 *
 *
 ************************************************************************/

/*
 *
 *  These need to be mapped into user space.
 *
 */

#ifndef HAVE_VOLATILE
#define volatile
#endif

typedef  enum {
          DSP = 1,
          RT = 2,
          TLI_RING = 3,
          TLI_DTMF = 4,
          TLI_LOOP = 5,
          OPTION_INTR = 6,
          UNKNOWN = 7
    } EventType;

struct  interrupt_event{
        EventType type;         /*  event type */
        long status;            /* value of the status register */
        struct timeval time;    /* Systems notion of time when it occured */
        long    dsptime;        /* DSP's notion of time when it occurred */
        long seq;               /* sequence number of the event */
        union {                 /* Additional data by type. */
                char dsp_data[8];
                struct codec_intr {
                        char master_ir;
                        char master_rx;
                        char slave_ir;
                        char slave_rx;
                } codec_data;
        } data;
};

struct lofi_info {
   int flag;                    /* Is open boolean */
   int event_size;              /* size of the events themselves */
   int event_list_size; /* Size of the event queue */
   struct interrupt_event *ks_start; /* Head of the queue in kernel space */
   struct interrupt_event *us_start; /* Head of the queue in user space */
   volatile int head;           /* head of circular list        */
   volatile int tail;           /* tail of circular list        */
   long last_seq;               /* last sequence number used    */
   struct lofi_reg *ks_reg;     /* addresses of the lofi option space in ks */
   struct lofi_reg *us_reg;     /*  addresses of the lofi option space in us */
   void *rsel;                  /* select address */
   long old_rd_csr;             /* old csr */
};
#define LOFI_DEBUG( who, level) ((lofi_debug & who) && ((0xff&lofi_debug) > level))
#define LOFI_INTR       (0x100)
#define LOFI_CONF       (0x1000)
#define LOFI_BUF        (0x2000)
#define LOFI_OTHER      (0x4000)

#define DEV_LOFI_STR "LoFi  "


/******** was lofi_reg.h ********/


/*
I believe this is a coherent view of the lo-fi memory
map based at one of the 1 MB boundaries.  Please
feel free to clean up the structures should you desire to.

I think the event queue structure should also include
the dsp's view of the current time.  24 bits is not enough and
48 is a little excessive.   32 bits accounts for about 6 days
if memory serves me correctly.

The driver should read two SRAM locations (stored as 24 lsb in low
and 8 msb in high SRAM address) and extract the 32 bit timestamp and
store into a longword in the event queue.  These locations will
be maintained by the DSP.  If it is not running, then time will
not progress, but that is OK.

To maintain a consistent view of time, the reader (device driver)
should read the timestamp according to Leslie Lamport's algorithm
in SRC report 27.

Briefly,

l1 and l2 are copies of the low order bits.  In this implementation
l1 and l2 are the low order 24 bits.
r contains the high order 8 bits in this implementation.

The reader performs the following sequence of instructions:

v1 := l1;
w  := r;
v2 := l2;

if v1 = v2 then return CombineTime(v2,w)
else            return CombineTime(v2,0)
fi

This is only slightly complicated by the bit shuffling that must
be done and the masking of the low order 8 bits on the each read.
CombineTime extracts the appropriate 32 bits from the 48/64 bits
read.
*/


#define NBREGION        (256*1024)

#define NWROM           (32*1024)
#define NRCOPIES        (NBREGION/NWROM/4)

typedef struct ROM {
    struct {
        volatile unsigned char romd;
        volatile unsigned char u0;
        volatile unsigned char u1;
        volatile unsigned char u2;
    } rcopy[NRCOPIES][NWROM];
} ROM;

#define NWHOST          (4*1024)
#define NHCOPIES        (NBREGION/NWHOST/4)
#define NWHBLOCK        (NWHOST/NHCOPIES)

typedef struct HBLOCK {
    struct {
        volatile unsigned char hd;
        volatile unsigned char u0;
        volatile unsigned char u1;
        volatile unsigned char u2;
    } hbcopy[NWHBLOCK];
} HBLOCK;

typedef struct HOST {
    struct {
        HBLOCK      h_wr[8];
        HBLOCK      h_rr[8];
    } hcopy[NHCOPIES];
} HOST;

#define NWSRAM          (32*1024)
#define NSRCOPIES       (NBREGION/NWSRAM/4)

typedef struct SRAM {
    union {
        volatile unsigned long  wd[NWSRAM];
        struct {
            volatile unsigned char   u0;
            volatile unsigned char   bd0;
            volatile unsigned char   bd1;
            volatile unsigned char   bd2;
        } bd[NWSRAM];
    } srcopy[NSRCOPIES];
} SRAM;

#define NWIO            (256*1024)
#define NIO             (NBREGION/NWIO/4)
#define NUSRAM          ((32+26)*1024)
#define NCSR            (2*1024)
#define NOPTION         (2*1024)

/*
 * Codec register offsets
 */
#define CR_OFFS         0               /* Write only */
#define IR_OFFS         CR_OFFS         /* Read only */
#define DR_OFFS         1               /* 2 register one RO one WO */
#define DSR1_OFFS       2               /* readonly */
#define DER_OFFS        3               /* readonly 2 byte fifo */
#define DCTB_OFFS       4               /* writeonly 8 byte fifo */
#define DCRB_OFFS       DCTB_OFFS       /* Readonly 8 byte fifo */
#define BBTB_OFFS       5               /* writeonly */
#define BBRB_OFFS       BBTB_OFFS       /* readonly */
#define BCTB_OFFS       6               /* writeonly */
#define BCRB_OFFS       BCTB_OFFS       /* readonly */
#define DSR2_OFFS       7               /* readonly */

typedef struct IO {
    volatile unsigned long    usram[NUSRAM];
    volatile unsigned long    csr[NCSR];
    volatile unsigned long    option[NOPTION];
    volatile unsigned long    codecs[1];
} IO;

/*
 * This structure overlays the lowest 1 MB of Lo-Fi option memory.
 */
struct lofi_reg {
    ROM         io_rom;
    HOST        io_host;
    SRAM        io_sram;
    IO          io_io;
} ;

#define rom_map(x)      io_rom.rcopy[0][(x)]

#define rd_host(rx)     io_host.hcopy[0].h_rr[(rx)].hbcopy[0].hd
#define wr_host(rx)     io_host.hcopy[0].h_wr[(rx)].hbcopy[0].hd

#define sram_map(x)     io_sram.srcopy[0].wd[(x)]

#define rd_csr          io_io.csr[0]
#define wr_csr          io_io.csr[0]

#define C0BASE          0x0000
#define C1BASE          0x0008

#define codec0(rx)      io_io.codecs[C0BASE + ((rx) << 2)]
#define codec1(rx)      io_io.codecs[C1BASE + ((rx) << 2)]

#define woption(x)      io_io.option[(x)]

/*
 * Rich's bit defs.
 */
#define CODEC1_INTR_NOT_DEF     (1<<31)
#define CODEC0_INTR_NOT_DEF     (1<<30)
#define FRAME_INTR_ENABLE_DEF   (1<<29)
#define TLI_INTR_ENABLE_DEF     (1<<28)
#define RING_STATUS_DEF (1<<27)
#define LOOP_CURRENT_DEF        (1<<26)
#define DTMF_STATUS_DEF (1<<25)
#define DTMF_VALID_DEF  (1<<20)
#define DSP_HOST_INTR_NOT_DEF   (1<<19)
#define OPTION_STATUS_DEF       (1<<18)

#define BASIC_INTR_MASK (DTMF_STATUS_DEF)
#define NOT_INTR_MASK (DSP_HOST_INTR_NOT_DEF|OPTION_STATUS_DEF|CODEC1_INTR_NOT_DEF|CODEC0_INTR_NOT_DEF)
#define CHANGE_INTR_MASK (RING_STATUS_DEF|LOOP_CURRENT_DEF)
#define DTMF_KEY(s)     (((s)>>20)0x0f)
#define DSP_HOST_INTR(s)        (((s) & DSP_HOST_INTR_NOT_DEF)== 0)
#define FRAME_INTR(s)   ((s) & FRAME_INTR_ENABLE_DEF)
#define RING_INTR(s)    ((s) & RING_STATUS_DEF)
#define DTMF_INTR(s)    ((s) & DTMF_STATUS_DEF)
#define CURRENT_LOOP_INTR(s)    ((s) &LOOP_CURRENT_DEF)
#define OPTIONS_INTR(s) ((s) & OPTION_STATUS_DEF)
#define QIOLOFIINFO     _IOR('a', 1, struct lofi_info *)
#define LOFI_REG_OFFSET (0)

/* assume low to hi bit order, not portable */
struct lofi_status_reg {
        unsigned int s_u0 : 14;
        unsigned int s_ed : 1;
        unsigned int s_hs : 1;
        unsigned int s_ea : 1;
        unsigned int s_dm : 1;
        unsigned int s_optstat_l : 1;
        unsigned int s_hoststat_l : 1;
        unsigned int s_dtmfvalid : 1;
        unsigned int s_dtmf : 4;
        unsigned int s_dtmfstat : 1;
        unsigned int s_lcdstat : 1;
        unsigned int s_ringstat : 1;
        unsigned int s_et : 1;
        unsigned int s_ef : 1;
        unsigned int s_codec0stat_l : 1;
        unsigned int s_codec1stat_l : 1;
} ;

/******** was hwddt.h ********/


#define MIN(x,y)        ((x) > (y) ? (y) : (x));
#define TABLEN  100
#define NAMELEN 256


#define BASE10  0
#define BASE16  1
#define BASE2   2


typedef struct SYM {
    struct SYM      *link;
    int     token;
    char    name[NAMELEN];
} SYM;


extern  void    InitSymbolTable();
extern  void    StoreKeywords();
extern  SYM     *StoreSearch();

extern  void    soutput();
extern  void    poutput();
extern  void    msg();

extern  void    dspLoad();

extern  void    fatal();
extern  void    usage();

