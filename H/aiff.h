/*  
    aiff.h:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Richard Dobson

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

/*                                                             AIFF.H   */

typedef long CKID;

typedef struct {
        CKID    ckID;
        long    ckSize;
} CkHdr;

typedef struct {
        CkHdr   ckHdr;
        CKID      formType;
} FormHdr;

typedef struct {
        CkHdr   ckHdr;                   /* CommonChunk in 2 parts to avoid */
        short   numChannels;             /*   this short rounded up to long */
} CommChunk1;

typedef struct {
        long    numSampleFrames;         /*   ... to accomodate this long   */
        short   sampleSize;
        char    sampleRate[10];          /* 80-bit extended value     */
} CommChunk2;

typedef struct {
        CKID      compressionType;
        char    compressionName[256];
} CommChunk3;

/* for AIFF-C/32 float*/

/* #define Float32Name {11, 'F','l','o','a','t',' ','3','2',' ',' ',' '} */
/*RWD 3:2000 handle pad byte correctly in the code! */
#define Float32Name    {8, 'F','l','o','a','t',' ','3','2'}
#define Float32Type    {'F','L','3','2'}           /*RWD 3:2000 NB not used */

typedef struct {
        CkHdr ckHdr;
        long version;
} AifcFverChunk;

typedef struct {
        CkHdr   ckHdr;
        unsigned long   applicationSignature;
        unsigned char   data[1];                        /* variable length array */
} AIFFAppSpecificChunk ;


/* end of stuff for AIFF-C 32-float */

typedef short MrkrID;

typedef struct {
        short   playMode;
        MrkrID  beginLoop;
        MrkrID  endLoop;
} Loop;

typedef struct {
        CkHdr   ckHdr;
        char    baseNote;
        char    detune;
        char    lowNote;
        char    highNote;
        char    lowVelocity;
        char    highVelocity;
        short   gain;
        Loop    sustainLoop;
        Loop    releaseLoop;
} InstrChunk;

typedef struct {
        CkHdr   ckHdr;
        long    offset;
        long    blockSize;
} SoundDataHdr;

/* To lump Form, CommonChunk, and SoundData into one,
 *  add FormHdr, CommChunk1, CommChunk2, SoundDataHdr.
 */
