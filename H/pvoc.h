/*
    pvoc.h:

    Copyright (C) 1990 Dan Ellis, John ffitch

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

/***************************************************************\
*       pvoc.h                                                  *
*       header defs for pvoc FFT files                          *
*       'inspired' by the NeXT SND system                       *
*       01aug90 dpwe                                            *
\***************************************************************/

#ifndef _PVOC_H_
#define _PVOC_H_

#include "sysdep.h"

#ifdef __GNUC__
#warning "Warning: pvoc.h is deprecated."
#warning "         Use the new pvfileio.h interface instead."
#endif

#define PVMAGIC 517730  /* look at it upside-down, esp on a 7-seg display */
#define PVDFLTBYTS 4

typedef struct pvstruct {
    long        magic;                  /* magic number to identify */
    long        headBsize;              /* byte offset from start to data */
    long        dataBsize;              /* number of bytes of data */
    long        dataFormat;             /* (int) format specifier */
    MYFLT       samplingRate;           /* of original sample */
    long        channels;               /* (int) mono/stereo etc */
    long        frameSize;              /* size of FFT frames (2^n) */
    long        frameIncr;              /* # new samples each frame */
    long        frameBsize;             /* bytes in each file frame */
    long        frameFormat;            /* (int) how words are org'd in frms */
    MYFLT       minFreq;                /* freq in Hz of lowest bin (exists) */
    MYFLT       maxFreq;                /* freq in Hz of highest (or next) */
    long        freqFormat;             /* (int) flag for log/lin frq */
    char        info[PVDFLTBYTS];       /* extendable byte area */
} PVSTRUCT;

#define PVMYFLT (4+32)  /* 32 bit float data */

#endif /* !_PVOC_H_ */

