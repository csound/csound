/*  
    soundio.h:

    Copyright (C) 1991, 2000 Barry Vercoe, Richard Dobson

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

                        /*                              SOUNDIO.H       */

#ifdef WIN32
#define IOBUFSAMPS   16384      /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   16384      /* default samps in hardware buffer,  -B settable */
#elif defined(NeXT)
#define IOBUFSAMPS   2048   /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   4096   /* default samps in hardware buffer,  -B settable */
#else
#define IOBUFSAMPS   1024   /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   1024   /* default samps in hardware buffer,  -B settable */
#endif
#define SNDINBUFSIZ  4096   /* soundin bufsize;   must be > sizeof(SFHEADER), */
                            /*                 but small is kind to net rexec */
#define SNDOUTSMPS   (1024)
                                /* standard audio encoding types */
#define AE_CHAR   0x101         /* Signed 8 bit */
#ifdef never
#define AE_ALAW   0x102
#endif
#ifdef ULAW
#define AE_ULAW   0x103
#endif
#define AE_SHORT  0x104
#define AE_LONG   0x105
#define AE_FLOAT  0x106
#define AE_UNCH   0x107         /* Unsigned 8 bit */
#define AE_24INT  0x108         /*not fully implemented yet */
#define AE_DOUBLE 0x109
#define AE_LAST   AE_DOUBLE     /* current last audio encoding value */

#define TYP_IRCAM 0
#define TYP_AIFF  1
#define TYP_WAV   2
#define TYP_AIFC  3

/*RWD 3:2000*/
#define PEAKCHUNK_VERSION (1L)
#define MIN_SHORTAMP (FL(1.0) / FL(32767.0))
#define MIN_LONGAMP  (FL(1.0) / FL(2147483647.0))
/*RWD 6:2001 */
#define MIN_24AMP       (FL(1.0) / FL(8338607.0))



#ifndef __cplusplus

/*RWD 3:2000 moved up */
typedef struct {
    float value;                  /* signed value of peak */
    unsigned long position;       /* the sample frame for the peak */
} PositionPeak;

typedef struct {
    long        ckID;             /* 'PEAK' */
    unsigned long chunkDataSize;  /* the size of the chunk */
    unsigned long version;        /* version of the PEAK chunk */
    unsigned long timeStamp;      /* secs since 1/1/1970 */
    PositionPeak peak[1];         /* the peak info */
} PeakChunk;

typedef struct {         /* struct for passing data to/from sfheader routines */
        long    sr;
        long    nchanls;
        long    sampsize;
        long    format;
        long    hdrsize;
        int     filetyp;
        AIFFDAT *aiffdata;
        long    audsize;
        long    readlong;
        long    firstlong;
        long    peak_timestamp;          /*RWD 3:2000*/
        long    peak_do_rescaling;       /* for when we get a command arg */
        PositionPeak peaks[MAXCHNLS];
        long     peaksvalid;
} HEADATA;

typedef struct {
        OPDS    h;
        MYFLT   *r[24];
        MYFLT   *ifilno, *iskptim, *iformat, *kfrqratio;
        short   format, channel, nchanls, sampframsiz, filetyp;
        short   analonly, endfile;
        long    sr, audrem, framesrem, getframes;    /* bytes, frames, frames */
        AIFFDAT *aiffdata;
        void    (*bytrev)(char*,int);
        FDCH    fdch;
        char    *inbufp, *bufend;
        char    inbuf[SNDINBUFSIZ+8];
                /*RWD 3:2000*/
        float   fscalefac;
        long    do_floatscaling;
        long    datpos;
                                /* work variables for soundin2 */
/*        double  phase_gab; */
/*         double  fl_buf; */
/*         int     base_sample_gab; */
/*         int     initflag_gab; */
} SOUNDIN;



typedef struct {
        MYFLT   *ifilcod, *iformat;
        short   format, filetyp;
        AIFFDAT *aiffdata;
        void    (*swrtmethod)(int, MYFLT *, int);
        FDCH    fdch;
        MYFLT   *outbufp, *bufend;
        MYFLT   outbuf[SNDOUTSMPS];
} SNDCOM;

typedef struct {
        OPDS    h;
        MYFLT   *asig;
        SNDCOM  c;
} SNDOUT;

typedef struct {
        OPDS    h;
        MYFLT   *asig1, *asig2;
        SNDCOM  c;
} SNDOUTS;

int sreadin(int, char *, int, SOUNDIN *);

#ifdef SFSUN41
#include <multimedia/audio_hdr.h>
# ifdef sol
#  include <multimedia/audio_device.h>
# endif
#include <multimedia/libaudio.h>
#endif

#ifdef SFIRCAM

#define SF_MAXCHAN      4
#define SF_MINCOM       400

#define SF_END          0         /* SFCODE types */
#define SF_MAXAMP       1
#define SF_AUDIOENCOD   2
#define SF_PVDATA       3
#define SF_COMMENT      4
#define SF_CODMAX       4

typedef struct {                  /* this code written by sfheaders.c */
        float   value[SF_MAXCHAN];
} SFMAXAMP;

typedef struct {                  /*     ditto                    */
        short   encoding;
        short   grouping;
} SFAUDIOENCOD;

typedef struct {                  /* this code written by pvanal */
        short   frameSize;
        short   frameIncr;
} SFPVDATA;

typedef struct {                  /* this code not possible yet */
        char    comment[SF_MINCOM];
} SFCOMMENT;

#endif
#endif

/*RWD 3:2000*/
#define INLONGFAC (1.0 / 65536.0)    /* convert 32bit long into quasi 16 bit range */
#define INMYFLTFAC (FL(32767.0))

/*RWD 5:2001: 24bit files support*/
#define OUT24LONGFAC (FL(65536.0))       /* convert 16bit into quasi 32bit long */
/* good for 4/6/8 framesize; but we probably need to calculate it per file...*/
#define SNDIN24BUFSIZ (4104)            /* mult of 216 */

typedef union samp_24 {
        long lsamp;
        unsigned char bytes[4];
} SAMP24;
