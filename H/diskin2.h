/*
    diskin2.h:

    Copyright (C) 2005 Istvan Varga

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

#ifndef CSOUND_DISKIN2_H
#define CSOUND_DISKIN2_H

#include "soundio.h"

#define DISKIN2_MAXCHN  40              /* for consistency with soundin   */
#define POS_FRAC_SHIFT  28              /* allows pitch accuracy of 2^-28 */
#define POS_FRAC_SCALE  0x10000000
#define POS_FRAC_MASK   0x0FFFFFFF

typedef struct {
    OPDS    h;
    MYFLT   *aOut[DISKIN2_MAXCHN];
    MYFLT   *iFileCode;
    MYFLT   *kTranspose;
    MYFLT   *iSkipTime;
    MYFLT   *iWrapMode;
    MYFLT   *iSampleFormat;
    MYFLT   *iWinSize;
    MYFLT   *iBufSize;
    MYFLT   *iSkipInit;
    MYFLT   *forceSync;
 /* ------------------------------------- */
    MYFLT   WinSize;
    MYFLT   BufSize;
    MYFLT   SkipInit;
    MYFLT   fforceSync;

    int32_t     initDone;
    int32_t     nChannels;
    int32_t     bufSize;            /* in sample frames, power of two */
    int32_t     wrapMode;
    int32   fileLength;         /* in sample frames */
    int32   bufStartPos;
    int64_t pos_frac;           /* type should be defined in sysdep.h */
    int64_t pos_frac_inc;
    int32   prvBufStartPos;
    int32   winSize;
    MYFLT   *buf;
    MYFLT   *prvBuf;
    MYFLT   prv_kTranspose;
    MYFLT   winFact;
    double  warpScale;
    void *sf;
    FDCH    fdch;
    AUXCH   auxData;            /* for dynamically allocated buffers */
    AUXCH   auxData2;
    MYFLT   *aOut_buf;
    MYFLT   aOut_bufsize;
    void    *cb;
    int32_t     async;
  MYFLT     transpose;
} DISKIN2;

typedef struct {
    OPDS    h;
    ARRAYDAT *aOut;
    MYFLT   *iFileCode;
    MYFLT   *kTranspose;
    MYFLT   *iSkipTime;
    MYFLT   *iWrapMode;
    MYFLT   *iSampleFormat;
    MYFLT   *iWinSize;
    MYFLT   *iBufSize;
    MYFLT   *iSkipInit;
    MYFLT   *forceSync;
 /* ------------------------------------- */
    MYFLT   WinSize;
    MYFLT   BufSize;
    MYFLT   SkipInit;
    MYFLT   fforceSync;
    int32_t     initDone;
    int32_t     nChannels;
    int32_t     bufSize;            /* in sample frames, power of two */
    int32_t     wrapMode;
    int32    fileLength;         /* in sample frames */
    int32    bufStartPos;
    int64_t pos_frac;           /* type should be defined in sysdep.h */
    int64_t pos_frac_inc;
    int32    prvBufStartPos;
    int32    winSize;
    MYFLT   *buf;
    MYFLT   *prvBuf;
    MYFLT   prv_kTranspose;
    MYFLT   winFact;
    double  warpScale;
    void *sf;
    FDCH    fdch;
    AUXCH   auxData;            /* for dynamically allocated buffers */
    AUXCH   auxData2;
  MYFLT *aOut_buf;
  MYFLT aOut_bufsize;
  void *cb;
  int32_t  async;
} DISKIN2_ARRAY;

int32_t diskin2_init(CSOUND *csound, DISKIN2 *p);
int32_t diskin2_init_S(CSOUND *csound, DISKIN2 *p);
int32_t diskin2_perf(CSOUND *csound, DISKIN2 *p);
int32_t diskin2_init_array_I(CSOUND *csound, DISKIN2_ARRAY *p);
int32_t diskin2_init_array_S(CSOUND *csound, DISKIN2_ARRAY *p);
int32_t diskin_init_array_I(CSOUND *csound, DISKIN2_ARRAY *p);
int32_t diskin_init_array_S(CSOUND *csound, DISKIN2_ARRAY *p);
int32_t diskin2_perf_array(CSOUND *csound, DISKIN2_ARRAY *p);
int32_t diskin2_async_deinit(CSOUND *csound, DISKIN2 *p);
int32_t soundout_deinit(CSOUND *csound, void *pp);
int32_t diskin2_async_deinit_array(CSOUND *csound, DISKIN2_ARRAY *p);

typedef struct {
    OPDS    h;
    MYFLT   *aOut[DISKIN2_MAXCHN];
    MYFLT   *iFileCode, *iSkipTime, *iSampleFormat, *iSkipInit, *iBufSize;
    int32_t     nChannels;
    int32_t     bufSize;            /* in sample frames (power of two) */
    int_least64_t   fileLength; /* in sample frames */
    int_least64_t   bufStartPos;
    int_least64_t   read_pos;   /* current sample frame being read */
    MYFLT   *buf;
    void *sf;
    MYFLT   scaleFac;
    FDCH    fdch;
    AUXCH   auxData;            /* for dynamically allocated buffers */
} SOUNDIN_;

#define SNDOUTSMPS  (1024)

typedef struct {
     void *sf;
    void    *fd;
    MYFLT   *outbufp, *bufend;
    MYFLT   outbuf[SNDOUTSMPS];
} SNDCOM;

typedef struct {
    OPDS    h;
    MYFLT   *asig, *ifilcod, *iformat;
    SNDCOM  c;
} SNDOUT;

typedef struct {
    OPDS    h;
    MYFLT   *asig1, *asig2, *ifilcod, *iformat;
    SNDCOM  c;
} SNDOUTS;

#endif      /* CSOUND_DISKIN2_H */
