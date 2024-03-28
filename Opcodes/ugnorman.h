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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
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
  double  amp;
  double  freq;
} ATS_DATA_LOC;

typedef struct _randiats { /* the data for the randi UG */
  int32_t     size;   /* size of the frame in samples this should be sr/freq. */
  int32_t     cnt;    /* sample position counter */
  int32   a1;     /* first amplitude value */
  int32   a2;     /* next  amplitude value */
} RANDIATS;

typedef struct _atsnzaux {
  double  buf[25];
  MYFLT   phaseinc[25];
  MYFLT   nfreq[25];
  RANDIATS randinoise[25];
} atsnzAUX;

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
  MYFLT   *ireturn, *ifileno, *ilocation;
} ATSINFO;

/* structures to pass data to the opcodes */

typedef struct _atsread {
  OPDS    h;
  /* outputs (2) and inputs */
  MYFLT   *kfreq, *kamp, *ktimpnt, *ifileno, *ipartial;
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
  double  timefrmInc;
  /* indicates if the data file is byte swapped or not */
  int32_t     swapped;
} ATSREAD;

typedef struct _atsreadnz {
  OPDS    h;
  MYFLT   *kenergy, *ktimpnt, *ifileno, *inzbin; /* outputs (1) and inputs */
  int32_t     maxFr;
  /* a flag used to indicate if we've steped out of the time range */
  /* of the data, so we do not print too many warnings */
  int32_t     prFlg;
  double  *datastart; /* points to the start of the data */
  int32_t     nzbandloc, frmInc;
  MEMFIL  *atsmemfile;
  double  timefrmInc;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
} ATSREADNZ;

typedef struct _atsadd {
  OPDS    h;
  /* audio output and k & i inputs */
  MYFLT   *aoutput, *ktimpnt, *kfmod, *ifileno, *ifn, *iptls;
  /* optional arguments */
  MYFLT   *iptloffset, *iptlincr, *igatefun;
  /* pointer to table with wave to synthesize sound */
  FUNC    *ftp, *AmpGateFunc;
  AUXCH   auxch;
  MEMFIL  *atsmemfile;

  double  maxFr;
  /* a flag used to indicate if we've steped out of the time range */
  /* of the data, so we do not print too many warnings */
  int32_t     prFlg;
  double  timefrmInc;
  double  MaxAmp;     /* maximum amplitude in anaylsis file */
  int32_t     firstpartial, partialinc, frmInc;
  double  *datastart;
  double  *oscphase;  /* oscillator phase */
  ATS_DATA_LOC *buf;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
  MYFLT *oldamps;
  int32_t floatph;
} ATSADD;

typedef struct _atsaddnz {
  OPDS    h;
  /* audio output and k & i inputs */
  MYFLT   *aoutput, *ktimpnt, *ifileno, *ibands;
  /* optional arguments */
  MYFLT   *ibandoffset, *ibandincr;

  MEMFIL  *atsmemfile;  /* a pointer into the ATS file */

  double  maxFr;
  int32_t     prFlg;
  int32_t     frmInc; /* amount to increment frame pointer to get to next frame */
  double  timefrmInc;
  double  winsize;    /* size of windows in analysis file, used to */
  /*   compute RMS amplitude from energy in noise band */
  double  *datastart;

  double  buf[25];      /* stores band information for passing data */
  double  phaseinc[25]; /* to create an array of noise */
  double  oscphase[25]; /* the phase of all the oscilators */
  RANDIATS randinoise[25]; /* pointer to the interpolated random noise info */
  double  nfreq[25];
  int32_t     firstband;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
  int32_t     bands, bandoffset, bandincr;
  ATSSTRUCT atshead;
} ATSADDNZ;

struct _atsbufread {
  OPDS    h;
  MYFLT   *ktimpnt, *kfmod, *ifileno, *iptls;
  MYFLT   *iptloffset, *iptlincr;     /* optional arguments */
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
  double  timefrmInc;
  MYFLT   MaxAmp;     /* maximum amplitude in anaylsis file */
  double  *datastart; /* pointer to the data (past the header) */
  ATSSTRUCT atshead;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
};

typedef struct _atscross {
  OPDS    h;
  /* audio output and k & i inputs */
  MYFLT   *aoutput, *ktimpnt, *kfmod, *ifileno, *ifn;
  MYFLT   *kmyamp, *katsbufamp, *iptls;
  /* optional arguments */
  MYFLT   *iptloffset, *iptlincr, *igatefun, *kthresh;

  /* pointer to table with wave to synthesize sound */
  FUNC    *ftp, *AmpGateFunc;
  AUXCH   auxch;
  MEMFIL  *atsmemfile;

  double  maxFr;
  /* a flag used to indicate if we've steped out of the time range */
  /* of the data, so we do not print too many warnings */
  int32_t     prFlg;
  double  timefrmInc;
  double  MaxAmp;     /* maximum amplitude in anaylsis file */
  int32_t     firstpartial, partialinc, frmInc;
  double  *datastart;
  double  *oscphase;  /* oscillator phase */
  ATS_DATA_LOC *buf;
  int32_t     swapped;    /* indicates if the data file is byte swapped or not */
  MYFLT   *oldamps;
  int32_t floatph;
} ATSCROSS;             /* modified from atsadd */

typedef struct _atssinnoi {
  OPDS    h;
  /* audio output and k & i inputs */
  MYFLT   *aoutput, *ktimpnt, *ksinamp, *knzamp, *kfreq, *ifileno, *iptls;
  /* optional arguments */
  MYFLT   *iptloffset, *iptlincr, *igatefun;

  MEMFIL  *atsmemfile;  /* a pointer into the ATS file */
  AUXCH   auxch;

  double  maxFr;
  int32_t prFlg;
  int32_t nzmemsize;
  /* double  winsize; */   /* size of windows in analysis file, used to */
  /* compute RMS amplitude from energy in noise band */
  double  *datastart;
  double  *nzdata;

  int32_t firstpartial;
  int32_t partialinc;
  int32_t firstband;
  int32_t frmInc; /* amount to increment frame pointer to get to next frame */
  double  timefrmInc;
  int32_t npartials;

  ATS_DATA_LOC *oscbuf; /* stores band information for passing data */

  double  *nzbuf;       /* stores band information for passing data */
  double  *oscphase;    /* the phase of all the oscilators */
  RANDIATS *randinoise; /* a pointer to the interpolated random noise info */
  ATSSTRUCT *atshead;
  char    *filename;
  int32_t swapped;    /* indicates if the data file is byte swapped or not */
  double noiphase[25];
  double phaseinc[25];

} ATSSINNOI;

typedef struct _atspartialtap {
  OPDS    h;
  MYFLT   *kfreq, *kamp, *iparnum;    /* out: freq, amp, in: partialnumber */
} ATSPARTIALTAP;

typedef struct _atsinterpread {
  OPDS    h;
  MYFLT   *kamp, *kfreq;              /* output amp, input: frequency */
  int32_t     overflowflag;
} ATSINTERPREAD;

