/*
    diskin.h:

    Copyright (C) 1998, 2001 matt ingalls, Richard Dobson, John ffitch
              (C) 2005 Istvan Varga

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

#ifndef CSOUND_DISKIN_H
#define CSOUND_DISKIN_H

#include "diskin2.h"

typedef struct {
    OPDS    h;
    MYFLT   *aOut[DISKIN2_MAXCHN];
    MYFLT   *iFileCode;
    MYFLT   *kTranspose;
    MYFLT   *iSkipTime;
    MYFLT   *iWrapMode;
    MYFLT   *iSampleFormat;
    MYFLT   *iSkipInit;
    MYFLT   *ibufsize;
 /* ------------------------------------- */
    int     initDone;
    int     nChannels;
    int     bufSize;            /* in sample frames, power of two */
    int     wrapMode;
    int32    fileLength;         /* in sample frames */
    int32    bufStartPos;
    int64_t pos_frac;           /* type should be defined in sysdep.h */
    int64_t pos_frac_inc;
    SNDFILE *sf;
    MYFLT   prv_kTranspose;
    MYFLT   scaleFac;
    float   *buf;          /* variable */
    AUXCH   auxch;
    FDCH    fdch;
} SOUNDINEW;

#define SNDOUTSMPS  (1024)

typedef struct {
    SNDFILE *sf;
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

#endif      /* CSOUND_DISKIN_H */

