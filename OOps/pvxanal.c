/*
    pvxanal.c:

    Copyright (C) 2001 Richard Dobson, Mark Dolson

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

/* pvxanal.c. Multi-channel pvoc analysis utility for CSound.
 * functions init() and generate_frame() based on CARL pvoc (c) Mark Dolson.
 * the CARL software distribution is due to be released under the GNU LGPL.
 * This adaptation (c) Richard Dobson August 2001.
 */

#include "cs.h"
#include "soundio.h"
#include <math.h>
#include <ctype.h>
#include "pvxanal.h"
#include "pvfileio.h"


#define DEFAULT_BUFLEN (8192)   /* per channel */

/* Only supports PVOC_AMP_FREQ format for now */

void fft_(MYFLT *, MYFLT *,int,int,int,int);
void fftmx(MYFLT *,MYFLT *,int,int,int,int,int,int *,
           MYFLT *,MYFLT *,MYFLT *,MYFLT *,int *,int[]);
void reals_(MYFLT *,MYFLT *,int,int);
void pvx_release(PVX **ppvx,long chans);
static long generate_frame(PVX *pvx,MYFLT *fbuf,float *outanal,
                           long samps,int frametype);
static void chan_split(const MYFLT *inbuf,MYFLT **chbuf,long insize,long chans);
static int init(PVX **pvx,long srate,long fftsize,long winsize,
                long overlap,pv_wtype wintype);
/* from elsewhere in Csound! But special form for CARL code*/
void hamming(MYFLT *win,int winLen,int even);
double besseli(double x);
void kaiser(MYFLT *win,int len,double Beta);
void vonhann(MYFLT *win,int winLen,int even);

/* not sure how to use 'verbose' yet; but it's here...*/
/* cannot add display code, as we may have 8 channels here...*/
int pvxanal(SOUNDIN *p, SNDFILE *fd, const char *fname, long srate,
            long chans, long fftsize, long overlap, long winsize,
            pv_wtype wintype, int verbose)
{

    int i,k,pvfile = -1,rc = 0;
    pv_stype stype = STYPE_16;          /* we assume! */
    long buflen,buflen_samps;
    long sampsread;
    long blocks_written = 0;            /* m/c framecount for user */
    PVX *pvx[MAXPVXCHANS];
    MYFLT  *inbuf_c[MAXPVXCHANS];
    float *frame_c[MAXPVXCHANS];        /* RWD : MUST be 32bit  */
    MYFLT *inbuf = NULL;
    MYFLT nyquist,chwidth;
    float *frame;                       /* RWD : MUST be 32bit  */
    MYFLT *chanbuf;
    char *ext;
    unsigned int nbins;
    long total_sampsread = 0;

    ext = strrchr(fname,'.');

    if (ext == NULL || ext[0]!='.' || tolower(ext[1]) != 'p' ||
        tolower(ext[2]) != 'v' ||
        tolower(ext[3]) != 'x' || ext[4] != '\0') {
      printf(Str("pvxanal: outfile name must use extension .pvx\n"));
      return 1;
    }
    if (chans > MAXPVXCHANS) {
      printf(Str(
                 "pvxanal - source has too many channels: Maxchans = %d.\n"),
             MAXPVXCHANS);
      return 1;
    }

    nbins = (fftsize/2) + 1;
    nyquist = (MYFLT) srate * FL(0.5);
    chwidth = nyquist/(MYFLT)(nbins - 1);
    for (i=0;i < MAXPVXCHANS;i++) {
      pvx[i] = NULL;
      inbuf_c[i] = NULL;
      frame_c[i] = NULL;
    }

    /* TODO: save some memory and create analysis window once! */

    for (i=0; i < chans;i++)
      rc += init(&pvx[i],srate,fftsize,winsize,overlap,wintype);

    if (rc)
      goto error;

    /* alloc all buffers */
    buflen = DEFAULT_BUFLEN;
    /* snap to overlap size*/
    buflen = (buflen/overlap) * overlap;
    buflen_samps = buflen * chans;
    inbuf = (MYFLT *) malloc((buflen_samps) * sizeof(MYFLT));
    if (inbuf==NULL) {
      rc = 1;
      goto error;
    }
    for (i=0;i < chans;i++) {
      inbuf_c[i] = (MYFLT *) malloc(buflen * sizeof(MYFLT));
      if (inbuf_c[i]==NULL) {
        rc = 1;
        goto error;
      }
      frame_c[i] = (float*)malloc((fftsize+2)*sizeof(float)); /*RWD 32bit*/
      if (frame_c[i]==NULL) {
        rc = 1;
        goto error;
      }
    }

    pvfile  = pvoc_createfile(fname,fftsize,overlap,chans,
                              PVOC_AMP_FREQ,srate,stype,
                              wintype,0.0f,NULL,winsize);
    if (pvfile < 0) {
      printf(Str("pvxanal: unable to create analysis file: %s"),
             pvoc_errorstr());
      rc = 1;
      goto error;
    }

    while ((sampsread = getsndin(&cenviron, fd, inbuf, buflen_samps, p)) > 0) {
      total_sampsread += sampsread;
      /* zeropad to full buflen */
      if (sampsread < buflen_samps) {
        for (i = sampsread;i< buflen_samps;i++)
          inbuf[i] = FL(0.0);
        sampsread = buflen_samps;
      }
      chan_split(inbuf,inbuf_c,sampsread,chans);

      for (i=0;i < sampsread/chans; i+= overlap) {
        for (k=0;k < chans;k++) {
          frame = frame_c[k];
          chanbuf = inbuf_c[k];
          generate_frame(pvx[k],chanbuf+i,frame,overlap,PVOC_AMP_FREQ);

          if (!pvoc_putframes(pvfile,frame,1)) {
            printf(Str("pvxanal: error writing analysis frames: %s\n"),
                   pvoc_errorstr());
            rc = 1;
            goto error;
          }
          blocks_written++;
        }
      }
      if ((blocks_written/chans) %20 == 0) {
        printf("%ld\n",blocks_written/chans);
      }
      if (total_sampsread >= p->getframes*chans) break;
    }

    /* write out remaining frames */
    sampsread = fftsize * chans;
    for (i = 0;i< sampsread;i++)
      inbuf[i] = FL(0.0);
    chan_split(inbuf,inbuf_c,sampsread,chans);
    for (i=0;i < sampsread/chans; i+= overlap) {
      for (k=0;k < chans;k++) {
        frame = frame_c[k];
        chanbuf = inbuf_c[k];
        generate_frame(pvx[k],chanbuf+i,frame,overlap,PVOC_AMP_FREQ);
        if (!pvoc_putframes(pvfile,frame,1)) {
          printf(Str("pvxanal: error writing analysis frames: %s\n"),
                 pvoc_errorstr());
          rc = 1;
          goto error;
        }
        blocks_written++;
      }
    }
    printf(Str("\n%ld %d-chan blocks written to %s\n"),
           blocks_written/chans,chans,fname);

 error:
    if (inbuf)
      free(inbuf);
    pvx_release(pvx,chans);
    for (i=0;i < chans;i++) {
      if (inbuf_c[i])
        free(inbuf_c[i]);
      if (frame_c[i])
        free(frame_c[i]);
      if (pvx[i])
        free(pvx[i]);
    }

    if (pvfile >=0)
      pvoc_closefile(pvfile);
    return rc;
}

int init(PVX **pvx,long srate,long fftsize,long winsize,
         long overlap,pv_wtype wintype)
{
    int i;
    long N,N2,M,Mf,D;
    MYFLT sum;
    PVX *thispvx;

    if (pvx==NULL)
      return 1;

    thispvx  = (PVX *) malloc(sizeof(PVX));
    if (thispvx==NULL)
      return 1;
    /* init all vars, as for a constructor */
    thispvx->input          =  NULL;
    thispvx->anal           =  NULL;
    thispvx->banal          =  NULL; /* pointer to anal[1] (for FFT calls) */
    thispvx->nextIn         =  NULL; /* pointer to next empty word in input */
    thispvx->analWindow     =  NULL; /* pointer to center of analysis window */
    thispvx->analWindow_base = NULL;
    thispvx->i0             =  NULL; /* pointer to amplitude channels */
    thispvx->i1             =  NULL; /* pointer to frequency channels */
    thispvx->oi             =  NULL; /* pointer to old phase channels */
    thispvx->oldInPhase     =  NULL; /* pointer to start of input phase buffer */
    thispvx->ibuflen        = 0;     /* length of input buffer */

    thispvx->nI   = 0;          /* current input (analysis) sample */
    thispvx->beta = FL(6.8);    /* parameter for Kaiser window */
    thispvx->Mf   = 0;          /* flag for even M */
    thispvx->K  = 0;
    thispvx->vH = 0;
    N = fftsize;
    M = winsize;
    D = overlap;

    switch(wintype) {
    case PVOC_HANN:
      thispvx->H = 1;
      break;
    case PVOC_KAISER:
      thispvx->K = 1;
      break;
    default:
      /* for now, anything else just sets Hamming window! */
      break;
    }

    if (N <=0)
      return 1;
    if (D < 0)
      return 1;
    if (M < 0)
      return 1;

    thispvx->isr         = srate;
    thispvx->R           = (MYFLT) srate;
    thispvx->N           = N  + N%2;    /* Make N even */
    thispvx->N2          = N2 = N / 2;
    thispvx->Fexact      = (MYFLT)srate / (MYFLT)N;         /* RWD */
    thispvx->M           = M;
    thispvx->Mf          = Mf = 1 - M%2;
    thispvx->ibuflen     = 4 * M;

    D = (int)((D != 0 ? D : M/(8.0))); /* Why floating 8?? */
    if (D == 0) {
      /*fprintf(stderr,"pvoc: warning - T greater than M/8 \n"); */
      D = 1;
    }
    thispvx->D       = D;
    thispvx->I       = D;
    thispvx->nMin    = 0;       /* first input (analysis) sample */
    thispvx->nMax    = 100000000;
    thispvx->nMax   -= thispvx->nMin;

    /* set up analysis window: The window is assumed to be symmetric
       with M total points.  After the initial memory allocation,
       analWindow always points to the midpoint of the window
       (or one half sample to the right, if M is even); analWinLen
       is half the true window length (rounded down). Any low pass
       window will work; a Hamming window is generally fine,
       but a Kaiser is also available.  If the window duration is
       longer than the transform (M > N), then the window is
       multiplied by a sin(x)/x function to meet the condition:
       analWindow[Ni] = 0 for i != 0.  In either case, the
       window is renormalised so that the phase vocoder amplitude
       estimates are properly scaled.  The maximum allowable
       window duration is ibuflen/2. */


    thispvx->analWindow_base = (MYFLT *) malloc((M+Mf) * sizeof(MYFLT));
    if (thispvx->analWindow_base==NULL)
      return 1;

    thispvx->analWindow =
      thispvx->analWindow_base + (thispvx->analWinLen = thispvx->M/2);
#ifdef USE_FFTW
    in_fftw_size = N;
    out_fftw_size = NO;
    forward_plan =
      rfftwnd_create_plan_specific(1,&in_fftw_size,
                                   FFTW_REAL_TO_COMPLEX,
                                   FFTW_ESTIMATE | FFTW_IN_PLACE,
                                   analWindow_base,1,NULL,1);
    inverse_plan =
      rfftwnd_create_plan(1,&out_fftw_size, FFTW_COMPLEX_TO_REAL,
                          FFTW_ESTIMATE | FFTW_IN_PLACE);
#endif

    if (thispvx->K)
      kaiser(thispvx->analWindow_base,M+Mf,thispvx->beta); /* ??? or just M?*/
    else if (thispvx->vH)
      vonhann(thispvx->analWindow,thispvx->analWinLen,Mf);

    else
      hamming(thispvx->analWindow,thispvx->analWinLen,Mf);


    for (i = 1; i <= thispvx->analWinLen; i++)
      *(thispvx->analWindow - i) = *(thispvx->analWindow + i - Mf);

    if (M > N) {
      if (Mf)
        *thispvx->analWindow *=
          (MYFLT)((double)N*sin(PI*0.5/(double)N)/(PI*0.5));
      for (i = 1; i <= thispvx->analWinLen; i++)
        *(thispvx->analWindow + i) *=   /* D.T. 2000*/
          (MYFLT)((double)N*sin((double)(PI*(i+0.5*Mf)/N))/(PI*(i+0.5*Mf)));
      for (i = 1; i <= thispvx->analWinLen; i++)
        *(thispvx->analWindow - i) = *(thispvx->analWindow + i - Mf);
    }

    sum = FL(0.0);
    for (i = -thispvx->analWinLen; i <= thispvx->analWinLen; i++)
      sum += *(thispvx->analWindow + i);

    sum = FL(2.0) / sum;        /*factor of 2 comes in later in trig identity*/

    for (i = -thispvx->analWinLen; i <= thispvx->analWinLen; i++)
      *(thispvx->analWindow + i) *= sum;

    /* set up input buffer:  nextIn always points to the next empty
       word in the input buffer (i.e., the sample following
       sample number (n + analWinLen)).  If the buffer is full,
       then nextIn jumps back to the beginning, and the old
       values are written over. */

    thispvx->input = (MYFLT *) malloc(thispvx->ibuflen * sizeof(MYFLT));
    if (thispvx->input==NULL)
      goto err;
    thispvx->nextIn = thispvx->input;


    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
       so the other channels are redundant. oldInPhase is used
       in the conversion to remember the previous phase when
       calculating phase difference between successive samples. */

    thispvx->anal       =       (MYFLT *) malloc((N+2) * sizeof(MYFLT));
    if (thispvx->anal == NULL)
      goto err;
    thispvx->banal      =       thispvx->anal + 1;
    thispvx->oldInPhase =       (MYFLT *) malloc ((N2+1) * sizeof(MYFLT));
    if (thispvx->oldInPhase == NULL)
      goto err;

    thispvx->rIn = ((MYFLT) thispvx->R / D);
    thispvx->invR =(FL(1.0) / thispvx->R);
    thispvx->RoverTwoPi = thispvx->rIn / TWOPI_F;

    thispvx->nI = -(thispvx->analWinLen / D) * D;  /* input time (in samples) */

    thispvx->Dd = thispvx->analWinLen + thispvx->nI + 1; /* number of new inputs to read */
    thispvx->flag = 1;

    for (i=0;i < thispvx->ibuflen;i++)
      thispvx->input[i] = FL(0.0);

    for (i=0;i < N+2;i++)
      thispvx->anal[i] = FL(0.0);

    for (i=0;i < N2+1;i++)
      thispvx->oldInPhase[i] = FL(0.0);

    *pvx = thispvx;
    return 0;

 err:
    if (thispvx->analWindow_base) {
      free(thispvx->analWindow_base);
      thispvx->analWindow_base = NULL;

    }
    if (thispvx->input) {
      free(thispvx->input);
      thispvx->input = NULL;
    }


    if (thispvx->anal) {
      free(thispvx->anal);
      thispvx->anal = NULL;
    }
    if (thispvx->oldInPhase) {
      free(thispvx->oldInPhase);
      thispvx->oldInPhase = NULL;
    }

    return 1;
}


void pvx_release(PVX **ppvx,long chans)
{
    long i;
    PVX *pvx;
    if (ppvx==NULL)
      return;

    for (i=0;i < chans;i++) {
      pvx = ppvx[i];
      if (pvx==NULL)
        continue;
      if (pvx->analWindow_base) {
        free(pvx->analWindow_base);
        pvx->analWindow_base = NULL;
      }
      if (pvx->input) {
        free(pvx->input);
        pvx->input = NULL;
      }


      if (pvx->anal) {
        free(pvx->anal);
        pvx->anal = NULL;
      }
      if (pvx->oldInPhase) {
        free(pvx->oldInPhase);
        pvx->oldInPhase = NULL;
      }
    }
}

#define MAX(a,b) (a>b ? a : b)
#define MIN(a,b) (a<b ? a : b)
/* RWD outanal MUST be 32bit */
long generate_frame(PVX *pvx,MYFLT *fbuf,float *outanal,long samps,int frametype)
{
    /*sblen = decfac = D */
    /*static int sblen = 0; */
    int got, tocp,i,j,k;
    long N = pvx->N;
    MYFLT *fp,*oi,*i0,*i1,real,imag,*anal,*banal,angleDif;
    double rratio,phase;
    float *ofp;           /* RWD MUST be 32bit */
    anal = pvx->anal;
    banal = pvx->banal;

    got = samps;         /*always assume */
    if (got < pvx->Dd)
      pvx->Dd = got;

    fp = fbuf;


    tocp = MIN(got, pvx->input+pvx->ibuflen-pvx->nextIn);
    got -= tocp;
    while (tocp-- > 0)
      *pvx->nextIn++ = *fp++;

    if (got > 0) {
      pvx->nextIn -= pvx->ibuflen;
      while (got-- > 0)
        *pvx->nextIn++ = *fp++;
    }
    if (pvx->nextIn >= (pvx->input + pvx->ibuflen))
      pvx->nextIn -= pvx->ibuflen;

    if (pvx->nI > 0)
      for (i = pvx->Dd; i < pvx->D; i++) {      /* zero fill at EOF */
        *(pvx->nextIn++) = FL(0.0);
        if (pvx->nextIn >= (pvx->input + pvx->ibuflen))
          pvx->nextIn -= pvx->ibuflen;
      }

    /* analysis: The analysis subroutine computes the complex output at
       time n of (N/2 + 1) of the phase vocoder channels.  It operates
       on input samples (n - analWinLen) thru (n + analWinLen) and
       expects to find these in input[(n +- analWinLen) mod ibuflen].
       It expects analWindow to point to the center of a
       symmetric window of length (2 * analWinLen +1).  It is the
       responsibility of the main program to ensure that these values
       are correct!  The results are returned in anal as succesive
       pairs of real and imaginary values for the lowest (N/2 + 1)
       channels.   The subroutines fft and reals together implement
       one efficient FFT call for a real input sequence.  */


    for (i = 0; i < N+2; i++)
      *(anal + i) = FL(0.0);        /*initialize*/

    j = (pvx->nI - pvx->analWinLen-1+pvx->ibuflen)%pvx->ibuflen;  /*input pntr*/

    k = pvx->nI - pvx->analWinLen - 1;                  /*time shift*/
    while (k < 0)
      k += N;
    k = k % N;
    for (i = -pvx->analWinLen; i <= pvx->analWinLen; i++) {
      if (++j >= pvx->ibuflen)
        j -= pvx->ibuflen;
      if (++k >= N)
        k -= N;
      *(anal + k) += *(pvx->analWindow + i) * *(pvx->input + j);
    }
#ifdef USE_FFTW
    rfftwnd_one_real_to_complex(forward_plan,anal,NULL);
    /* reals_(anal,banal,N2,-2); */
#else
    fft_(anal,banal,1,pvx->N2,1,-2);
    reals_(anal,banal,pvx->N2,-2);
#endif
    /* conversion: The real and imaginary values in anal are converted to
       magnitude and angle-difference-per-second (assuming an
       intermediate sampling rate of rIn) and are returned in
       anal. */
    /* only support this format for now, in Csound */
    if (frametype==PVOC_AMP_FREQ) {
      for (i=0,i0=anal,i1=anal+1,oi=pvx->oldInPhase;
           i <= pvx->N2;
           i++,i0+=2,i1+=2, oi++) {
        real = *i0;
        imag = *i1;
        *i0 =(MYFLT) sqrt((double)(real * real + imag * imag));
        /* phase unwrapping */
        /*if (*i0 == 0.)*/
        if (*i0 < FL(1.0E-10))        /* RWD don't mess with v small numbers! */
          angleDif = FL(0.0);

        else {
          rratio = atan2((double)imag,(double)real);
          angleDif  = (MYFLT)((phase = rratio) - *oi);
          *oi = (MYFLT) phase;
        }

        if (angleDif > PI)
          angleDif = (MYFLT)(angleDif - TWOPI);
        if (angleDif < -PI)
          angleDif = (MYFLT)(angleDif + TWOPI);

        /* add in filter center freq.*/
        *i1 = angleDif * pvx->RoverTwoPi + ((MYFLT) i * pvx->Fexact);
      }
    }
    /* else must be PVOC_COMPLEX */
    fp = anal;
    ofp = outanal;
    for (i=0;i < N+2;i++)
      *ofp++ = (float) *fp++;  /* RWD need 32bit cast incase MYFLT is double */

    pvx->nI += pvx->D;                          /* increment time */
    pvx->Dd = MIN(pvx->D,                       /* CARL */
                  MAX(0, pvx->D + pvx->nMax - pvx->nI - pvx->analWinLen));

    return pvx->D;
}

void chan_split(const MYFLT *inbuf,MYFLT **chbuf,long insize,long chans)
{
    long i,j,len;
    MYFLT ampfac;
/*     static const MYFLT ampfac = FL(1.0) / FL(32768.0);  /\* for Csound ONLY! *\/ */
    MYFLT *buf_c[MAXPVXCHANS];
    const MYFLT *p_inbuf = inbuf;

    len = insize/chans;

    ampfac = FL(1.0) / cenviron.e0dbfs;

    for (i=0;i < chans;i++)
      buf_c[i] = chbuf[i];

    for (i = len;i;--i)
      for (j=0;j < chans;j++)
        *(buf_c[j]++) = ampfac * *p_inbuf++;

}


void hamming(MYFLT *win,int winLen,int even)
{
    double ftmp;
    int i;

    ftmp = PI/winLen;

    if (even) {
      for (i=0; i<winLen; i++)
        *(win+i) = (MYFLT)(0.54 + 0.46*cos(ftmp*((double)i+0.5)));
      *(win+winLen) = FL(0.0);
    }
    else {
      *(win) = FL(1.0);
      for (i=1; i<=winLen; i++)
        *(win+i) = (MYFLT)(0.54 + 0.46*cos(ftmp*(double)i));
    }

}

/* Was defined in fgens.c */
double besseli( double x)
{
    double ax, ans;
    double y;

    if (( ax = fabs( x)) < 3.75)     {
      y = x / 3.75;
      y *= y;
      ans = (1.0 + y * ( 3.5156229 +
                         y * ( 3.0899424 +
                               y * ( 1.2067492 +
                                     y * ( 0.2659732 +
                                           y * ( 0.360768e-1 +
                                                 y * 0.45813e-2))))));
    }
    else {
      y = 3.75 / ax;
      ans = ((exp ( ax) / sqrt(ax))
             * (0.39894228 +
                y * (0.1328592e-1 +
                     y * (0.225319e-2 +
                          y * (-0.157565e-2 +
                               y * (0.916281e-2 +
                                    y * (-0.2057706e-1 +
                                         y * (0.2635537e-1 +
                                              y * (-0.1647633e-1 +
                                                   y * 0.392377e-2)))))))));
    }
    return ans;
}

void kaiser(MYFLT *win, int len, double Beta)
{
    MYFLT *ft = win;
    double i, xarg = 1.0;        /*xarg = amp scalefactor */
    for (i = -len/2.0 + 0.1 ; i < len/2.0 ; i++) {
      double z = 2.0*i/(double)(len - 1);
      *ft++ = (MYFLT) (xarg *
                       besseli(Beta * sqrt(1.0-z*z)) /
                       besseli(Beta));
    }
    /*  assymetrical hack: sort out first value! */
    win[0] = win[len-1];
}

void vonhann(MYFLT *win, int winLen, int even)
{
    MYFLT ftmp;
    int i;

    ftmp = PI_F/winLen;

    if (even) {
      for (i=0; i<winLen; i++)
        *(win+i) = (MYFLT)(0.5 + 0.5 *cos(ftmp*((double)i+0.5)));
      *(win+winLen) = FL(0.0);
    }
    else {
      *(win) = FL(1.0);
      for (i=1; i<=winLen; i++)
        *(win+i) =(MYFLT)(0.5 + 0.5 *cos(ftmp*(double)i));
    }
}

