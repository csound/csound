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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUND_DISKIN2_H
#define CSOUND_DISKIN2_H

#include <sndfile.h>

#define DISKIN2_MAXCHN  24              /* for consistency with soundin   */
#define POS_FRAC_SHIFT  24              /* allows pitch accuracy of 2^-24 */
#define POS_FRAC_SCALE  0x01000000
#define POS_FRAC_MASK   0x00FFFFFF

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
 /* ------------------------------------- */
    int     initDone;
    int     nChannels;
    int     bufSize;            /* in sample frames, power of two */
    int     wrapMode;
    long    fileLength;         /* in sample frames */
    long    bufStartPos;
    long    prvBufStartPos;
    long    pos_int;
    int     pos_frac;
    int     pos_frac_inc;
    int     winSize;
    MYFLT   prv_kTranspose;
    MYFLT   warpScale;
    MYFLT   winFact;
    MYFLT   *buf;
    MYFLT   *prvBuf;
    SNDFILE *sf;
    FDCH    fdch;
    AUXCH   auxData;            /* for dynamically allocated buffers */
} DISKIN2;

int diskin2_init(ENVIRON *csound, DISKIN2 *p);
int diskin2_perf(ENVIRON *csound, DISKIN2 *p);

#endif      /* CSOUND_DISKIN2_H */

