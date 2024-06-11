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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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

static int32_t hrtferxkSet(CSOUND *csound, HRTFER *p)
{
    // int32_t    i; /* standard loop counter */
    char   filename[MAXNAME];
    int32_t    bytrev_test;
    MEMFIL *mfp;

        /* first check if orchestra's sampling rate is compatible with HRTF
           measurement's */
    if (UNLIKELY(CS_ESR != SAMP_RATE)) {
      return csound->InitError(csound,
                  Str("Orchestra sampling rate is not compatible with HRTF.\n"
                      "Should be %d...exiting."), SAMP_RATE);
      return NOTOK; /* not reached */
    }

    if (!strcmp("HRTFcompact", p->ifilno->data)) {
      strncpy(filename, p->ifilno->data, MAXNAME);
      //filename[MAXNAME-1] = '\0';
    }
    else {
      csound->Message(csound, "%s", Str("\nLast argument must be the string "
                                  "'HRTFcompact' ...correcting.\n"));
      strncpy(filename, "HRTFcompact", MAXNAME); /* for safety */
    }

    if ((mfp = p->mfp) == NULL)
      mfp = csound->LoadMemoryFile(csound, filename, CSFTYPE_HRTF, NULL);
    p->mfp = mfp;
    p->fpbegin = (int16*) mfp->beginp;
    bytrev_test = 0x1234;
    if (*((unsigned char*) &bytrev_test) == (unsigned char) 0x34) {
      /* Byte reverse on data set if necessary */
      int16 *x = p->fpbegin;
      int32 len = (mfp->length)/sizeof(int16);
      while (len != 0) {
        int16 v = *x;
        v = ((v & 0xFF) << 8) + ((v >> 8) & 0xFF);  /* Swap bytes */
        *x = v;
        x++; len--;
      }
    }
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
    int16      numskip; /* number of shorts to skip in HRTF file */
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
    else if (az_index >= elevation_data[el_index])
      az_index = elevation_data[el_index] - 1;

        /* calculate offset into HRTFcompact file */
        /* first get to the first value of the requested elevation */
    if (el_index == 0)
      fpindex = (int16 *) p->fpbegin;
    else
      for (i=0; i<=el_index; i++) {
        numskip = (int16)(GET_NFAZ(i) * BUF_LEN);
        fpindex += numskip;
      }
        /* fpindex should now point to first azimuth at requested el_index */
        /* now get to first value of requested azimuth */
    if (az_index == 0) {
   /* csound->Message(csound, "in az_index == 0\n"); */
      //numskip = 0;
    }
    else {
      for (i=0, numskip=0; i<az_index; i++)
        numskip += BUF_LEN;
      fpindex += (int16) (numskip);
    }

        /* read in (int16) data from stereo interleave HRTF file.
           Split into left and right channel data. */
    for (i=0; i<FILT_LEN; i++) {
      sl[i] = *fpindex++;
      sr[i] = *fpindex++;
    }
    {
      MYFLT scaleFac;
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

    aIn    = p->aIn;
    aLeft  = p->aLeft;
    aRight = p->aRight;

    nsmpsi =  nsmpso = CS_KSMPS;

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
      if (incount == 0) {
        for (ii = 0; ii < toread; ii++)
          x[ii] = *aIn++;
      }
      else {
        for (ii = incount; ii<(incount + toread); ii++)
          x[ii] = *aIn++;
      }

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

              /* put convolution ouput into circular output buffer */
        if (outend <= FILT_LEN) {
                  /* output will fit in buffer boundaries, therefore
                     no circular reference required */
          for (i = outend; i < (outend + FILT_LEN); i++) {
            outl[i] = yl[i-outend];
            outr[i] = yr[i-outend];
          }
          outcount += FILT_LEN;
          outend   += FILT_LEN;
        }
        else {
                  /* circular reference required due to buffer boundaries */
          for (i = outend; i < BUF_LEN; i++) {
            outl[i] = yl[i-outend];
            outr[i] = yr[i-outend];
          }
          for (i = 0; i < (-FILT_LEN + outend); i++) {
            outl[i] = yl[(BUF_LEN-outend) + i];
            outr[i] = yr[(BUF_LEN-outend) + i];
          }
          outcount += FILT_LEN;
          outend   -= FILT_LEN;
        }
      }

          /* output audio to audio stream.
                  Can only output one control period (ksmps) worth of samples */
      if (nsmpso < outcount) {
        if ((outfront+nsmpso) < BUF_LEN) {
          for (i = 0; i < nsmpso; i++) {
            *aLeft++ = outl[outfront + i];
            *aRight++ = outr[outfront + i];
          }
          outcount -= nsmpso;
          outfront += nsmpso;
          nsmpso = 0;
        }
        else {
          uint32 j = 0;
                  /* account for circular reference */
          for (i = outfront; i < BUF_LEN; i++, j++) {
            *aLeft++  = (j<offset || j>early) ? FL(0.0) : outl[i];
            *aRight++ = (j<offset || j>early) ? FL(0.0) : outr[i];
          }
          outcount -= nsmpso;
          nsmpso -= (BUF_LEN - outfront);
          for (i = 0; i < nsmpso; i++, j++) {
            *aLeft++  = (j<offset || j>early) ? FL(0.0) : outl[i];
            *aRight++ = (j<offset || j>early) ? FL(0.0) : outr[i];
          }
          outfront = nsmpso;
          nsmpso = 0;
        }
      }
      else {
        uint32 j = 0;
        if ((outfront+nsmpso) < BUF_LEN) {
          for (i = 0; i < outcount; i++, j++) {
            *aLeft++  =  (j<offset || j>early) ? FL(0.0) : outl[outfront + i];
            *aRight++ =  (j<offset || j>early) ? FL(0.0) : outr[outfront + i];
          }
          nsmpso   -= outcount;
          outfront += outcount;
          outcount = 0;
        }
        else {
          /* account for circular reference */
          for (i = outfront; i < BUF_LEN; i++, j++) {
            *aLeft++  =  (j<offset || j>early) ? FL(0.0) : outl[i];
            *aRight++ =  (j<offset || j>early) ? FL(0.0) : outr[i];
          }
          nsmpso   -= outcount;
          outcount -= (BUF_LEN - outfront);
          for (i = 0; i < outcount; i++, j++) {
            *aLeft++  =  (j<offset || j>early) ? FL(0.0) : outl[i];
            *aRight++ =  (j<offset || j>early) ? FL(0.0) : outr[i];
          }
          outfront = outcount;
          outcount = 0;
        }
      } /* end of audio processing loop - "if" */
    } /* end of control period loop - "while" */

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
   { "hrtfer",   sizeof(HRTFER), _QQ,  "aa", "akkS",
     (SUBR)hrtferxkSet, (SUBR)hrtferxk},
};

LINKAGE_BUILTIN(hrtferX_localops)
