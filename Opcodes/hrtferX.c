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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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


#include "csdl.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "hrtferx.h"

/* This array transferred here so as to be declared once.  Belongs to
   the structure of the HRTF data really in 3Dug.h */

int elevation_data[N_ELEV] = {56, 60, 72, 72, 72, 72, 72,
                              60, 56, 45, 36, 24, 12, 1 };

#define ROUND(x) ((int)floor((x)+FL(0.5)))
#define GET_NFAZ(el_index)      ((elevation_data[el_index] / 2) + 1)

static void cfft (MYFLT [], int, int);
void bitreverse (MYFLT [], int);
void rfft (MYFLT [], int, int);
static void cmult(MYFLT [], MYFLT [], MYFLT [], int);
extern int bytrevhost(void);

int hrtferxkSet(HRTFER *p)
{
    int    i; /* standard loop counter */
    char   filename[MAXNAME];
    MEMFIL *mfp;

        /* first check if orchestra's sampling rate is compatible with HRTF
           measurement's */
    if (esr != SAMP_RATE) {
      printf (Str(X_399,"Orchestra sampling rate is not compatible with HRTF.\n"));
      printf (Str(X_462,"Should be %d...exiting.\n"), SAMP_RATE);
      longjmp(p->h.insdshead->csound->exitjmp_,1);
    }

    if (*p->ifilno == SSTRCOD)
      strcpy(filename, p->STRARG);
    else {
      printf(Str(X_552,"\nLast argument must be the string 'HRTFcompact' "
             "...correcting.\n"));
      strcpy(filename, "HRTFcompact");
/*       longjmp(pcglob->exitjmp,1); */
    }

    if ((mfp = p->mfp) == NULL)
      mfp = ldmemfile(filename);
    p->mfp = mfp;
    p->fpbegin = (short *) mfp->beginp;
    if (bytrevhost()) {         /* Byte reverse on data set if necessary */
      short *x = p->fpbegin;
      long len = (mfp->length)/sizeof(short);
      while (len!=0) {
        short v = *x;
        v = ((v&0xFF)<<8) + ((v>>8)&0xFF); /* Swap bytes */
        *x = v;
        x++; len--;
      }
    }
#ifdef CLICKS
        /* initialize ramp arrays for crossfading */
    for (i=0; i<FILT_LEN; i++) {
        p->rampup[i] = (MYFLT)i / (MYFLT) FILT_LEN;
        p->rampdown[i] = FL(1.0) - p->rampup[i];
    }
#endif
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
    for (i=0; i<BUF_LEN; i++)   {
#ifdef CLICKS
      p->oldhrtf_data.left[i]  = FL(0.0);
      p->oldhrtf_data.right[i] = FL(0.0);
#endif
      p->x[i]                  = FL(0.0);
      p->yl[i]                 = FL(0.0);
      p->yr[i]                 = FL(0.0);
      p->outl[i]               = FL(0.0);
      p->outr[i]               = FL(0.0);
    }

        /* initialize left overlap buffer */
        /* initialize right overlap buffer */
    for (i=0; i<FILT_LENm1; i++) {
      p->bl[i] = FL(0.0);
      p->br[i] = FL(0.0);
    }

    return OK;
}

/********************** a-rate code ***********************************/

int hrtferxk(HRTFER *p)
{
    MYFLT      *aLeft, *aRight; /* audio output streams */
    MYFLT      *aIn, *kAz, *kElev; /* audio and control input streams */
    int        azim, elev, el_index, az_index,oldel_index, oldaz_index;
    int        nsmpsi, nsmpso; /* number of samples in/out */
    /*         input,      out-left,    out-right */
    MYFLT      *x, *yl, *yr;    /* Local copies of address */
    /*         overlap left,   overlap right */
    MYFLT      *bl, *br;
                        /* copy of current input convolved with old HRTFs */
#ifdef CLICKS
    MYFLT      yl2[BUF_LEN], yr2[BUF_LEN];
#endif
    MYFLT      *outl, *outr; /* output left/right */
    int        outfront, outend; /* circular output indices */
    int        incount, outcount; /* number of samples in/out */
    int        toread; /* number of samples to read */
    int        i; /* standard loop counter */
    HRTF_DATUM hrtf_data; /* local hrtf instances */
#ifdef CLICKS
    HRTF_DATUM oldhrtf_data;
    int        crossfadeflag; /* flag - true if crossfading old and new HRTFs */
#endif
    int        flip; /* flag - true if we need to flip the channels */
    short      *fpindex; /* pointer into HRTF file */
    short      numskip; /* number of shorts to skip in HRTF file */
                        /* short arrays into which HRTFs are stored locally */
    short      sl[FILT_LEN], sr[FILT_LEN];
                        /* float versions of above to be sent to FFT routines */
    MYFLT      xl[BUF_LEN], xr[BUF_LEN];


    if (p->mfp==NULL) {         /* RWD fix */
      perferror(Str(X_832,"hrtfer: not initialised"));
      return NOTOK;
    }
        /* update local variables */
    kElev = p->kElev;
    kAz = p->kAz;
    elev = (int) *kElev;
    azim = (int) *kAz;
    oldel_index = p->oldel_index;
    oldaz_index = p->oldaz_index;
    fpindex = (short *) p->fpbegin;
    flip = 0;
#ifdef CLICKS
    crossfadeflag = 0;
#endif


        /* Convert elevation in degrees to elevation array index. */
    el_index = ROUND((elev - MIN_ELEV) / ELEV_INC);
    if (el_index < 0)
      el_index = 0;
    else if (el_index >= N_ELEV)
      el_index = N_ELEV-1;

        /* Convert azimuth in degrees to azimuth array index. */
    azim = (int)fmod((double)azim, 360.0);
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
      fpindex = (short *) p->fpbegin;
    else
      for (i=0; i<=el_index; i++) {
        numskip = (short)(GET_NFAZ(i) * BUF_LEN);
        fpindex += numskip;
      }
        /* fpindex should now point to first azimuth at requested el_index */
        /* now get to first value of requested azimuth */
    if (az_index == 0) {
                                /*printf("in az_index == 0\n")*/
      numskip = 0;
    }
    else {
      for (i=0, numskip=0; i<az_index; i++)
        numskip += BUF_LEN;
      fpindex += (short) (numskip);
    }

#ifdef CLICKS
    if ((oldel_index != el_index) || (oldaz_index != az_index)) {
      crossfadeflag = 1;
    }
#endif
        /* read in (short) data from stereo interleave HRTF file.
           Split into left and right channel data. */
    for (i=0; i<FILT_LEN; i++) {
      sl[i] = *fpindex++;
      sr[i] = *fpindex++;
    }
    for (i=0; i<FILT_LEN; i++) {                        /* IV - Jul 11 2002 */
      xl[i] = (MYFLT)sl[i]/FL(32768.0); /* copy short buffers into
                                           float buffers */
      xr[i] = (MYFLT)sr[i]/FL(32768.0);
    }
    for (i=FILT_LEN; i<BUF_LEN; i++) {
      xl[i] = FL(0.0);     /* pad buffers with zeros to BUF_LEN */
      xr[i] = FL(0.0);
    }

        /**************
        FFT xl and xr here
        ***************/
    rfft(xl, FILT_LEN, 1);
    rfft(xr, FILT_LEN, 1);

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

#ifdef CLICKS
    for (i=0; i<FILT_LEN; i++) {
      oldhrtf_data.left[i] = p->oldhrtf_data.left[i];
      oldhrtf_data.right[i] = p->oldhrtf_data.right[i];
    }
#endif
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

    nsmpsi = ksmps;
    nsmpso = ksmps;

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
        for (i = 0; i < toread; i++)
          x[i] = *aIn++;
      }
      else {
        for (i = incount; i<(incount + toread); i++)
          x[i] = *aIn++;
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
        rfft(x, FILT_LEN, 1);

              /* complex multiplication, y = hrtf_data * x */
        cmult(yl, hrtf_data.left, x, BUF_LEN);
        cmult(yr, hrtf_data.right, x, BUF_LEN);

              /* convolution is the inverse FFT of above result (yl,yr) */
        rfft(yl, FILT_LEN, 0);
        rfft(yr, FILT_LEN, 0);

#ifdef CLICKS
        if (crossfadeflag) {    /* convolve current input with old HRTFs */
          /* ***** THIS CODE IS SERIOUSLY BROKEN ***** */
                  /* complex multiplication, y2 = oldhrtf_data * x */
          cmult(yl2, oldhrtf_data.left, x, BUF_LEN);
          cmult(yr2, oldhrtf_data.right, x, BUF_LEN);

                  /* convolution is the inverse FFT of above result (y) */
          rfft(yl2, FILT_LEN, 0);
          rfft(yr2, FILT_LEN, 0);

                  /* linear crossfade */
          for (i=0; i<FILT_LEN; i++) {
            yl[i] = yl[i]*p->rampup[i] + yl2[i]*p->rampdown[i];
            yr[i] = yr[i]*p->rampup[i] + yr2[i]*p->rampdown[i];
          }
        }
#endif
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
          outend += FILT_LEN;
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
          outend += -FILT_LEN;
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
                  /* account for circular reference */
          for (i = outfront; i < BUF_LEN; i++) {
            *aLeft++  =  outl[i];
            *aRight++ =  outr[i];
          }
          outcount -= nsmpso;
          nsmpso -= (BUF_LEN - outfront);
          for (i = 0; i < nsmpso; i++) {
            *aLeft++  =  outl[i];
            *aRight++ =  outr[i];
          }
          outfront = nsmpso;
          nsmpso = 0;
        }
      }
      else {
        if ((outfront+nsmpso) < BUF_LEN) {
          for (i = 0; i < outcount; i++) {
            *aLeft++  =  outl[outfront + i];
            *aRight++ =  outr[outfront + i];
          }
          nsmpso   -= outcount;
          outfront += outcount;
          outcount = 0;
        }
        else {
          /* account for circular reference */
          for (i = outfront; i < BUF_LEN; i++) {
            *aLeft++  =  outl[i];
            *aRight++ =  outr[i];
          }
          nsmpso   -= outcount;
          outcount -= (BUF_LEN - outfront);
          for (i = 0; i < outcount; i++) {
            *aLeft++  =  outl[i];
            *aRight++ =  outr[i];
          }
          outfront = outcount;
          outcount = 0;
        }
      } /* end of audio processing loop - "if" */
    } /* end of control period loop - "while" */


        /* update state in p */
    p->outcount = outcount;
    p->incount = incount;
    p->outfront = outfront;
    p->outend = outend;
    p->oldel_index = el_index;
    p->oldaz_index = az_index;

#ifdef CLICKS
    for (i=0; i<FILT_LEN; i++) {        /* archive current HRTFs */
      p->oldhrtf_data.left[i]  = hrtf_data.left[i];
      p->oldhrtf_data.right[i] = hrtf_data.right[i];
    }
#endif
/*     for (i=0; i<BUF_LEN; i++) { */
/*       p->outl[i] = outl[i]; */
/*       p->outr[i] = outr[i]; */
/*     } */

/*     for (i=0; i<FILT_LENm1; i++) { */
/*       p->bl[i] = bl[i]; */
/*       p->br[i] = br[i]; */
/*     } */

/*     for (i=0; i<BUF_LEN; i++) */
/*       p->x[i] = x[i]; */

/*     for (i=0; i<BUF_LEN; i++) { */
/*       p->yl[i] = yl[i]; */
/*       p->yr[i] = yr[i]; */
/*     } */
    return OK;
}


/*********************** FFT functions *****************
 * The functions cfft, rfft, and cmult are taken from the
 * code in Elements of Computer Music by F. R. Moore, pp. 81-88
 *******************************************************/

/********************cfft**********************************/
/*
 * cfft replaces float array x containing NC complex values
 * (2*NC float alternating real, imaginary, and so on)
 * by its Fourier transform if forward id true, or by its
 * inverse Fourier transform if forward is false. NC must be
 * a power of 2.
 */

static void cfft (MYFLT x[], int NC, int forward)
{
    MYFLT wr, wi, wpr, wpi, scale;
    double theta, sth;
    int mmax, ND, m, i, j, delta;

    ND = NC<<1;
    bitreverse (x, ND);
    for (mmax = 2; mmax < ND; mmax = delta) {
      delta = mmax+mmax;
      theta = TWOPI/(forward ? mmax : -mmax);
      sth = sin(0.5*theta);
      wpr = -FL(2.0)*(MYFLT)(sth * sth);
      wpi = (MYFLT)sin(theta);
      wr = FL(1.0);
      wi = FL(0.0);
      for (m = 0; m < mmax; m += 2) {
        MYFLT rtemp, itemp;
        for (i = m; i < ND; i += delta) {
          j       = i + mmax;
          rtemp   = wr*x[j] - wi*x[j+1];
          itemp   = wr*x[j+1] + wi*x[j];
          x[j]    = x[i] - rtemp;
          x[j+1]  = x[i+1] - itemp;
          x[i]   += rtemp;
          x[i+1] += itemp;
        }
        wr = (rtemp = wr)*wpr - wi*wpi + wr;
        wi = wi*wpr + rtemp*wpi + wi;
      }
    }
    /* scale the output */
    scale = forward ? FL(1.0)/ND : FL(2.0);
    for (i = 0; i < ND; i++)
      x[i] *= scale;
}


/* bitreverse places float array x containing N/2 complex values into
   bit-reversed order */
void bitreverse (MYFLT x[], int N)
{
    MYFLT rtemp, itemp;
    int i, j, m;

    for (i = j = 0; i < N; i += 2, j += m) {
      if (j>i) {
        rtemp = x[j]; itemp = x[j+1]; /* complex exchange */
        x[j] = x[i]; x[j+1] = x[i+1];
        x[i] = rtemp; x[i+1] = itemp;
      }
      for (m = N>>1; m >= 2 && j >= m; m >>= 1)
        j -= m;
    }
}

/********************rfft**********************************/
/* If forward is true, rfft replaces 2*N real data points
   with N complex values representing the positive frequency half
   of their Fourrier spectrum, with x[1] replaced with the real
   part of the Nyquist frequency values. If forward is false, rfft
   expects x to contain a * posistive frequency spectrum arranged
   as before, and replaces it with 2*N real values. N must be a power
   of 2.
*/

void rfft (MYFLT x[], int N, int forward)
{
    MYFLT c2, h1r, h1i, h2r, h2i, wr, wi, wpr, wpi, temp;
    MYFLT xr, xi;
    int i,i1, i2, i3, i4, N2p1;
    double theta, sth;

    theta = PI/N;
    wr = FL(1.0);
    wi = FL(0.0);
/*     c1 = FL(0.5); */
    if (forward) {
      c2    = -FL(0.5);
      cfft (x,N,forward);
      xr    = x[0];
      xi    = x[1];
    }
    else {
      c2    = FL(0.5);
      theta = -theta;
      xr    = x[1];
      xi    = FL(0.0);
      x[1]  = FL(0.0);
    }
    sth = sin(0.5*theta);
    wpr = -FL(2.0)*(MYFLT)(sth*sth);
    wpi = (MYFLT)sin(theta);
    N2p1 = (N<<1) + 1;
    for (i = 0; i <= N>>1; i++) {
      i1 = i<<1;
      i2 = i1 + 1;
      i3 = N2p1 - i2;
      i4 = i3 + 1;
      if (i==0) {
        h1r   = FL(0.5)*(x[i1] + xr);
        h1i   = FL(0.5)*(x[i2] - xi);
        h2r   = -c2*(x[i2] + xi);
        h2i   = c2*(x[i1] - xr);
        x[i1] = h1r + wr*h2r - wi*h2i;
        x[i2] = h1i + wr*h2i + wi*h2r;
        xr    = h1r - wr*h2r + wi*h2i;
        xi    = -h1i + wr*h2i + wi*h2r;
      }
      else {
        h1r   = FL(0.5)*(x[i1] + x[i3]);
        h1i   = FL(0.5)*(x[i2] - x[i4]);
        h2r   = -c2*(x[i2] + x[i4]);
        h2i   = c2*(x[i1] - x[i3]);
        x[i1] = h1r + wr*h2r - wi*h2i;
        x[i2] = h1i + wr*h2i + wi*h2r;
        x[i3] = h1r - wr*h2r + wi*h2i;
        x[i4] = -h1i + wr*h2i + wi*h2r;
      }
      wr = (temp = wr)*wpr - wi*wpi + wr;
      wi = wi*wpr + temp*wpi + wi;
    }
    if (forward)
      x[1] = xr;
    else
      cfft (x,N,forward);
}


/* c, a, and b are rfft-format spectra each containing n floats--
 * place complex product of a and b into c
 */
static void cmult( MYFLT c[], MYFLT a[], MYFLT b[], int n)
{
    int i,j;
    MYFLT re, im;

    c[0] = a[0] * b[0];
    c[1] = a[1] * b[1];
    for (i=2, j=3; i<n; i+=2, j+=2) {
      re = a[i] * b[i]  -  a[j] * b[j];
      im = a[i] * b[j]  +  a[j] * b[i];
      c[i] = re;
      c[j] = im;
    }
}

#define S       sizeof

static OENTRY localops[] = {
{ "hrtfer",   S(HRTFER),5, "aa", "akkS", (SUBR)hrtferxkSet, NULL, (SUBR)hrtferxk},
};

LINKAGE
