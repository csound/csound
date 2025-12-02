/*
  coreDefs.h: core Csound definitions

  Copyright (C) 1991-2024 

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

#ifndef _CORE_DEFS_H_
#define _CORE_DEFS_H_
#ifdef __cplusplus
extern "C" {
#endif /*  __cplusplus */

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

#define OK        (0)
#define NOTOK     (-1)
#define DEFAULT_STRING_SIZE 64
#define CSFILE_FD_R     1
#define CSFILE_FD_W     2
#define CSFILE_STD      3
#define CSFILE_SND_R    4
#define CSFILE_SND_W    5
#define MAXINSNO  (200)
#define PMAX      (1998)
#define VARGMAX   (1999)
#define NOT_AN_INSTRUMENT (-1)

// long max table length is the default for doubles  
#if defined(USE_DOUBLE) && !defined(SHORT_TABLE_LENGTH)  
// MAXLEN is the largest positive 32bit signed pow of two  
static const int32_t MAXLEN = 1 << 30;
static const double FMAXLEN = (double) (1 << 30);
static const uint32_t PHMASK = (1 << 30) - 1;
#else   // this is the original max table length - floats
static const int32_t MAXLEN =  1 << 24;
static const double FMAXLEN = (double) (1 << 24);
static const uint32_t PHMASK = (1 << 24) - 1;
#endif  
#define PFRAC(x)   ((MYFLT)((x) & ftp->lomask) * ftp->lodiv)
#define MAXPOS     0x7FFFFFFFL
#define MAX_STRING_CHANNEL_DATASIZE 16384
#define BYTREVS(n) ((n>>8  & 0xFF) | (n<<8 & 0xFF00))
#define BYTREVL(n) ((n>>24 & 0xFF) | (n>>8 & 0xFF00L) |         \
                    (n<<8 & 0xFF0000L) | (n<<24 & 0xFF000000L))
#define OCTRES     8192
#define CPSOCTL(n) ((MYFLT)(1<<((int32_t)(n)>>13))*csound->cpsocfrc[(int32_t)(n)&8191])
#ifdef USE_DOUBLE
  extern int64_t MYNAN;
#define SSTRCOD    (double) NAN
#else
  extern int32 MYNAN;
#define SSTRCOD    (float) NAN
#endif
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
#define MAXCHAN         16      /* 16 MIDI channels per port */
/* A440 tuning factor */
#define ONEPT (csound->GetA4(csound)/430.5389646099018460319362438314060262605)
#define LOG10D20 0.11512925              /* for db to ampfac   */
#define DV32768 FL(0.000030517578125)

#ifndef PI
#define PI      (3.141592653589793238462643383279502884197)
#endif /* pi */ 
#define TWOPI   (6.283185307179586476925286766559005768394)
#define HALFPI  (1.570796326794896619231321691639751442099)
#ifndef PI_F  
#define PI_F    ((MYFLT) PI)
#endif
#ifndef TWOPI_F    
#define TWOPI_F ((MYFLT) TWOPI)
#endif
#ifndef HALFPI_F  
#define HALFPI_F ((MYFLT) HALFPI)
#endif  
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
/* check for power of two ftable sizes */
#define IS_POW_TWO(N) ((N != 0) ? !(N & (N - 1)) : 0)

#define CS_STATE_PRE    (1)
#define CS_STATE_COMP   (2)
#define CS_STATE_UTIL   (4)
#define CS_STATE_CLN    (8)
#define CS_STATE_JMP    (16)

/* max number of input/output args for user defined opcodes */
#define OPCODENUMOUTS_LOW   16
#define OPCODENUMOUTS_HIGH  64
#define OPCODENUMOUTS_MAX   256

#define MBUFSIZ         (4096)
#define MIDIINBUFMAX    (1024)
#define MIDIINBUFMSK    (MIDIINBUFMAX-1)
#define MIDIMAXPORTS    (64)

#define NAMELEN 40              /* array size of repeat macro names */
#define RPTDEPTH 40             /* size of repeat_n arrays (39 loop levels) */  
#define LBUFSIZ   32768
  
  typedef int32_t (*SUBR)(CSOUND *, void *); 
  enum {FFT_LIB=0, PFFT_LIB, VDSP_LIB};
  enum {FFT_FWD=0, FFT_INV};
  
#ifdef __cplusplus
}
#endif /*  __cplusplus */

#endif // _CORE_DEFS_H_
