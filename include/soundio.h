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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/
                                /*                      SOUNDIO.H       */
#ifndef CSOUND_SOUNDIO_H
#define CSOUND_SOUNDIO_H

#include "csoundCore.h"


#ifdef WIN32
#define IOBUFSAMPS   4096   /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   16384  /* default samps in hardware buffer,  -B settable */
#elif defined(NeXT) || defined(__MACH__)
#define IOBUFSAMPS   1024   /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   4096   /* default samps in hardware buffer,  -B settable */
#elif defined(ANDROID)
#define IOBUFSAMPS   2048   /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   4096   /* default samps in hardware buffer,  -B settable */
#else
#define IOBUFSAMPS   256    /* default sampframes in audio iobuf, -b settable */
#define IODACSAMPS   1024   /* default samps in hardware buffer,  -B settable */
#endif

#define SNDINBUFSIZ  4096   /* soundin bufsize;   must be > sizeof(SFHEADER), */
                            /*                 but small is kind to net rexec */
#define MAXSNDNAME   1024

#ifdef __cplusplus
extern "C" {
#endif

/* generic sound input structure */

typedef struct {
        void   *sinfd;             /* sound file handle                    */
        MYFLT   *inbufp, *bufend;   /* current buffer position, end of buf  */
        void    *fd;                /* handle returned by csoundFileOpen()  */
        int32_t     bufsmps;            /* number of mono samples in buffer     */
        int32_t     format;             /* sample format (AE_SHORT, etc.)       */
        int32_t     channel;            /* requested channel (ALLCHNLS: all)    */
        int32_t     nchanls;            /* number of channels in file           */
        int32_t     sampframsiz;        /* sample frame size in bytes           */
        int32_t     filetyp;            /* file format (TYP_WAV, etc.)          */
        int32_t     analonly;           /* non-zero for analysis utilities      */
        int32_t     endfile;            /* end of file reached ? non-zero: yes  */
        int32_t     sr;                 /* sample rate in Hz                    */
        int32_t     do_floatscaling;    /* scale floats by fscalefac ? 0: no    */
        int64_t audrem, framesrem, getframes;   /* samples, frames, frames */
        MYFLT   fscalefac;
        MYFLT   skiptime;
        char    sfname[MAXSNDNAME];
        MYFLT   inbuf[SNDINBUFSIZ];
} SOUNDIN;

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_SOUNDIO_H */
