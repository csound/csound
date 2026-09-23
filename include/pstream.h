/*
    pstream.h:

    Copyright (C) 2001 Richard Dobson

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

#ifndef __PSTREAM_H_INCLUDED
#define __PSTREAM_H_INCLUDED

/* pstream.h.  Implementation of PVOCEX streaming opcodes.
   (c) Richard Dobson August 2001
   NB pvoc routines based on CARL distribution (Mark Dolson).
   This file is licensed according to the terms of the GNU LGPL.
 */

/* opcodes:     PROVISIONAL DEFINITIONS

  fsig      pvsanal ain,ifftsize,ioverlap,iwinsize,iwintype[,iformat,iinit]

    iwintype:   0 =  HAMMING, 1 =  VonHann, 2 = Kaiser(?)
    iformat:    only PVS_AMP_FREQ (0) supported at present
                (TODO: add f-table support for custom window)
                ( But: really need a param to associate with the window too,
                       or just use a standard default value...)

  fsig      pvsfread ktimpt,ifn[,ichan]

  asig      pvsynth fsig[,iinit]

  asig      pvsadsyn fsig,inoscs,kfmod[,ibinoffset,ibinincr,iinit]

    ibinoffset: starting bin (default 0)
    ibinincr:   distance between successive bins (default 1)
    kfmod:      multiplier; 1 = no change, 2 = up one octave.

  fsig      pvscross fsrc,fdest,kamp1,kamp2

  fsig      pvsmaska  fsrc,ifn,kdepth

  ioverlap,inumbins,iwinsize,iformat    pvsinfo     fsig

    ( will need sndinfo supporting pvocex files anyway,
      to know numchans, wintype, etc.)

  fdest     =   fsrc

    ( woo-hoo! operator overloading in Csound!)
    ( NB an init statement for fsigs is not supported. One day....)

  kflag     pvsftw fsig,ifna [,ifnf]
            pvsftr fsig,ifna [,ifnf]

    ( this modifies an ~existing~ signal, does not create a new one,
      hence no output)

  Re iinit: not implemented yet: and I still need to establish
                                 if it's possible...
 */

typedef struct {
  cs_float re;
  cs_float im;
} CMPLX;

struct pvsdat {
        int32           N;
        int32_t             sliding; /* Flag to indicate sliding case */
        int32           NB;
        int32           overlap;
        int32           winsize;
        int32_t             wintype;
        int32           format;         /* fixed for now to AMP:FREQ */
        uint32          framecount;
        AUXCH           frame;          /* RWD MUST always be 32bit floats */
                                        /* But not in sliding case when cs_float */
};

/* may be no point supporting Kaiser in an opcode unless we can support
   the param too but we can have kaiser in a PVOCEX file. */

typedef struct {
        OPDS    h;
        PVSDAT  *fsig;                  /* output signal is an analysis frame */
        cs_float   *ain;                   /* input sig is audio */
        cs_float   *fftsize;               /* params */
        cs_float   *overlap;
        cs_float   *winsize;
        cs_float   *wintype;
        cs_float   *format;                /* always PVS_AMP_FREQ at present */
        cs_float   *init;                  /* not yet implemented */
        /* internal */
        int32    buflen;
        float   fund,arate;
        float   RoverTwoPi,TwoPioverR,Fexact;
        cs_float   *nextIn;
        int32    nI,Ii,IOi;              /* need all these ?; double as N and NB */
        int32    inptr;

        AUXCH   input;
        AUXCH   overlapbuf;
        AUXCH   analbuf;
        AUXCH   analwinbuf;     /* prewin in SDFT case */
        AUXCH   oldInPhase;
        AUXCH           trig;
        cs_double          *cosine, *sine;
        void    *setup;
} PVSANAL;

typedef struct {
        OPDS    h;
        cs_float   *aout;                  /* audio output signal */
        PVSDAT  *fsig;                  /* input signal is an analysis frame */
        cs_float   *init;                  /* not yet implemented */
        /* internal */
        /* check these against fsig vals */
        int32    overlap,winsize,fftsize,wintype,format;
        /* can we allow variant window tpes?  */
        int32    buflen;
        cs_float   fund,arate;
        cs_float   RoverTwoPi,TwoPioverR,Fexact;
        cs_float   *nextOut;
        int32    nO,Ii,IOi;      /* need all these ?*/
        int32    outptr;
        int32    bin_index;      /* for phase normalization across frames */
        /* renderer gets all format info from fsig */

        AUXCH   output;
        AUXCH   overlapbuf;
        AUXCH   synbuf;
        AUXCH   analwinbuf;     /* may get away with a local alloc and free */
        AUXCH   synwinbuf;
        AUXCH   oldOutPhase;

        void    *setup;
} PVSYNTH;

/* for pvadsyn */

typedef struct {
        OPDS    h;
        cs_float   *aout;
        PVSDAT  *fsig;
        cs_float   *n_oscs;
        cs_float   *kfmod;
        cs_float   *ibinoffset;    /* default 0 */
        cs_float   *ibinincr;      /* default 1 */
        cs_float   *init;          /* not yet implemented  */
        /* internal */
        int32    outptr;
        uint32   lastframe;
        /* check these against fsig vals */
        int32    overlap,winsize,fftsize,wintype,format,startbin;
        int32    binincr,lastbin;
        float   one_over_overlap,pi_over_sr, one_over_sr;
        float   fmod;
        AUXCH   a;
        AUXCH   x;
        AUXCH   y;
        AUXCH   amps;
        AUXCH   lastamps;
        AUXCH   freqs;
        AUXCH   outbuf;
} PVADS;

/* for pvscross */
typedef struct {
        OPDS h;
        PVSDAT  *fout;
        PVSDAT  *fsrc;
        PVSDAT  *fdest;
        cs_float   *kamp1;
        cs_float   *kamp2;
        /* internal */
        int32    overlap,winsize,fftsize,wintype,format;
        uint32   lastframe;
} PVSCROSS;

/* for pvsmaska */
typedef struct {
        OPDS    h;
        PVSDAT  *fout;
        PVSDAT  *fsrc;
        cs_float   *ifn;
        cs_float   *kdepth;
        /* internal*/
        int32    overlap,winsize,fftsize,wintype,format;
        uint32   lastframe;
        int32_t             nwarned,pwarned;    /* range errors for kdepth */
        FUNC    *maskfunc;
} PVSMASKA;

/* for pvsftw, pvsftr */

typedef struct {
        OPDS    h;
        cs_float   *kflag;
        PVSDAT  *fsrc;
        cs_float   *ifna;   /* amp, required */
        cs_float   *ifnf;   /* freq: optional*/
        /* internal */
        int32    overlap,winsize,fftsize,wintype,format;
        uint32   lastframe;
        FUNC    *outfna, *outfnf;
} PVSFTW;

typedef struct {
        OPDS    h;
        /* no output var*/
        PVSDAT  *fdest;
        cs_float   *ifna;   /* amp, may be 0 */
        cs_float   *ifnf;   /* freq: optional*/
        /* internal */
        int32    overlap,winsize,fftsize,wintype,format;
        uint32   lastframe;
        FUNC    *infna, *infnf;
        cs_float   *ftablea,*ftablef;
} PVSFTR;

/* for pvsfread */
/*  wsig pvsread ktimpt,ifilcod */
typedef struct {
        OPDS h;
        PVSDAT  *fout;
        cs_float   *kpos;
        cs_float   *ifilno;
        cs_float   *ichan;
        /* internal */
        int32_t     ptr;
        int32   overlap,winsize,fftsize,wintype,format;
        uint32  chans, nframes,lastframe,chanoffset,blockalign;
        cs_float   arate;
        float   *membase;        /* RWD MUST be 32bit: reads file */
} PVSFREAD;

/* for pvsinfo */

typedef struct {
        OPDS    h;
        cs_float   *ioverlap;
        cs_float   *inumbins;
        cs_float   *iwinsize;
        cs_float   *iformat;
        /* internal*/
        PVSDAT  *fsrc;
} PVSINFO;

typedef struct {
        OPDS    h;
        PVSDAT  *fout;
        PVSDAT  *fsrc;
} FASSIGN;

#endif

