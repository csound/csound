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

#ifndef SOUNDIO_H
#define SOUNDIO_H

#ifdef WIN32
#define IOBUFSAMPS   4096   /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   16384  /* default samps in hardware buffer,  -B settable */
#elif defined(NeXT)
#define IOBUFSAMPS   1024   /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   4096   /* default samps in hardware buffer,  -B settable */
#else
#define IOBUFSAMPS   256    /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   1024   /* default samps in hardware buffer,  -B settable */
#endif
#define SNDINBUFSIZ  4096   /* soundin bufsize;   must be > sizeof(SFHEADER), */
                            /*                 but small is kind to net rexec */
#define SNDOUTSMPS   (1024)
                                /* standard audio encoding types */
#define AE_CHAR   SF_FORMAT_PCM_S8
#define AE_SHORT  SF_FORMAT_PCM_16
#define AE_24INT  SF_FORMAT_PCM_24
#define AE_LONG   SF_FORMAT_PCM_32
#define AE_UNCH   SF_FORMAT_PCM_U8
#define AE_FLOAT  SF_FORMAT_FLOAT
#define AE_DOUBLE SF_FORMAT_DOUBLE
#define AE_ULAW   SF_FORMAT_ULAW
#define AE_ALAW   SF_FORMAT_ALAW
#define AE_IMA_ADPCM    SF_FORMAT_IMA_ADPCM
#define AE_MS_ADPCM     SF_FORMAT_MS_ADPCM
#define AE_GSM610       SF_FORMAT_GSM610
#define AE_VOX          SF_FORMAT_VOX_ADPCM
#define AE_G721_32      SF_FORMAT_G721_32
#define AE_G723_24      SF_FORMAT_G723_24
#define AE_G723_40      SF_FORMAT_G723_40
#define AE_DWVW_12      SF_FORMAT_DWVW_12
#define AE_DWVW_16      SF_FORMAT_DWVW_16
#define AE_DWVW_24      SF_FORMAT_DWVW_24
#define AE_DWVW_N       SF_FORMAT_DWVW_N
#define AE_DPCM_8       SF_FORMAT_DPCM_8
#define AE_DPCM_16      SF_FORMAT_DPCM_16
#define AE_LAST   SF_FORMAT_DPCM_16     /* current last audio encoding value */

#define TYP_WAV   (SF_FORMAT_WAV>>16)
#define TYP_AIFF  (SF_FORMAT_AIFF>>16)
#define TYP_AU    (SF_FORMAT_AU>>16)
#define TYP_RAW   (SF_FORMAT_RAW>>16)
#define TYP_PAF   (SF_FORMAT_PAF>>16)
#define TYP_SVX   (SF_FORMAT_SVX>>16)
#define TYP_NIST  (SF_FORMAT_NIST>>16)
#define TYP_VOC   (SF_FORMAT_VOC>>16)
#define TYP_IRCAM (SF_FORMAT_IRCAM>>16)
#define TYP_W64   (SF_FORMAT_W64>>16)
#define TYP_MAT4  (SF_FORMAT_MAT4>>16)
#define TYP_MAT5  (SF_FORMAT_MAT5>>16)
#define TYP_PVF   (SF_FORMAT_PVF>>16)
#define TYP_XI    (SF_FORMAT_XI>>16)
#define TYP_HTK   (SF_FORMAT_HTK>>16)
#define TYP_SDS   (SF_FORMAT_SDS>>16)

#define format2sf(x) (x)
#define sf2format(x) (x&0xffff)

#ifdef  USE_DOUBLE
#define sf_write_MYFLT  sf_write_double
#define sf_read_MYFLT   sf_read_double
#else
#define sf_write_MYFLT  sf_write_float
#define sf_read_MYFLT   sf_read_float
#endif

#ifndef __cplusplus

/*RWD 3:2000 moved up */
typedef struct {
    float value;                  /* signed value of peak */
    unsigned long position;       /* the sample frame for the peak */
} PositionPeak;

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
        FDCH    fdch;
        MYFLT   *inbufp, *bufend;
        MYFLT   inbuf[SNDINBUFSIZ];
                /*RWD 3:2000*/
        float   fscalefac;
        long    do_floatscaling;
        long    datpos;
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

#include <sndfile.h>
int sreadin(SNDFILE*, MYFLT *, int, SOUNDIN *);


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

#endif

