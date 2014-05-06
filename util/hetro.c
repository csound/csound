/*
    hetro.c:

    Copyright (C) 1992 Tom Sullivan, Richard Dobson, John ffitch

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

#include "std_util.h"                                   /*  HETRO.C   */
#include "soundio.h"
#include <math.h>

#ifndef WIN32
#include <unistd.h>
#endif
#define INCSDIF 1

#if INCSDIF
/*RWD need to set this to prevent sdif.h including windows.h */
#define _WINDOWS_
/* CNMAT sdif library, subject to change..... */
#include "SDIF/sdif.h"
#include "SDIF/sdif-mem.h"
typedef struct {
    sdif_float32 index, freq, amp, phase;
} SDIF_RowOf1TRC;

static int is_sdiffile(char *name);
#endif

#define SQRTOF3 1.73205080756887729352
#define SQUELCH 0.5     /* % of max ampl below which delta_f is frozen */
#define HMAX    50

/* Authors:   Tom Sullivan, Nov'86, Mar'87;  bv revised Jun'92, Aug'92  */
/* Function:  Fixed frequency heterodyne filter analysis.               */
/* Simplifications and partial recoding by John Fitch Dec 1994 */
/* SDIF extensions by Richard Dobson, Aug 2000 */

/* lowest heterodyne freq = sr/bufsiz */

typedef struct {
  MYFLT    x1,x2,yA,y2,y3;      /* lpf coefficients*/
  MYFLT    cur_est,             /* current freq. est.*/
           freq_est, max_frq, max_amp,/* harm freq. est. & max vals found */
           fund_est,            /* fundamental est.*/
           t,                   /* fundamental period est.*/
           delta_t, outdelta_t, /* sampling period, outpnt period */
           sr,                  /* sampling rate */
           freq_c,              /* filter cutoff freq.*/
           beg_time, input_dur,
                                /* begin time & sample input duration*/
           **MAGS, **FREQS;     /* magnitude and freq. output buffers*/

  double *cos_mul, *sin_mul,    /* quad. term buffers*/
         *a_term, *b_term,      /*real & imag. terms*/
         *r_ampl,               /* pt. by pt. amplitude buffer*/
         *r_phase,              /* pt. by pt. phase buffer*/
         *a_avg,                /* output dev. freq. buffer*/
         new_ph,                /* new phase value*/
         old_ph,                /* previous phase value*/
         jmp_ph,                /* for phase unwrap*/
         *ph_av1, *ph_av2, *ph_av3,      /*tempor. buffers*/
         *amp_av1, *amp_av2, *amp_av3,   /* same for ampl.*/
         m_ampsum;              /* maximum amplitude at output*/
  int32  windsiz;               /* # of pts. in one per. of sample*/
  int16  hmax;                  /* max harmonics requested */
  int    num_pts,               /* breakpoints per harmonic */
         amp_min;               /* amplitude cutout threshold */
  int    skip,                  /* flag to stop analysis if zeros*/
         bufsiz;                /* circular buffer size */
  int32  smpsin;                /* num sampsin */
  int32  midbuf,                /* set to bufsiz / 2   */
         bufmask;               /* set to bufsiz - 1   */
  char   *infilnam,             /* input file name */
         *outfilnam;            /* output file name */
  MYFLT  *auxp;                 /* pointer to input file */
  MYFLT  *adp;                  /* pointer to front of sample file */
  double *c_p,*s_p;             /* pointers to space for sine and cos terms */
  int    newformat;             /* flag for m/c independent format */
} HET;

#if INCSDIF
static int writesdif(CSOUND*, HET*);
#endif
static  double  GETVAL(HET *, double *, int32);
static  double  sq(double);
static  void    PUTVAL(HET *,double *, int32, double);
static  int     hetdyn(CSOUND *csound, HET *, int);
static  void    lpinit(HET*);
static  void    lowpass(HET *,double *, double *, int32);
static  void    average(HET *,int32, double *, double *, int32);
static  void    output(HET *,int32, int, int);
static  void    output_ph(HET *, int32);
static  int     filedump(HET *, CSOUND *);
static  int     quit(CSOUND *, char *);

#define sgn(x)  (x<0.0 ? -1 : 1)
#define u(x)    (x>0.0 ? 1 : 0)

#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-'))  \
                            return quit(csound, MSG);

static void init_het(HET *thishet)
{
    thishet->freq_est  = FL(0.0);
    thishet->fund_est  = FL(100.0);
    thishet->sr        = FL(0.0);       /* sampling rate */
    thishet->freq_c    = FL(0.0);       /* filter cutoff freq.*/
    thishet->beg_time  = FL(0.0);
    thishet->input_dur = FL(0.0);
    thishet->old_ph    = 0.0;           /* previous phase value*/
    thishet->jmp_ph    = 0.0;           /* for phase unwrap*/
    thishet->m_ampsum  = 32767.0;       /* maximum amplitude at output*/
    thishet->hmax      = 10;            /* max harmonics requested */
    thishet->num_pts   = 256;           /* breakpoints per harmonic */
    thishet->amp_min   = 64;            /* amplitude cutout threshold */
    thishet->bufsiz    = 1;             /* circular buffer size */
    thishet->skip      = 0;             /* JPff: this was missing */
    thishet->newformat = 1;
}

static int hetro(CSOUND *csound, int argc, char **argv)
{
    SNDFILE *infd;
    int     i, hno, channel = 1, retval = 0;
    int32   nsamps, smpspc, bufspc, mgfrspc;
    char    *dsp, *dspace, *mspace;
    double  *begbufs, *endbufs;
    HET     het;
    HET     *thishet = &het;
    SOUNDIN *p;         /* space allocated by SAsndgetset() */

 /* csound->dbfs_to_float = csound->e0dbfs = FL(1.0);   Needed ? */
    init_het(thishet);

    if (!(--argc)) {
      return quit(csound,Str("no arguments"));
    }
    do {
      char *s = *++argv;
      if (*s++ == '-')
        switch (*s++) {
        case 's':
          FIND(Str("no sampling rate"))
#if defined(USE_DOUBLE)
          csound->sscanf(s,"%lf",&thishet->sr);
#else
          csound->sscanf(s,"%f",&thishet->sr);
#endif
          break;
        case 'c':
          FIND(Str("no channel"))
          sscanf(s,"%d",&channel);
          break;
        case 'b':
          FIND(Str("no begin time"))
#if defined(USE_DOUBLE)
          csound->sscanf(s,"%lf",&thishet->beg_time);
#else
          csound->sscanf(s,"%f",&thishet->beg_time);
#endif
          break;
        case 'd':
          FIND(Str("no duration time"))
#if defined(USE_DOUBLE)
          csound->sscanf(s,"%lf",&thishet->input_dur);
#else
          csound->sscanf(s,"%f",&thishet->input_dur);
#endif
          break;
        case 'f':
          FIND(Str("no fundamental estimate"))
#if defined(USE_DOUBLE)
          csound->sscanf(s,"%lf",&thishet->fund_est);
#else
          csound->sscanf(s,"%f",&thishet->fund_est);
#endif
          break;
        case 'h':
          FIND(Str("no harmonic count"))
          sscanf(s,"%hd",&thishet->hmax);
          if (thishet->hmax > HMAX)
            csound->Message(csound,Str("over %d harmonics but continuing"),
                            HMAX);
          if (thishet->hmax < 1) {
            csound->Message(csound,Str("h of %d too low, reset to 1\n"),
                            thishet->hmax);
                thishet->hmax = 1;
          }
          break;
        case 'M':
          FIND(Str("no amplitude maximum"))
          csound->sscanf(s,"%lf",&thishet->m_ampsum);
          break;
        case 'm':
          FIND(Str("no amplitude minimum"))
          sscanf(s,"%d",&thishet->amp_min);
          break;
        case 'n':
          FIND(Str("no number of output points"))
          sscanf(s,"%d",&thishet->num_pts);
          break;
        case 'l':
          FIND(Str("no filter cutoff"))
#if defined(USE_DOUBLE)
          csound->sscanf(s,"%lf",&thishet->freq_c);
#else
          csound->sscanf(s,"%f",&thishet->freq_c);
#endif
          break;
        case 'X':
          het.newformat = 1;
          break;
        case 'x':
          het.newformat = 0;
          break;
        case '-':
          FIND(Str("no log file"));
          while (*s++)
               ; s--;
          break;
        default:
          return quit(csound, Str("Invalid switch option"));
        }
      else break;
    } while (--argc);

    if (argc != 2)
      return quit(csound, Str("incorrect number of filenames"));
    thishet->infilnam = *argv++;
    thishet->outfilnam = *argv;

    if (thishet->freq_c > 1)
      csound->Message(csound, Str("Filter cutoff freq. = %f\n"),
                              thishet->freq_c);

    if ((thishet->input_dur < 0) || (thishet->beg_time < 0))
      return quit(csound,Str("input and begin times cannot be less than zero"));
    /* open sndfil, do skiptime */
    if ((infd = csound->SAsndgetset(csound, thishet->infilnam, &p,
                                    &thishet->beg_time, &thishet->input_dur,
                                    &thishet->sr, channel)) == NULL) {
      char errmsg[256];
      snprintf(errmsg, 256, Str("Cannot open %s"), thishet->infilnam);
      return quit(csound, errmsg);
    }
    nsamps = p->getframes;
    /* alloc for MYFLTs */
    thishet->auxp = (MYFLT*) csound->Malloc(csound, nsamps * sizeof(MYFLT));
    /* & read them in */
    if ((thishet->smpsin = csound->getsndin(csound, infd,
                                            thishet->auxp, nsamps, p)) <= 0) {
      char errmsg[256];
      csound->Message(csound, "smpsin = %ld\n", (long) thishet->smpsin);
      snprintf(errmsg, 256, Str("Read error on %s\n"), thishet->infilnam);
      return quit(csound, errmsg);
    }
    thishet->sr = (MYFLT) p->sr;                /* sr now from open  */
    /* samps in fund prd */
    thishet->windsiz = (int32)(thishet->sr / thishet->fund_est + FL(0.5));
#if INCSDIF
    /* RWD no limit for SDIF files! */
    if (is_sdiffile(thishet->outfilnam)) {
      if (thishet->num_pts >= nsamps - thishet->windsiz)
        return quit(csound, Str("number of output points is too great"));
    }
    else
#endif
      if (thishet->num_pts > 32767 ||
          thishet->num_pts >= nsamps - thishet->windsiz)
        return quit(csound, Str("number of output points is too great"));
    thishet->delta_t = FL(1.0)/thishet->sr;
    thishet->t = FL(1.0)/thishet->fund_est;
    thishet->outdelta_t = (MYFLT) thishet->num_pts
                          / (thishet->smpsin - thishet->windsiz);

    while (thishet->bufsiz < (thishet->sr/thishet->fund_est + FL(0.5)))
      thishet->bufsiz *= 2;
    thishet->midbuf = thishet->bufsiz/2;
    thishet->bufmask = thishet->bufsiz - 1;

    smpspc = thishet->smpsin * sizeof(double);
    bufspc = thishet->bufsiz * sizeof(double);

    dsp = dspace = csound->Malloc(csound, smpspc * 2 + bufspc * 13);
    thishet->c_p = (double *) dsp;      dsp += smpspc;  /* space for the    */
    thishet->s_p = (double *) dsp;      dsp += smpspc;  /* quadrature terms */
    begbufs = (double *) dsp;
    thishet->cos_mul = (double *) dsp;  dsp += bufspc;  /* bufs that will be */
    thishet->sin_mul = (double *) dsp;  dsp += bufspc;  /* refilled each hno */
    thishet->a_term = (double *) dsp;   dsp += bufspc;
    thishet->b_term = (double *) dsp;   dsp += bufspc;
    thishet->r_ampl = (double *) dsp;   dsp += bufspc;
    thishet->ph_av1 = (double *) dsp;   dsp += bufspc;
    thishet->ph_av2 = (double *) dsp;   dsp += bufspc;
    thishet->ph_av3 = (double *) dsp;   dsp += bufspc;
    thishet->r_phase = (double *) dsp;  dsp += bufspc;
    thishet->amp_av1 = (double *) dsp;  dsp += bufspc;
    thishet->amp_av2 = (double *) dsp;  dsp += bufspc;
    thishet->amp_av3 = (double *) dsp;  dsp += bufspc;
    thishet->a_avg = (double *) dsp;    dsp += bufspc;
    endbufs = (double *) dsp;

    mgfrspc = thishet->num_pts * sizeof(MYFLT);
    dsp = mspace = csound->Malloc(csound, mgfrspc * thishet->hmax * 2);
    thishet->MAGS = (MYFLT **) csound->Malloc(csound,
                                              thishet->hmax * sizeof(MYFLT*));
    thishet->FREQS = (MYFLT **) csound->Malloc(csound,
                                               thishet->hmax * sizeof(MYFLT*));
    for (i = 0; i < thishet->hmax; i++) {
      thishet->MAGS[i] = (MYFLT *) dsp;    dsp += mgfrspc;
      thishet->FREQS[i] = (MYFLT *) dsp;   dsp += mgfrspc;
    }
    lpinit(thishet);                        /* calculate LPF coeffs.  */
    thishet->adp = thishet->auxp;           /* point to beg sample data block */
    for (hno = 0; hno < thishet->hmax; hno++) { /* for requested harmonics */
      double *dblp;
      thishet->freq_est += thishet->fund_est; /*   do analysis */
      thishet->cur_est = thishet->freq_est;
      dblp = begbufs;
      do {
        *dblp++ = FL(0.0);                    /* clear all refilling buffers */
      } while (dblp < endbufs);
      thishet->max_frq = FL(0.0);
      thishet->max_amp = FL(0.0);

      csound->Message(csound,Str("analyzing harmonic #%d\n"),hno);
      csound->Message(csound,Str("freq estimate %6.1f,"), thishet->cur_est);
      if (hetdyn(csound, thishet, hno) != 0)  /* perform actual computation */
        return -1;
      if (!csound->CheckEvents(csound))
        return -1;
      csound->Message(csound, Str(" max found %6.1f, rel amp %6.1f\n"),
                              thishet->max_frq, thishet->max_amp);
    }
    csound->Free(csound, dspace);
#if INCSDIF
    /* RWD if extension is .sdif, write as 1TRC frames */
    if (is_sdiffile(thishet->outfilnam)) {
      if (!writesdif(csound,thishet)) {
        csound->Message(csound, Str("Unable to write to SDIF file\n"));
        retval = -1;
      }
    }
    else
#endif
      retval |= filedump(thishet, csound);  /* write output to adsyn file */

    return retval;
}

static double GETVAL(HET* thishet, double *inb, int32 smpl)
{                               /* get value at position smpl in array inb */
    if (smpl<0) return 0.0;
    return   inb[(smpl + thishet->midbuf) & thishet->bufmask];
}

static void PUTVAL(HET* thishet, double *outb, int32 smpl, double value)
{                               /* put value in array outb at postn smpl */
    outb[(smpl + thishet->midbuf) & thishet->bufmask] = value;
}

static int hetdyn(CSOUND *csound, HET* thishet, int hno) /* HETERODYNE FILTER */
{
    int32   smplno;
    double  temp_a, temp_b, tpidelest;
    double  *cos_p, *sin_p, *cos_wp, *sin_wp;
    int32   n;
    int     outpnt, lastout = -1;
    MYFLT   *ptr;

    thishet->jmp_ph = 0;                     /* set initial phase to 0 */
    temp_a = temp_b = 0;
    cos_p = thishet->c_p;
    sin_p = thishet->s_p;
    tpidelest = TWOPI * thishet->cur_est * thishet->delta_t;
    for (smplno = 0; smplno < thishet->smpsin; smplno++) {
      double phase = smplno * tpidelest;     /* do all quadrature calcs */
      ptr = thishet->adp + smplno;           /* at once and point to it */
      *cos_p++ = (double)(*ptr) * cos(phase);
      *sin_p++ = (double)(*ptr) * sin(phase);
    }

    cos_p = cos_wp = thishet->c_p;
    sin_p = sin_wp = thishet->s_p;
    for (smplno = 0; smplno < thishet->smpsin - thishet->windsiz; smplno++) {
      if (smplno == 0 && thishet->smpsin >= thishet->windsiz) {
        /* for first smplno */
        n = thishet->windsiz;
        do {
          temp_a += *cos_wp++;     /* sum over windsiz = nsmps in */
          temp_b += *sin_wp++;     /*    1 period of fund. freq.  */
        } while (--n);
      }
      else {      /* if more than 1 fund. per. away from file end */
                  /* remove front value and add on new rear value */
                  /* to obtain summation term for new sample! */
        if (smplno <= thishet->smpsin - thishet->windsiz) {
          temp_a += (*cos_wp++ - *cos_p++);  /* _wp = _p + windsiz */
          temp_b += (*sin_wp++ - *sin_p++);
        }
        else {
          thishet->skip = 1;
          temp_a = temp_b = 0;
        }
      }
      /* store values into buffers */
      PUTVAL(thishet, thishet->cos_mul, smplno, temp_a);
      PUTVAL(thishet, thishet->sin_mul, smplno, temp_b);
      if ((thishet->freq_c <= 1) || (smplno < 3)) {
        average(thishet, thishet->windsiz, thishet->cos_mul, thishet->a_term,
                         smplno); /* average over previous */
        average(thishet, thishet->windsiz, thishet->sin_mul, thishet->b_term,
                         smplno); /* values 1 fund prd ago */
      }
      else {
        lowpass(thishet, thishet->a_term,thishet->cos_mul,smplno);
        lowpass(thishet, thishet->b_term,thishet->sin_mul,smplno);
      }
      output_ph(thishet, smplno);       /* calculate mag. & phase for sample */
      if ((outpnt = (int)(smplno * thishet->outdelta_t)) > lastout) {
        /* if next out-time */
        output(thishet, smplno, hno, outpnt);  /*     place in     */
        lastout = outpnt;                      /*     output array */
        if (!csound->CheckEvents(csound))
          return -1;
      }
      if (thishet->skip) {
        thishet->skip = 0;       /* quit if no more samples in file */
        break;
      }
    }

    return 0;
}

static void lpinit(HET *thishet) /* lowpass coefficient ititializer */
{               /* 3rd order butterworth LPF coefficients calculated using */
                /* impulse invariance */
    MYFLT costerm,sinterm;
    double omega_c;

    omega_c = thishet->freq_c*TWOPI;
    costerm = (MYFLT)cos(SQRTOF3*omega_c*thishet->delta_t/2.0);
    sinterm = (MYFLT)sin(SQRTOF3*omega_c*thishet->delta_t/2.0);
    thishet->x1 = (MYFLT)(omega_c*thishet->delta_t*
                          (exp(-omega_c*thishet->delta_t) +
                           exp(-omega_c*thishet->delta_t/2.0)
                           * (-costerm + sinterm/SQRTOF3)));
    thishet->x2 = (MYFLT)(omega_c*thishet->delta_t*
                          (exp(-omega_c*thishet->delta_t) -
                           exp(-3*omega_c*thishet->delta_t/2)
                           * (costerm + sinterm/SQRTOF3)));
    thishet->yA = (-((MYFLT)exp(-omega_c*thishet->delta_t) +
            FL(2.0)*(MYFLT)exp(-omega_c*thishet->delta_t/2)*costerm));
    thishet->y2 = FL(2.0) * (MYFLT)exp(-3.0*omega_c*thishet->delta_t/2.0)*costerm +
            (MYFLT)exp(-omega_c*thishet->delta_t);
    thishet->y3 = (-(MYFLT)exp(-2.0*omega_c*thishet->delta_t));
}

static void lowpass(HET *thishet, double *out, double *in, int32 smpl)
  /* call with x1,x2,yA,y2,y3 initialised  */
  /* calls LPF function */
{
    PUTVAL(thishet, out, smpl,
           (thishet->x1 *
            GETVAL(thishet,in,smpl-1) + thishet->x2 * GETVAL(thishet,in,smpl-2) -
            thishet->yA * GETVAL(thishet,out,smpl-1) - thishet->y2 *
            GETVAL(thishet,out,smpl-2) -
            thishet->y3 * GETVAL(thishet,out,smpl-3)));
}

static void average(HET *thishet, int32 window,double *in,double *out, int32 smpl)
  /* AVERAGES OVER 'WINDOW' SAMPLES */
  /* this is actually a comb filter with 'Z' */
  /* transform of (1/w *[1 - Z**-w]/[1 - Z**-1]) */
  /* ie. zeros at all harmonic frequencies except*/
  /* the current one where the pole cancels it */
{
    if (smpl==0) return;
    if (smpl<window)
      PUTVAL(thishet,out, smpl,
             (double)(GETVAL(thishet,out,smpl-1) +
                      (1.0/(double)window) * (GETVAL(thishet,in,smpl))));
    else
      PUTVAL(thishet,out, smpl,
             (double)(GETVAL(thishet,out,smpl-1) +
                      (1.0/(double)window) *
                      (GETVAL(thishet,in,smpl) - GETVAL(thishet,in,smpl-window))));
}

                                 /* update phase counter */
static void output_ph(HET *thishet,int32 smpl)
                                /* calculates magnitude and phase components */
                                /* for each samples quadrature components, & */
                                /* and unwraps the phase.  A phase difference*/
{                               /* is taken to represent the freq. change.   */
    double      delt_temp;      /* the pairs are then comb filtered.         */
    double      temp_a;

    if ((temp_a=GETVAL(thishet,thishet->a_term,smpl)) == 0)
            thishet->new_ph=
              (-PI/FL(2.0))*sgn(GETVAL(thishet,thishet->b_term,smpl));
    else thishet->new_ph=
           -atan(GETVAL(thishet,thishet->b_term,smpl)/temp_a) - PI*u(-temp_a);

    if (fabs((double)thishet->new_ph - thishet->old_ph)>PI)
      thishet->jmp_ph -= TWOPI*sgn(temp_a);

    thishet->old_ph = thishet->new_ph;
    PUTVAL(thishet,thishet->r_phase,smpl,thishet->old_ph+thishet->jmp_ph);
    delt_temp = ((GETVAL(thishet,thishet->r_phase,smpl) -
                  GETVAL(thishet,thishet->r_phase,smpl-1))/
                 (TWOPI*thishet->delta_t));
    if ((thishet->freq_c <= 1) || (smpl < 3)) {
      PUTVAL(thishet,thishet->amp_av1,smpl,
             (MYFLT)sqrt(sq(GETVAL(thishet,thishet->a_term,smpl))
                         + sq(GETVAL(thishet,thishet->b_term,smpl))));
      average(thishet, thishet->windsiz,thishet->amp_av1,thishet->amp_av2,smpl);
      average(thishet, thishet->windsiz,thishet->amp_av2,thishet->amp_av3,smpl);
      average(thishet, thishet->windsiz,thishet->amp_av3,thishet->r_ampl,smpl);
      PUTVAL(thishet,thishet->ph_av1,smpl,delt_temp);
      average(thishet, thishet->windsiz,thishet->ph_av1,thishet->ph_av2,smpl);
      average(thishet, thishet->windsiz,thishet->ph_av2,thishet->ph_av3,smpl);
      average(thishet, thishet->windsiz,thishet->ph_av3,thishet->a_avg,smpl);
    }
    else {
      PUTVAL(thishet,thishet->r_ampl,smpl,
             (MYFLT)sqrt(sq(GETVAL(thishet,thishet->a_term,smpl))
                         + sq(GETVAL(thishet,thishet->b_term,smpl))));
      PUTVAL(thishet,thishet->a_avg,smpl,delt_temp);
    }
}

static void output(HET *thishet, int32 smpl, int hno, int pnt)
                        /* output one freq_mag pair */
                        /* when called, gets frequency change */
                        /* and adds it to current freq. stores*/
{                       /* current amp and new freq in arrays */
    double delt_freq;
    MYFLT  new_amp, new_freq;

    if (pnt < thishet->num_pts) {
      delt_freq = GETVAL(thishet,thishet->a_avg,smpl); /* 0.5 for rounding ? */
      thishet->FREQS[hno][pnt] =
        new_freq = (MYFLT)(delt_freq + thishet->cur_est);
      thishet->MAGS[hno][pnt] =
        new_amp = (MYFLT)GETVAL(thishet, thishet->r_ampl,smpl);
      if (new_freq > thishet->max_frq)
        thishet->max_frq = new_freq;
      if (new_amp > thishet->max_amp)
        thishet->max_amp = new_amp;
    }
}

/* If this function worthwhile?  Need to coinsider recalculation */
inline static double sq(double num)     /* RETURNS SQUARE OF ARGUMENT */
{
    return (num * num);
}

static int quit(CSOUND *csound, char *msg)
{
    csound->ErrorMsg(csound, Str("hetro:  %s\n\tanalysis aborted"), msg);
    return -1;
}

#define END 32767

/* WRITE OUTPUT FILE in DATA-REDUCED format */

static int filedump(HET *thishet, CSOUND *csound)
{
    int     h, pnt, ofd, nbytes;    double  scale,x,y;
    int16   **mags, **freqs, *magout, *frqout;
    double  ampsum, maxampsum = 0.0;
    int32   lenfil = 0;
    int16   *TIME;
    MYFLT   timesiz;
    FILE    *ff;

    mags = (int16 **) csound->Malloc(csound, thishet->hmax * sizeof(int16*));
    freqs = (int16 **) csound->Malloc(csound, thishet->hmax * sizeof(int16*));
    for (h = 0; h < thishet->hmax; h++) {
      mags[h] = (int16 *)csound->Malloc(csound,
                                        thishet->num_pts * sizeof(int16));
      freqs[h] = (int16 *)csound->Malloc(csound,
                                         thishet->num_pts * sizeof(int16));
    }

    TIME = (int16 *)csound->Malloc(csound, thishet->num_pts * sizeof(int16));
    timesiz = FL(1000.0) * thishet->input_dur /thishet-> num_pts;
    for (pnt = 0; pnt < thishet->num_pts; pnt++)
      TIME[pnt] = (int16)(pnt * timesiz);

    /* fullpath else cur dir */
    if (thishet->newformat) {
      if (csound->FileOpen2(csound, &ff, CSFILE_STD, thishet->outfilnam,
                              "w", "", CSFTYPE_HETROT, 0) == NULL)
      return quit(csound, Str("cannot create output file\n"));
    } else
      if (csound->FileOpen2(csound, &ofd, CSFILE_FD_W, thishet->outfilnam,
                            NULL, "", CSFTYPE_HETRO, 0) == NULL)
        return quit(csound, Str("cannot create output file\n"));

    if (thishet->newformat)
      fprintf(ff,"HETRO %d\n", thishet->hmax);        /* Header */
    else {
      if (UNLIKELY(write(ofd, (char*)&thishet->hmax, sizeof(thishet->hmax))<0))
        csound->Message(csound,Str("Write failure\n")); /* Write header */
    }
    for (pnt=0; pnt < thishet->num_pts; pnt++) {
      ampsum = 0.0;
      for (h = 0; h < thishet->hmax; h++)
        ampsum += thishet->MAGS[h][pnt];
      if (ampsum > maxampsum)
        maxampsum = ampsum;
    }
    scale = thishet->m_ampsum / maxampsum;
    csound->Message(csound,Str("scale = %f\n"), scale);

    for (h = 0; h < thishet->hmax; h++) {
      for (pnt = 0; pnt < thishet->num_pts; pnt++) {
        x = thishet->MAGS[h][pnt] * scale;
        mags[h][pnt] = (int16)(x*u(x));
        y = thishet->FREQS[h][pnt];
        freqs[h][pnt] = (int16)(y*u(y));
      }
    }

    magout = (int16 *)csound->Malloc(csound,
                                     (thishet->num_pts+1) * 2 * sizeof(int16));
    frqout = (int16 *)csound->Malloc(csound,
                                     (thishet->num_pts+1) * 2 * sizeof(int16));
    for (h = 0; h < thishet->hmax; h++) {
      int16 *mp = magout, *fp = frqout;
      int16 *lastmag, *lastfrq, pkamp = 0;
      int mpoints, fpoints, contig = 0;
      *mp++ = -1;                      /* set brkpoint type codes  */
      *fp++ = -2;
      lastmag = mp;
      lastfrq = fp;
      for (pnt = 0; pnt < thishet->num_pts; pnt++) {
        int16 tim, mag, frq;
        tim = TIME[pnt];
        frq = freqs[h][pnt];
        if ((mag = mags[h][pnt]) > pkamp)
          pkamp = mag;
        if (mag > thishet->amp_min) {
          if (contig > 1) {        /* if third time this value  */
            if ((mag == *(mp-1) && mag == *(mp-3))
                /*    or 2nd time this slope */
                || (MYFLT)(mag - *(mp-1)) / (tim - *(mp-2)) ==
                (MYFLT)(*(mp-1) - *(mp-3)) / (*(mp-2) - *(mp-4)))
              mp -= 2;              /* overwrite the previous */
            if ((frq == *(fp-1) && frq == *(fp-3))
                || (MYFLT)(frq - *(fp-1)) / (tim - *(fp-2)) ==
                (MYFLT)(*(fp-1) - *(fp-3)) / (*(fp-2) - *(fp-4)))
              fp -= 2;
          }
          *mp++ = tim;
          *mp++ = mag;
          *fp++ = tim;
          *fp++ = frq;
          lastmag = mp;         /* record last significant seg  */
          lastfrq = fp;
          contig++;
        }
        else {
          if (mp > lastmag) {   /* for non-significant segments */
            mp = lastmag + 2; /*   only two points necessary  */
            fp = lastfrq + 2; /*   to peg the ends            */
          }
          *mp++ = tim;
          *mp++ = 0;
          *fp++ = tim;
          *fp++ = frq;
          contig = 0;
        }
      }
      if (lastmag < mp) {          /* if last signif not last point */
        mp = lastmag + 2;        /*   make it the penultimate mag */
        fp = lastfrq + 2;
      }
      *(mp-1) = 0;                 /* force the last mag to zero    */
      if (fp - frqout > 3)
        *(fp-1) = *(fp-3);       /*   & zero the freq change      */
      *mp++ = END;                 /* add the sequence delimiters   */
      *fp++ = END;
      mpoints = ((mp - magout) / 2) - 1;
      nbytes = (mp - magout) * sizeof(int16);
      if (thishet->newformat) {
        int i;
        for (i=0; i<(mp - magout); i++)
          fprintf(ff,"%hd%c", magout[i], i==(mp-magout-1)?'\n':',');
      }
      else {
        if (UNLIKELY(write(ofd, (char *)magout, nbytes)<0))
          csound->Message(csound, Str("Write failure\n"));
      }
#ifdef DEBUG
      {
        int i;
        for (i=0; i<(mp-magout); i++)
          csound->Message(csound, "%hd,", magout[i]);
        csound->Message(csound, "\n");
      }
#endif
      lenfil += nbytes;
      fpoints = ((fp - frqout) / 2) - 1;
      nbytes = (fp - frqout) * sizeof(int16);
      if (thishet->newformat) {
        int i;
        for (i=0; i<fp - frqout; i++)
          fprintf(ff,"%hd%c", frqout[i], i==(fp-frqout-1)?'\n':',');
        fprintf(ff,"\n");
      }
      else {
        if (UNLIKELY(write(ofd, (char *)frqout, nbytes)<0))
          csound->Message(csound, Str("Write failure\n"));
      }
#ifdef DEBUG
      {
        int i;
        for (i=0; i<(fp-frqout); i++)
          csound->Message(csound, "%hd,", frqout[i]);
        csound->Message(csound, "\n");
      }
#endif
      lenfil += nbytes;
      csound->Message(csound,
                      Str("harmonic #%d:\tamp points %d, \tfrq points %d,"
                          "\tpeakamp %d\n"),
                      h, mpoints, fpoints, pkamp);
    }
    csound->Message(csound,Str("wrote %ld bytes to %s\n"),
                    (long)lenfil, thishet->outfilnam);
    csound->Free(csound, magout);
    csound->Free(csound, frqout);
    csound->Free(csound, TIME);
    for (h = 0; h < thishet->hmax; h++) {
      csound->Free(csound, mags[h]);
      csound->Free(csound, freqs[h]);
    }
    csound->Free(csound, mags);
    csound->Free(csound, freqs);

    return 0;
}

#if INCSDIF
/* simply writes the number of frames generated - no data reduction,
   no interpolation */

static int writesdif(CSOUND *csound, HET *thishet)
{
    int         i,j,h, pnt;
    double      scale;
    double      ampsum, maxampsum = 0.0;
    MYFLT       timesiz;
    SDIFresult  r;
    SDIF_FrameHeader head;
    SDIF_MatrixHeader mh;
    FILE        *sdiffile = NULL;

    if (SDIF_Init() != ESDIF_SUCCESS) {
      csound->Message(csound,
                      Str("OOPS: SDIF does not work on this machine!\n"));
      return 0;
    }

    /* esential rescaling, from filedump() above  */
    for (pnt=0; pnt < thishet->num_pts; pnt++) {
      ampsum = 0.0;
      for (h = 0; h < thishet->hmax; h++)
        ampsum += thishet->MAGS[h][pnt];
      if (ampsum > maxampsum)
        maxampsum = ampsum;
    }
    scale = thishet->m_ampsum / maxampsum;
    /* SDIF does not specify a range, 'cos it's too clever for that sort
     * of thing, but this seems consistent with existing examples! */
    scale *= (double) (1.0/csound->Get0dBFS(csound));

    for (h = 0; h < thishet->hmax; h++) {
      for (pnt = 0; pnt < thishet->num_pts; pnt++) {
        thishet->MAGS[h][pnt] *= (MYFLT) scale;
        /* skip code to force positive values, for now */
      }
    }

    if ((r = SDIF_OpenWrite(thishet->outfilnam, &sdiffile))!=ESDIF_SUCCESS) {
      /* can get SDIF error messages, but trickly for CSTRINGS */
      csound->Message(csound,Str("Error creating %s\n"),thishet->outfilnam);
      fclose(sdiffile);
      return 0;
    }
    csound->NotifyFileOpened(csound, thishet->outfilnam, CSFTYPE_SDIF, 1, 0);

    SDIF_Copy4Bytes(head.frameType,"1TRC");
    head.streamID = SDIF_UniqueStreamID();
    head.matrixCount = 1;                       /*  data matrix */
    /* frame size =  sizeof (data matrix) + frame header fields*/
    /* add main header fields: time,ID,matrixcount */
    head.size = sizeof(sdif_float64) + 2 * sizeof(sdif_int32);
    /* add header block of a matrix: ID,datatype,nrows,ncols  */
    head.size += 4 * sizeof(sdif_int32);
    /* add size of data  */
    /* row count can be different each matrix, in SDIF, but not here! */
    head.size += 4 * sizeof(sdif_float32) * thishet->hmax;
    /* no padding bytes */
    /*timesiz = 1000.0f * thishet->input_dur / thishet->num_pts;*/
    timesiz = thishet->input_dur / thishet->num_pts;
    /* do not even need a TIME array */

    /* main loop to write 1TRC frames */
    for (i=0; i < thishet->num_pts; i++) {
      sdif_float32 amp,freq,phase = 0.0f;
      /* cannot offer anything interesting with phase! */
      head.time = (sdif_float32) ((MYFLT)i * timesiz);
      if ((r = SDIF_WriteFrameHeader(&head,sdiffile))!=ESDIF_SUCCESS) {
        csound->Message(csound,Str("Error writing SDIF frame header.\n"));
        return 0;
      }
      /*setup data matrix */
      mh.rowCount = thishet->hmax ;
      mh.columnCount = 4;
      SDIF_Copy4Bytes(mh.matrixType,"1TRC");
      mh.matrixDataType = SDIF_FLOAT32;
      if ((r = SDIF_WriteMatrixHeader(&mh,sdiffile))!=ESDIF_SUCCESS) {
        csound->Message(csound,Str("Error writing SDIF matrix header.\n"));
        return 0;
      }
      for (j=0;j < thishet->hmax;j++) {
        sdif_float32 index;
        /* zero index not used in SDIF */
        index = (sdif_float32)(j+1);
        amp = (sdif_float32) thishet->MAGS[j][i];
        freq = (sdif_float32) thishet->FREQS[j][i];
        if (((r = SDIF_Write4(&index,1,sdiffile))!= ESDIF_SUCCESS) ||
            ((r = SDIF_Write4(&freq,1,sdiffile))!= ESDIF_SUCCESS)  ||
            ((r = SDIF_Write4(&amp,1,sdiffile))!= ESDIF_SUCCESS)   ||
            ((r = SDIF_Write4(&phase,1,sdiffile))!= ESDIF_SUCCESS)) {
          csound->Message(csound,Str("Error writing SDIF data.\n"));
          return 0;
        }
      }
      /* 64-bit alignment can be relied upon here, so no need to calc padding */
    }
    csound->Message(csound,
                    Str("wrote %d 1TRC frames to %s\n"),
                    thishet->num_pts, thishet->outfilnam);
    SDIF_CloseWrite(sdiffile);
    return 1;
}

static int is_sdiffile(char *name)
{
    char *dot;
    if (name==NULL || strlen(name) < 6)
      return 0;
    dot = strrchr(name,'.');
    if (dot==NULL)
      return 0;
    if (strcmp(dot,".sdif")==0 ||
        strcmp(dot,".SDIF")==0)
      return 1;
    return 0;
}
#endif

/* module interface */

int hetro_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "hetro", hetro);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "hetro",
                                             Str("Soundfile analysis for adsyn"));
    }
    return retval;
}
