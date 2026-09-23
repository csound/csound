/*
  ugnorman.h:

  Copyright 2004 Alex Norman

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

/* ats-csound version 0.1
 * Mon May 10 19:44:46 PDT 2004
 * header file for all of the ATScsound functions by Alex Norman
 */

#pragma once

#include "stdopcod.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef struct atsdataloc {
  cs_double  amp;
  cs_double  freq;
} ATS_DATA_LOC;

typedef struct _randiats { /* the data for the randi UG */
  int32_t     size;   /* size of the frame in samples this should be sr/freq. */
  int32_t     cnt;    /* sample position counter */
  int32   a1;     /* first amplitude value */
  int32   a2;     /* next  amplitude value */
} RANDIATS;

typedef struct _atsnzaux {
  cs_double  buf[25];
  cs_float   phaseinc[25];
  cs_float   nfreq[25];
  RANDIATS randinoise[25];
} atsnzAUX;

/* The mapped ATS header and datastart buffers always contain 64-bit doubles. */
typedef struct atsstruct {
  double  magic;      /* ats magic number */
  double  sampr;      /* sampling rate */
  double  frmsz;      /* frame size in samples */
  double  winsz;      /* window size in samples */
  double  npartials;  /* number of partials */
  double  nfrms;      /* number of frames */
  double  ampmax;     /* max amplitude */
  double  freqmax;    /* max frequency */
  double  dur;        /* duration seconds */
  double  type;       /* Ats Frame type 1-4 */
} ATSSTRUCT;

typedef struct _atsinfo {
  OPDS    h;
  /* the return value, the ats file and a location selection */
  cs_float   *ireturn, *ifileno, *ilocation;
} ATSINFO;

/* structures to pass data to the opcodes */

typedef struct _atsread {
  OPDS    h;
  /* outputs (2) and inputs */
  cs_float   *kfreq, *kamp, *ktimpnt, *ifileno, *ipartial;
  /* indicates the maximun frame */
  int32_t     maxFr;
  /* a flag used to indicate if we've steped out of the time range */
  /* of the data, so we do not print too many warnings */
  int32_t     prFlg;
  /* points to the start of the data */
  double  *datastart;
  /* tells the location of the partal to output */
  /* and the number of doubles to increment to get to the next frame */
  int32_t     partialloc, frmInc;
  MEMFIL  *atsmemfile;
  cs_double  timefrmInc;
  /* indicates if the data file is byte swapped or not */
  int32_t     swapped;
} ATSREAD;

typedef struct _atsreadnz {
  OPDS    h;
  cs_float   *kenergy, *ktimpnt, *ifileno, *inzbin; /* outputs (1) and inputs */
  int32_t     maxFr;
  /* a flag used to indicate if we've steped out of the time range */
  /* of the data, so we do not print too many warnings */
  int32_t     prFlg;
  double  *datastart; /* points to the start of the data */
  int32_t     nzbandloc, frmInc;
  MEMFIL  *atsmemfile;
  cs_double  timefrmInc;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
} ATSREADNZ;

typedef struct _atsadd {
  OPDS    h;
  /* audio output and k & i inputs */
  cs_float   *aoutput, *ktimpnt, *kfmod, *ifileno, *ifn, *iptls;
  /* optional arguments */
  cs_float   *iptloffset, *iptlincr, *igatefun;
  /* pointer to table with wave to synthesize sound */
  FUNC    *ftp, *AmpGateFunc;
  AUXCH   auxch;
  MEMFIL  *atsmemfile;

  cs_double  maxFr;
  /* a flag used to indicate if we've steped out of the time range */
  /* of the data, so we do not print too many warnings */
  int32_t     prFlg;
  cs_double  timefrmInc;
  cs_double  MaxAmp;     /* maximum amplitude in anaylsis file */
  int32_t     firstpartial, partialinc, frmInc;
  double  *datastart;
  cs_double  *oscphase;  /* oscillator phase */
  ATS_DATA_LOC *buf;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
  cs_float *oldamps;
  int32_t floatph;
} ATSADD;

typedef struct _atsaddnz {
  OPDS    h;
  /* audio output and k & i inputs */
  cs_float   *aoutput, *ktimpnt, *ifileno, *ibands;
  /* optional arguments */
  cs_float   *ibandoffset, *ibandincr;

  MEMFIL  *atsmemfile;  /* a pointer into the ATS file */

  cs_double  maxFr;
  int32_t     prFlg;
  int32_t     frmInc; /* amount to increment frame pointer to get to next frame */
  cs_double  timefrmInc;
  cs_double  winsize;    /* size of windows in analysis file, used to */
  /*   compute RMS amplitude from energy in noise band */
  double  *datastart;

  cs_double  buf[25];      /* stores band information for passing data */
  cs_double  phaseinc[25]; /* to create an array of noise */
  cs_double  oscphase[25]; /* the phase of all the oscilators */
  RANDIATS randinoise[25]; /* pointer to the interpolated random noise info */
  cs_double  nfreq[25];
  int32_t     firstband;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
  int32_t     bands, bandoffset, bandincr;
  ATSSTRUCT atshead;
} ATSADDNZ;

struct _atsbufread {
  OPDS    h;
  cs_float   *ktimpnt, *kfmod, *ifileno, *iptls;
  cs_float   *iptloffset, *iptlincr;     /* optional arguments */
  MEMFIL  *mfp;
  int32_t     maxFr, prFlg;
  /* base Frame (in frameData0) and maximum frame on file, ptr to fr, size */
  AUXCH   auxch;
  ATS_DATA_LOC *table;  /* store freq and amp info for later use */
  ATS_DATA_LOC *utable; /* store freq and amp info for later use (unsorted) */
  int32_t     frmInc; /* amount to increment frame pointer to get to next frame */
  int32_t     firstpartial; /* location of first wanted partial in the frame */
  int32_t     partialinc; /* amount to increment pointer by */
  /*   to get at the next partial in a frame */
  cs_double  timefrmInc;
  cs_float   MaxAmp;     /* maximum amplitude in anaylsis file */
  double  *datastart; /* pointer to the data (past the header) */
  ATSSTRUCT atshead;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
};

typedef struct _atscross {
  OPDS    h;
  /* audio output and k & i inputs */
  cs_float   *aoutput, *ktimpnt, *kfmod, *ifileno, *ifn;
  cs_float   *kmyamp, *katsbufamp, *iptls;
  /* optional arguments */
  cs_float   *iptloffset, *iptlincr, *igatefun, *kthresh;

  /* pointer to table with wave to synthesize sound */
  FUNC    *ftp, *AmpGateFunc;
  AUXCH   auxch;
  MEMFIL  *atsmemfile;

  cs_double  maxFr;
  /* a flag used to indicate if we've steped out of the time range */
  /* of the data, so we do not print too many warnings */
  int32_t     prFlg;
  cs_double  timefrmInc;
  cs_double  MaxAmp;     /* maximum amplitude in anaylsis file */
  int32_t     firstpartial, partialinc, frmInc;
  double  *datastart;
  cs_double  *oscphase;  /* oscillator phase */
  ATS_DATA_LOC *buf;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
  cs_float   *oldamps;
  int32_t floatph;
} ATSCROSS;             /* modified from atsadd */

typedef struct _atssinnoi {
  OPDS    h;
  /* audio output and k & i inputs */
  cs_float   *aoutput, *ktimpnt, *ksinamp, *knzamp, *kfreq, *ifileno, *iptls;
  /* optional arguments */
  cs_float   *iptloffset, *iptlincr, *igatefun;

  MEMFIL  *atsmemfile;  /* a pointer into the ATS file */
  AUXCH   auxch;

  cs_double  maxFr;
  int32_t prFlg;
  cs_double  winsize; /* analysis window size for noise energy to amplitude */
  double  *datastart;

  int32_t firstpartial;
  int32_t partialinc;
  int32_t firstband;
  int32_t frmInc; /* amount to increment frame pointer to get to next frame */
  cs_double  timefrmInc;
  int32_t partials;

  ATS_DATA_LOC *oscbuf; /* stores band information for passing data */

  cs_double  *nzbuf;       /* stores band information for passing data */
  cs_double  *oscphase;    /* the phase of all the oscilators */
  RANDIATS *randinoise; /* a pointer to the interpolated random noise info */
  int32_t swapped;    /* indicates if the data file is byte swapped or not */
  cs_double noiphase[25];
  cs_double phaseinc[25];

} ATSSINNOI;

typedef struct _atspartialtap {
  OPDS    h;
  cs_float   *kfreq, *kamp, *iparnum;    /* out: freq, amp, in: partialnumber */
} ATSPARTIALTAP;

typedef struct _atsinterpread {
  OPDS    h;
  cs_float   *kamp, *kfreq;              /* output amp, input: frequency */
  int32_t     overflowflag;
} ATSINTERPREAD;

