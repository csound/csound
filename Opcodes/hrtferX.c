/*
    hrtferX.c:

    Copyright (C) 1995, 2001 Eli Breder, David McIntyre, John ffitch

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

/***********************************************************
 *                    FINAL PROJECT                        *
 *                                                         *
 *           course: 216-605A                              *
 *           Digital Sound Synthesis & Audio Processing    *
 *                                                         *
 *                                                         *
 *                      hrtferxk.c                         *
 *                      ----------                         *
 *                                                         *
 *      Eli Breder (9111216) and David McIntyre (9005614)  *
 *                                                         *
 *      Dec. 17, 1995                                      *
 *                                                         *
 *      Code modifications John ffitch January 2001        *
 *                                                         *
 ***********************************************************/

/***************************************************************
 * This version of hrtfer loads the file HRTFcompact into memory.
 * Offsets into the file are calculated in the a-rate code to
 * get the requested HRTF measurements. We've implemented
 * a linear crossfade to deal with the clicks which occur
 * when the input audio is convolved with new HRTFs. Although this
 * the clicking, it does not eliminate them.
 * A better solution would be to implement interpolation between
 * the old and new HRTFs (probably a project in itself).
 ***************************************************************/

/* Legacy opcode behavior in this file is retained for compatibility.
 * Read the local CSOUND_DEPRECATED_OPCODE markers and docs/opcode-deprecation.md before
 * correcting historical output; use the supported replacement for new work.
 */
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "hrtferx.h"

/* This array transferred here so as to be declared once.  Belongs to
   the structure of the HRTF data really in 3Dug.h */

static const int32_t elevation_data[N_ELEV] = {56, 60, 72, 72, 72, 72, 72,
                                           60, 56, 45, 36, 24, 12, 1 };

#define ROUND(x) ((int32_t)floor((x)+FL(0.5)))
#define GET_NFAZ(el_index)      ((elevation_data[el_index] / 2) + 1)

/* LoadMemoryFile calls this only for a newly loaded file, before sharing it. */
static int32_t hrtfer_prepare_data(CSOUND *csound, MEMFIL *mfp)
{
    int32_t i, records = 0;
    const uint16_t endian = 1;
    for (i = 0; i < N_ELEV; i++) records += GET_NFAZ(i);
    if (UNLIKELY(mfp->length < records * BUF_LEN * (int32_t)sizeof(int16))) {
      csound->Message(csound, "%s", Str("hrtfer: incomplete HRTFcompact data\n"));
      return NOTOK;
    }
    if (*(const unsigned char *)&endian) {
      unsigned char *bytes = (unsigned char *)mfp->beginp;
      for (i = 0; i < records * BUF_LEN; i++, bytes += 2) {
        unsigned char tmp = bytes[0];
        bytes[0] = bytes[1];
        bytes[1] = tmp;
      }
    }
    return OK;
}

/* DO NOT FIX historical behavior here without an explicit maintainer decision. */
CSOUND_PRESERVE_LEGACY_BEHAVIOR("hrtfer")
static int32_t hrtferxkSet(CSOUND *csound, HRTFER *p)
{
    MEMFIL *mfp;

        /* first check if orchestra's sampling rate is compatible with HRTF
           measurement's */
    if (UNLIKELY(CS_ESR != SAMP_RATE)) {
      return csound->InitError(csound,
                  Str("Orchestra sampling rate is not compatible with HRTF.\n"
                      "Should be %d...exiting."), SAMP_RATE);
      return NOTOK; /* not reached */
    }

    if (strcmp("HRTFcompact", p->ifilno->data))
      csound->Message(csound, "%s", Str("\nLast argument must be the string "
                                      "'HRTFcompact' ...correcting.\n"));

    p->mfp = mfp = csound->LoadMemoryFile(csound, "HRTFcompact", CSFTYPE_HRTF,
                                        hrtfer_prepare_data);
    if (UNLIKELY(mfp == NULL))
      return csound->InitError(csound, "%s", Str("hrtfer: cannot load HRTFcompact"));
    p->fpbegin = (int16 *)mfp->beginp;
    if (p->aIn == p->aLeft || p->aIn == p->aRight)
      csound->AuxAlloc(csound, CS_KSMPS * sizeof(MYFLT), &p->auxch);
        /* initialize counters and indices */
    p->outcount = 0;
    p->incount = 0;
    p->outfront = p->outend = 0; /* working indices for circ output buffer */

        /* initialize oldhrtf_data with zeros */
        /* initialize input buffer */
        /* initialize left result buffer */
        /* initialize right result buffer */
        /* initialize left output buffer */
        /* initialize right output buffer */
    /* for (i=0; i<BUF_LEN; i++)   { */
    /*   p->x[i]                  = FL(0.0); */
    /*   p->yl[i]                 = FL(0.0); */
    /*   p->yr[i]                 = FL(0.0); */
    /*   p->outl[i]               = FL(0.0); */
    /*   p->outr[i]               = FL(0.0); */
    /* } */
    memset(p->x, 0, BUF_LEN*sizeof(MYFLT));
    memset(p->yl, 0, BUF_LEN*sizeof(MYFLT));
    memset(p->yr, 0, BUF_LEN*sizeof(MYFLT));
    memset(p->outl, 0, BUF_LEN*sizeof(MYFLT));
    memset(p->outr, 0, BUF_LEN*sizeof(MYFLT));

        /* initialize left overlap buffer */
        /* initialize right overlap buffer */
    /* for (i=0; i<FILT_LENm1; i++) { */
    /*   p->bl[i] = FL(0.0); */
    /*   p->br[i] = FL(0.0); */
    /* } */
    memset(p->bl, 0, FILT_LENm1*sizeof(MYFLT));
    memset(p->br, 0, FILT_LENm1*sizeof(MYFLT));
    p->setup = csound->RealFFTSetup(csound, BUF_LEN, FFT_FWD);
    p->isetup = csound->RealFFTSetup(csound, BUF_LEN, FFT_INV);
    return OK;
}

/********************** a-rate code ***********************************/

/* DO NOT FIX historical behavior here without an explicit maintainer decision. */
CSOUND_PRESERVE_LEGACY_BEHAVIOR("hrtfer")
static int32_t hrtferxk(CSOUND *csound, HRTFER *p)
{
    MYFLT      *aLeft, *aRight; /* audio output streams */
    MYFLT      *aIn, *kAz, *kElev; /* audio and control input streams */
    int32_t        azim, elev, el_index, az_index;
    int32_t        nsmpsi, nsmpso; /* number of samples in/out */
    /*         input,      out-left,    out-right */
    MYFLT      *x, *yl, *yr;    /* Local copies of address */
    /*         overlap left,   overlap right */
    MYFLT      *bl, *br;
                        /* copy of current input convolved with old HRTFs */
    MYFLT      *outl, *outr; /* output left/right */
    int32_t        outfront, outend; /* circular output indices */
    int32_t        incount, outcount; /* number of samples in/out */
    uint32_t   toread; /* number of samples to read */
    int32_t        i; /* standard loop counter */
    HRTF_DATUM hrtf_data; /* local hrtf instances */
    int32_t        flip; /* flag - true if we need to flip the channels */
    int16      *fpindex; /* pointer into HRTF file */
    int32_t    numskip; /* number of shorts to skip in HRTF file */
                        /* short arrays into which HRTFs are stored locally */
    int16      sl[FILT_LEN], sr[FILT_LEN];
                        /* float versions of above to be sent to FFT routines */
    MYFLT      xl[BUF_LEN], xr[BUF_LEN];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t ii;

    if (UNLIKELY(p->mfp==NULL)) goto err1;         /* RWD fix */
        /* update local variables */
    kElev = p->kElev;
    kAz = p->kAz;
    elev = (int32_t) *kElev;
    azim = (int32_t) *kAz;
    //oldel_index = p->oldel_index;
    fpindex = (int16 *) p->fpbegin;
    flip = 0;

        /* Convert elevation in degrees to elevation array index. */
    el_index = ROUND((double)(elev - MIN_ELEV) / ELEV_INC);
    if (el_index < 0)
      el_index = 0;
    else if (el_index >= N_ELEV)
      el_index = N_ELEV-1;

        /* Convert azimuth in degrees to azimuth array index. */
    azim = (int32_t)fmod((double)azim, 360.0);
    if (azim < 0)
      azim += 360;
    if (azim > 180) {
      azim = 360 - azim;
      flip = 1; /* set to true */
    }
/*     else */
/*       flip = 0;  /\* it is still false *\/ */

        /* azim should be 0<=azim<=180 so calculate az_index and clip
           to legal range
           note - accesses global array elevation_data */
    az_index = ROUND((double)azim / (360.0 / elevation_data[el_index]));
    if (az_index < 0)
      az_index = 0;
    else if (az_index >= GET_NFAZ(el_index))
      az_index = GET_NFAZ(el_index) - 1;

        /* calculate offset into HRTFcompact file */
        /* first get to the first value of the requested elevation */
    numskip = 0;
    for (i = 0; i < el_index; i++)
      numskip += GET_NFAZ(i) * BUF_LEN;
    fpindex += numskip + az_index * BUF_LEN;

        /* read in (int16) data from stereo interleave HRTF file.
           Split into left and right channel data. */
    for (i=0; i<FILT_LEN; i++) {
      sl[i] = *fpindex++;
      sr[i] = *fpindex++;
    }
    {
      MYFLT scaleFac;
      /* Preserve the legacy gain; the manual calls for output compensation. */
      scaleFac = csound->GetInverseRealFFTScale(csound, BUF_LEN) / FL(256.0);
      scaleFac /= FL(32768.0);
      /* copy int16 buffers into float buffers */
      for (i=0; i<FILT_LEN; i++) {
        xl[i] = (MYFLT) sl[i] * scaleFac;
        xr[i] = (MYFLT) sr[i] * scaleFac;
      }
    }
    for (i=FILT_LEN; i<BUF_LEN; i++) {
      xl[i] = FL(0.0);     /* pad buffers with zeros to BUF_LEN */
      xr[i] = FL(0.0);
    }

        /**************
        FFT xl and xr here
        ***************/
    csound->RealFFT(csound, p->setup, xl);
    csound->RealFFT(csound, p->setup, xr);

        /* If azimuth called for right side of head, use left side
           measurements and flip output channels.
           This is due to the fact that the HRTFs we are using only
           include left side placements. */
    if (flip) {
      for (i=0; i<BUF_LEN; i++) {
        hrtf_data.left[i] = xr[i];
        hrtf_data.right[i] = xl[i];
      }
    }
    else {
      for (i=0; i<BUF_LEN; i++) {
        hrtf_data.left[i] = xl[i];
        hrtf_data.right[i] = xr[i];
      }
    }

        /* update local counters and indices */
    incount = p->incount;
    outcount = p->outcount;
    outfront = p->outfront;
    outend = p->outend;

    /* Get local pointers to structures */
    outl   = &p->outl[0];
    outr   = &p->outr[0];
    bl     = &p->bl[0];
    br     = &p->br[0];
    x      = &p->x[0];
    yl     = &p->yl[0];
    yr     = &p->yr[0];

    nsmpsi = nsmpso = CS_KSMPS - offset - early;
    aIn = p->aIn + offset;
    if (p->aIn == p->aLeft || p->aIn == p->aRight) {
      /* Draining queued output can otherwise overwrite unread input. */
      memcpy(p->auxch.auxp, aIn, nsmpsi * sizeof(MYFLT));
      aIn = (MYFLT *)p->auxch.auxp;
    }
    aLeft = p->aLeft + offset;
    aRight = p->aRight + offset;
    if (UNLIKELY(offset)) {
      memset(p->aLeft, 0, offset * sizeof(MYFLT));
      memset(p->aRight, 0, offset * sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      memset(p->aLeft + CS_KSMPS - early, 0, early * sizeof(MYFLT));
      memset(p->aRight + CS_KSMPS - early, 0, early * sizeof(MYFLT));
    }

        /* main loop for a-rate code.  Audio read in, processed,
           and output in this loop.  Loop exits when control period
           (ksmps) is finished.  */
    while (nsmpsi > 0) {
                /* determine how much audio may be read in */
      if ((incount + nsmpsi) <= FILT_LEN)
        toread = nsmpsi;
      else
        toread = FILT_LEN - incount;

                /* reading in audio into x */
      for (ii = 0; ii < toread; ii++)
        x[incount + ii] = *aIn++;

          /* update counters for amount of audio read */
      nsmpsi -= toread;
      incount += toread;

          /* loop for audio processing */
      if (incount == FILT_LEN) {
              /* enough audio for convolution - so do it! */
        incount = 0;
              /* pad x to BUF_LEN with zeros for Moore FFT */
        for (i = FILT_LEN; i <  BUF_LEN; i++)
          x[i] = FL(0.0);
        csound->RealFFT(csound, p->setup, x);

              /* complex multiplication, y = hrtf_data * x */
        csound->RealFFTMult(csound, yl, hrtf_data.left, x, BUF_LEN, FL(1.0));
        csound->RealFFTMult(csound, yr, hrtf_data.right, x, BUF_LEN, FL(1.0));

              /* convolution is the inverse FFT of above result (yl,yr) */
        csound->RealFFT(csound, p->isetup, yl);
        csound->RealFFT(csound, p->isetup, yr);
            /* overlap-add the results */
        for (i = 0; i < FILT_LENm1; i++) {
          yl[i] += bl[i];
          yr[i] += br[i];
          bl[i]  = yl[FILT_LEN+i];
          br[i]  = yr[FILT_LEN+i];
        }

        /* Each convolution produces half of the circular output buffer. */
        for (i = 0; i < FILT_LEN; i++) {
          outl[outend + i] = yl[i];
          outr[outend + i] = yr[i];
        }
        outcount += FILT_LEN;
        outend += FILT_LEN;
        if (outend == BUF_LEN) outend = 0;
      }

      /* Preserve this deprecated opcode's block scheduling: incomplete input
         frames produce no output, so latency and gaps depend on ksmps.
         Use hrtfstat for modern scheduling; do not silently change it here. */
      while (nsmpso > 0 && outcount > 0) {
        int32_t count = nsmpso < outcount ? nsmpso : outcount;
        if (count > BUF_LEN - outfront) count = BUF_LEN - outfront;
        for (i = 0; i < count; i++) {
          *aLeft++ = outl[outfront + i];
          *aRight++ = outr[outfront + i];
        }
        nsmpso -= count;
        outcount -= count;
        outfront += count;
        if (outfront == BUF_LEN) outfront = 0;
      }
    } /* end of control period loop - "while" */

    /* Clear output for which no complete input frame is available. */
    if (nsmpso > 0) {
      memset(aLeft, 0, nsmpso * sizeof(MYFLT));
      memset(aRight, 0, nsmpso * sizeof(MYFLT));
    }

        /* update state in p */
    p->outcount    = outcount;
    p->incount     = incount;
    p->outfront    = outfront;
    p->outend      = outend;
    p->oldel_index = el_index;

    return OK;
 err1:
    return csound->PerfError(csound, &(p->h), "%s",
                             Str("hrtfer: not initialised"));
}

static OENTRY hrtferX_localops[] =
  {
   CSOUND_DEPRECATED_OPCODE("hrtfer", "hrtfstat", FROZEN, "Legacy HRTFcompact behavior is frozen; hrtfstat uses different data and arguments. See PR #3009.")
   { "hrtfer",   sizeof(HRTFER), _QQ,  "aa", "akkS",
     (SUBR)hrtferxkSet, (SUBR)hrtferxk},
};

LINKAGE_BUILTIN(hrtferX_localops)
