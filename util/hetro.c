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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "std_util.h"                                   /*  HETRO.C   */
#include "soundio.h"
#include <math.h>
#include <inttypes.h>

//#define DEBUG 1

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

static int32_t is_sdiffile(char *name);
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
  int32_t windsiz;               /* # of pts. in one per. of sample*/
  int16  hmax;                  /* max harmonics requested */
  int32_t num_pts,               /* breakpoints per harmonic */
         amp_min;               /* amplitude cutout threshold */
  int32_t skip,                  /* flag to stop analysis if zeros*/
         bufsiz;                /* circular buffer size */
  int32_t smpsin;                /* num sampsin */
  int32_t midbuf,                /* set to bufsiz / 2   */
         bufmask;               /* set to bufsiz - 1   */
  char   *infilnam,             /* input file name */
         *outfilnam;            /* output file name */
  MYFLT  *auxp;                 /* pointer to input file */
  MYFLT  *adp;                  /* pointer to front of sample file */
  double *c_p,*s_p;             /* pointers to space for sine and cos terms */
  int32_t newformat;             /* flag for m/c independent format */
} HET;

#if INCSDIF
static int32_t writesdif(CSOUND*, HET*);
#endif
static  double  GETVAL(HET *, double *, int32);
//static  double  sq(double);
static  void    PUTVAL(HET *,double *, int32, double);
static  int32_t hetdyn(CSOUND *csound, HET *, int32_t);
static  void    lpinit(HET*);
static  void    lowpass(HET *,double *, double *, int32);
static  void    average(HET *,int32, double *, double *, int32);
static  void    output(HET *,int32, int32_t, int32_t);
static  void    output_ph(HET *, int32);
static  int32_t filedump(HET *, CSOUND *);
static  int32_t quit(CSOUND *, char *);

#define sgn(x)  (x<0.0 ? -1 : 1)
#define u(x)    (x>0.0 ? 1 : 0)

#define FIND(MSG)   if (*s == '\0')  \
    if (UNLIKELY(!(--argc) || ((s = *++argv) && *s == '-')))    \
      return quit(csound, MSG);

static void init_het(HET *t)
{
    t->freq_est  = FL(0.0);
    t->fund_est  = FL(100.0);
    t->sr        = FL(0.0);       /* sampling rate */
    t->freq_c    = FL(0.0);       /* filter cutoff freq.*/
    t->beg_time  = FL(0.0);
    t->input_dur = FL(0.0);
    t->old_ph    = 0.0;           /* previous phase value*/
    t->jmp_ph    = 0.0;           /* for phase unwrap*/
    t->m_ampsum  = 32767.0;       /* maximum amplitude at output*/
    t->hmax      = 10;            /* max harmonics requested */
    t->num_pts   = 256;           /* breakpoints per harmonic */
    t->amp_min   = 64;            /* amplitude cutout threshold */
    t->bufsiz    = 1;             /* circular buffer size */
    t->skip      = 0;             /* JPff: this was missing */
    t->newformat = 1;
}

static int32_t hetro(CSOUND *csound, int32_t argc, char **argv)
{
    SNDFILE *infd;
    int32_t i, hno, channel = 1, retval = 0;
    int32   nsamps, smpspc, bufspc, mgfrspc;
    char    *dsp, *dspace;
    double  *begbufs, *endbufs;
    HET     het;
    HET     *t = &het;
    SOUNDIN *p;         /* space allocated by SAsndgetset() */

 /* csound->dbfs_to_float = csound->e0dbfs = FL(1.0);   Needed ? */
    init_het(t);

    if (UNLIKELY(!(--argc))) {
      return quit(csound,Str("no arguments"));
    }
    do {
      char *s = *++argv;
      if (*s++ == '-')
        switch (*s++) {
        case 's':
          FIND(Str("no sampling rate"))
#if defined(USE_DOUBLE)
          csound->Sscanf(s,"%lf",&t->sr);
#else
          csound->Sscanf(s,"%f",&t->sr);
#endif
          break;
        case 'c':
          FIND(Str("no channel"))
          csound->Sscanf(s,"%d",&channel);
          break;
        case 'b':
          FIND(Str("no begin time"))
#if defined(USE_DOUBLE)
          csound->Sscanf(s,"%lf",&t->beg_time);
#else
          csound->Sscanf(s,"%f",&t->beg_time);
#endif
          break;
        case 'd':
          FIND(Str("no duration time"))
#if defined(USE_DOUBLE)
          csound->Sscanf(s,"%lf",&t->input_dur);
#else
          csound->Sscanf(s,"%f",&t->input_dur);
#endif
          break;
        case 'f':
          FIND(Str("no fundamental estimate"))
#if defined(USE_DOUBLE)
          csound->Sscanf(s,"%lf",&t->fund_est);
#else
          csound->Sscanf(s,"%f",&t->fund_est);
#endif
          break;
        case 'h':
          FIND(Str("no harmonic count"))
          csound->Sscanf(s,"%hd",&t->hmax);
          if (UNLIKELY(t->hmax > HMAX))
            csound->Message(csound, Str("over %d harmonics but continuing"),
                            HMAX);
          if (UNLIKELY(t->hmax < 1)) {
            csound->Message(csound,Str("h of %d too low, reset to 1\n"),
                            t->hmax);
                t->hmax = 1;
          }
          break;
        case 'M':
          FIND(Str("no amplitude maximum"))
          csound->Sscanf(s,"%lf",&t->m_ampsum);
          break;
        case 'm':
          FIND(Str("no amplitude minimum"))
          csound->Sscanf(s,"%d",&t->amp_min);
          break;
        case 'n':
          FIND(Str("no number of output points"))
          csound->Sscanf(s,"%d",&t->num_pts);
          break;
        case 'l':
          FIND(Str("no filter cutoff"))
#if defined(USE_DOUBLE)
          csound->Sscanf(s,"%lf",&t->freq_c);
#else
          csound->Sscanf(s,"%f",&t->freq_c);
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
          while (*s++) {}; s--;
          break;
        default:
          return quit(csound, Str("Invalid switch option"));
        }
      else break;
    } while (--argc);

    if (UNLIKELY(argc != 2))
      return quit(csound, Str("incorrect number of filenames"));
    t->infilnam = *argv++;
    t->outfilnam = *argv;

    if (UNLIKELY(t->freq_c > 1))
      csound->Message(csound, Str("Filter cutoff freq. = %f\n"),
                              t->freq_c);

    if (UNLIKELY((t->input_dur < 0) || (t->beg_time < 0)))
      return quit(csound,Str("input and begin times cannot be less than zero"));
    /* open sndfil, do skiptime */
    if (UNLIKELY((infd = (csound->GetUtility(csound))->SndinGetSetSA(csound, t->infilnam, &p,
                                    &t->beg_time, &t->input_dur,
                                             &t->sr, channel)) == NULL)) {
      char errmsg[256];
      snprintf(errmsg, 256, Str("Cannot open %s"), t->infilnam);
      return quit(csound, errmsg);
    }
    nsamps = (int32_t) p->getframes;
    /* alloc for MYFLTs */
    t->auxp = (MYFLT*) csound->Malloc(csound, nsamps * sizeof(MYFLT));
    /* & read them in */
    if (UNLIKELY((t->smpsin =
                  (csound->GetUtility(csound))->Sndin(csound, infd,
                                   t->auxp, nsamps, p)) <= 0)) {
      char errmsg[256];
      csound->Message(csound, "smpsin = %"PRId64"\n", (int64_t) t->smpsin);
      snprintf(errmsg, 256, Str("Read error on %s\n"), t->infilnam);
      return quit(csound, errmsg);
    }
    t->sr = (MYFLT) p->sr;                /* sr now from open  */
    /* samps in fund prd */
    t->windsiz = (int32)(t->sr / t->fund_est /*+ FL(0.5)*/);
    //printf("widsize = %d\n", t->windsiz);
#if INCSDIF
    /* RWD no limit for SDIF files! */
    if (is_sdiffile(t->outfilnam)) {
      if (UNLIKELY(t->num_pts >= nsamps - t->windsiz))
        return quit(csound, Str("number of output points is too great"));
    }
    else
#endif
      if (UNLIKELY(t->num_pts > 32767 ||
                   t->num_pts >= nsamps - t->windsiz))
        return quit(csound, Str("number of output points is too great"));
    t->delta_t = FL(1.0)/t->sr;
    t->t = FL(1.0)/t->fund_est;
    t->outdelta_t = (MYFLT) t->num_pts
                          / (t->smpsin - t->windsiz);
    //printf("sizes: delta_t = %f t = %f outdelta = %f\n",
    //       t->delta_t, t->t, t->outdelta_t);
    while (t->bufsiz < t->windsiz)
      t->bufsiz *= 2;
    t->midbuf = t->bufsiz/2;
    t->bufmask = t->bufsiz - 1;

    smpspc = t->smpsin * sizeof(double);
    bufspc = t->bufsiz * sizeof(double);
//printf("sizes2: smpspc - %d  bufspc - %d\n", smpspc, bufspc);
    dsp = dspace = csound->Calloc(csound, smpspc * 2 + bufspc * 13);
    t->c_p = (double *) dsp;      dsp += smpspc;  /* space for the    */
    t->s_p = (double *) dsp;      dsp += smpspc;  /* quadrature terms */
    begbufs = (double *) dsp;
    t->cos_mul = (double *) dsp;  dsp += bufspc;  /* bufs that will be */
    t->sin_mul = (double *) dsp;  dsp += bufspc;  /* refilled each hno */
    t->a_term = (double *) dsp;   dsp += bufspc;
    t->b_term = (double *) dsp;   dsp += bufspc;
    t->r_ampl = (double *) dsp;   dsp += bufspc;
    t->ph_av1 = (double *) dsp;   dsp += bufspc;
    t->ph_av2 = (double *) dsp;   dsp += bufspc;
    t->ph_av3 = (double *) dsp;   dsp += bufspc;
    t->r_phase = (double *) dsp;  dsp += bufspc;
    t->amp_av1 = (double *) dsp;  dsp += bufspc;
    t->amp_av2 = (double *) dsp;  dsp += bufspc;
    t->amp_av3 = (double *) dsp;  dsp += bufspc;
    t->a_avg = (double *) dsp;    dsp += bufspc;
    endbufs = (double *) dsp;

    mgfrspc = t->num_pts * sizeof(MYFLT);
    dsp = csound->Malloc(csound, mgfrspc * t->hmax * 2);
    t->MAGS = (MYFLT **) csound->Malloc(csound,
                                              t->hmax * sizeof(MYFLT*));
    t->FREQS = (MYFLT **) csound->Malloc(csound,
                                               t->hmax * sizeof(MYFLT*));
    for (i = 0; i < t->hmax; i++) {
      t->MAGS[i] = (MYFLT *) dsp;    dsp += mgfrspc;
      t->FREQS[i] = (MYFLT *) dsp;   dsp += mgfrspc;
    }
    lpinit(t);                        /* calculate LPF coeffs.  */
    t->adp = t->auxp;           /* point to beg sample data block */
    for (hno = 0; hno < t->hmax; hno++) { /* for requested harmonics */
      double *dblp;
      t->freq_est += t->fund_est; /*   do analysis */
      t->cur_est = t->freq_est;
      dblp = begbufs;
      // TODO? memset(begbufs, '\0', (endbufs-begbufs)*sizeof(double));
      do {
        *dblp++ = FL(0.0);                    /* clear all refilling buffers */
      } while (dblp < endbufs);
      t->max_frq = FL(0.0);
      t->max_amp = -FL(1.0);

      csound->Message(csound,Str("analyzing harmonic #%d\n"),hno);
      csound->Message(csound,Str("freq estimate %6.1f,"), t->cur_est);
      if (hetdyn(csound, t, hno) != 0)  /* perform actual computation */
        return -1;
      if (!csound->CheckEvents(csound))
        return -1;
      csound->Message(csound, Str(" max found %6.1f, rel amp %6.1f\n"),
                              t->max_frq, t->max_amp);
    }
    csound->Free(csound, dspace);
#if INCSDIF
    /* RWD if extension is .sdif, write as 1TRC frames */
    if (is_sdiffile(t->outfilnam)) {
      if (UNLIKELY(!writesdif(csound,t))) {
        csound->Message(csound, "%s", Str("Unable to write to SDIF file\n"));
        retval = -1;
      }
    }
    else
#endif
      retval |= filedump(t, csound);  /* write output to adsyn file */

    return retval;
}

static double GETVAL(HET* t, double *inb, int32 smpl)
{                               /* get value at position smpl in array inb */
    if (smpl<0) return 0.0;
    return   inb[(smpl + t->midbuf) & t->bufmask];
}

static void PUTVAL(HET* t, double *outb, int32 smpl, double value)
{                               /* put value in array outb at postn smpl */
    outb[(smpl + t->midbuf) & t->bufmask] = value;
}

static int32_t hetdyn(CSOUND *csound,
                      HET* t, int32_t hno) /* HETERODYNE FILTER */
{
    int32   smplno;
    double  temp_a, temp_b, tpidelest;
    double  *cos_p, *sin_p;
    int32   n;
    int32_t outpnt, lastout = -1;
    MYFLT   *ptr;

    t->jmp_ph = 0;                     /* set initial phase to 0 */
    temp_a = temp_b = 0;
    cos_p = t->c_p;
    sin_p = t->s_p;
    tpidelest = TWOPI * t->cur_est * t->delta_t;
    for (smplno = 0; smplno < t->smpsin; smplno++) {
      //double phase = smplno * tpidelest;     /* do all quadrature calcs */
      ptr = t->adp;           /* at once and point to it */
      cos_p[smplno] = (double)(ptr[smplno] * cos(smplno * tpidelest));
      sin_p[smplno] = (double)(ptr[smplno] * sin(smplno * tpidelest));
    }

    for (smplno = 0; smplno < t->smpsin - t->windsiz; smplno++) {
      cos_p = t->c_p + smplno;
      sin_p = t->s_p + smplno;
      if (smplno == 0 && t->smpsin >= t->windsiz) {
        /* for first smplno */
        for (n=0; n< t->windsiz; n++) {
          //printf("cos/sin = %f / %f\n", cos_p[n], sin_p[n]);
          temp_a += cos_p[n];     /* sum over windsiz = nsmps in */
          temp_b += sin_p[n];     /*    1 period of fund. freq.  */
          //printf("temp = %f / %f\n", temp_a, temp_b);
        }
      }
      else {      /* if more than 1 fund. per. away from file end */
                  /* remove front value and add on new rear value */
                  /* to obtain summation term for new sample! */
        if (smplno <= t->smpsin - t->windsiz) {
          temp_a += cos_p[t->windsiz - 1] - cos_p[-1];  /* _wp = _p + windsiz */
          temp_b += sin_p[t->windsiz - 1] - sin_p[- 1];
          //printf("**temp = %f / %f\n", temp_a, temp_b);
        }
        else {
          t->skip = 1;
          temp_a = temp_b = 0;
        }
      }
      /* store values into buffers */
      //printf("**temp = %f / %f\n", temp_a, temp_b);
      PUTVAL(t, t->cos_mul, smplno, temp_a);
      PUTVAL(t, t->sin_mul, smplno, temp_b);
      if ((t->freq_c <= 1) || (smplno < 3)) {
        average(t, t->windsiz, t->cos_mul, t->a_term,
                         smplno); /* average over previous */
        average(t, t->windsiz, t->sin_mul, t->b_term,
                         smplno); /* values 1 fund prd ago */
        //printf("average smplno=%d sin/cos = %f/%f\n",
        // smplno,GETVAL(t,t->sin_mul, smplno), GETVAL(t,t->cos_mul, smplno));
      }
      else {
        lowpass(t, t->a_term,t->cos_mul,smplno);
        lowpass(t, t->b_term,t->sin_mul,smplno);
        //printf("lowpass smplno=%d sin/cos = %f/%f\n",
        //       smplno,GETVAL(t,t->sin_mul, smplno),
        //       GETVAL(t,t->cos_mul, smplno));
      }
      output_ph(t, smplno);       /* calculate mag. & phase for sample */
      if ((outpnt = (int32_t)(smplno * t->outdelta_t)) > lastout) {
        /* if next out-time */
        output(t, smplno, hno, outpnt);  /*     place in     */
        lastout = outpnt;                      /*     output array */
        if (!csound->CheckEvents(csound))
          return -1;
      }
      if (t->skip) {
        t->skip = 0;       /* quit if no more samples in file */
        break;
      }
    }

    return 0;
}

static void lpinit(HET *t) /* lowpass coefficient ititializer */
{               /* 3rd order butterworth LPF coefficients calculated using */
                /* impulse invariance */
    MYFLT costerm,sinterm;
    double omega_c;

    omega_c = t->freq_c*TWOPI;
    costerm = (MYFLT)cos(SQRTOF3*omega_c*t->delta_t*0.5);
    sinterm = (MYFLT)sin(SQRTOF3*omega_c*t->delta_t*0.5);
    t->x1 = (MYFLT)(omega_c*t->delta_t*
                          (exp(-omega_c*t->delta_t) +
                           exp(-omega_c*t->delta_t/2.0)
                           * (-costerm + sinterm/SQRTOF3)));
    t->x2 = (MYFLT)(omega_c*t->delta_t*
                          (exp(-omega_c*t->delta_t) -
                           exp(-3*omega_c*t->delta_t/2)
                           * (costerm + sinterm/SQRTOF3)));
    t->yA = (-((MYFLT)exp(-omega_c*t->delta_t) +
            FL(2.0)*(MYFLT)exp(-omega_c*t->delta_t/2)*costerm));
    t->y2 = FL(2.0) * (MYFLT)exp(-3.0*omega_c*t->delta_t/2.0)*costerm +
            (MYFLT)exp(-omega_c*t->delta_t);
    t->y3 = (-(MYFLT)exp(-2.0*omega_c*t->delta_t));
}

static void lowpass(HET *t, double *out, double *in, int32 smpl)
  /* call with x1,x2,yA,y2,y3 initialised  */
  /* calls LPF function */
{
    PUTVAL(t, out, smpl,
           (t->x1 *
            GETVAL(t,in,smpl-1) + t->x2 * GETVAL(t,in,smpl-2) -
            t->yA * GETVAL(t,out,smpl-1) - t->y2 *
            GETVAL(t,out,smpl-2) -
            t->y3 * GETVAL(t,out,smpl-3)));
}

static void average(HET *t, int32 window,double *in,double *out, int32 smpl)
  /* AVERAGES OVER 'WINDOW' SAMPLES */
  /* this is actually a comb filter with 'Z' */
  /* transform of (1/w *[1 - Z**-w]/[1 - Z**-1]) */
  /* ie. zeros at all harmonic frequencies except*/
  /* the current one where the pole cancels it */
{
    //if (smpl==0) return;
    if (smpl<window) {
      //printf("inside window: %f %f\n", GETVAL(t,out,smpl-1), GETVAL(t,in,smpl));
      PUTVAL(t,out, smpl,
             (double)(GETVAL(t,out,smpl-1) +
                      (1.0/(double)window) * (GETVAL(t,in,smpl))));
    }
    else {
      //printf("outside window: %f %f %f\n", GETVAL(t,out,smpl-1),
      //       GETVAL(t,in,smpl), GETVAL(t,in,smpl-window));
      PUTVAL(t,out, smpl,
             (double)(GETVAL(t,out,smpl-1) +
                      (1.0/(double)window) *
                      (GETVAL(t,in,smpl) - GETVAL(t,in,smpl-window))));
    }
}

                                 /* update phase counter */
static void output_ph(HET *t,int32 smpl)
                                /* calculates magnitude and phase components */
                                /* for each samples quadrature components, & */
                                /* and unwraps the phase.  A phase difference*/
{                               /* is taken to represent the freq. change.   */
    double      delt_temp;      /* the pairs are then comb filtered.         */
    double      temp_a;

    //printf("phase: %f %f\n", GETVAL(t, t->a_term,smpl), GETVAL(t,t->b_term,smpl));
    if ((temp_a=GETVAL(t,t->a_term,smpl)) == 0)
            t->new_ph=
              (-PI/FL(2.0))*sgn(GETVAL(t,t->b_term,smpl));
    else t->new_ph=
           -atan(GETVAL(t,t->b_term,smpl)/temp_a) - PI*u(-temp_a);

    if (fabs((double)t->new_ph - t->old_ph)>PI)
      t->jmp_ph -= TWOPI*sgn(temp_a);

    //printf("output-ph: %f ->%f\n",t->old_ph, t->new_ph);
    t->old_ph = t->new_ph;
    PUTVAL(t,t->r_phase,smpl,t->old_ph+t->jmp_ph);
    delt_temp = ((GETVAL(t,t->r_phase,smpl) -
                  GETVAL(t,t->r_phase,smpl-1))/
                 (TWOPI*t->delta_t));
    if ((t->freq_c <= 1) || (smpl < 3)) {
      PUTVAL(t,t->amp_av1,smpl,
             (MYFLT)hypot(GETVAL(t,t->a_term,smpl),
                          GETVAL(t,t->b_term,smpl)));
      average(t, t->windsiz,t->amp_av1,t->amp_av2,smpl);
      average(t, t->windsiz,t->amp_av2,t->amp_av3,smpl);
      average(t, t->windsiz,t->amp_av3,t->r_ampl,smpl);
      PUTVAL(t,t->ph_av1,smpl,delt_temp);
      average(t, t->windsiz,t->ph_av1,t->ph_av2,smpl);
      average(t, t->windsiz,t->ph_av2,t->ph_av3,smpl);
      average(t, t->windsiz,t->ph_av3,t->a_avg,smpl);
      //printf("AV3: %f (%d) %d\n", GETVAL(t, t->a_avg,smpl), smpl, t->windsiz);
    }
    else {
      PUTVAL(t,t->r_ampl,smpl,
              (MYFLT)hypot(GETVAL(t,t->a_term,smpl),
                           GETVAL(t,t->b_term,smpl)));
      PUTVAL(t,t->a_avg,smpl,delt_temp);
    }
    //printf("***r_ampl[%d]= %f\n", smpl, GETVAL(t,t->r_ampl,smpl));
}

static void output(HET *t, int32 smpl, int32_t hno, int32_t pnt)
                        /* output one freq_mag pair */
                        /* when called, gets frequency change */
                        /* and adds it to current freq. stores*/
{                       /* current amp and new freq in arrays */
    double delt_freq;
    MYFLT  new_amp, new_freq;

    if (pnt < t->num_pts) {
      delt_freq = GETVAL(t,t->a_avg,smpl); /* 0.5 for rounding ? */
      t->FREQS[hno][pnt] =
        new_freq = (MYFLT)(delt_freq + t->cur_est);
      t->MAGS[hno][pnt] =
        new_amp = (MYFLT)GETVAL(t, t->r_ampl,smpl);
      if (new_freq > t->max_frq)
        t->max_frq = new_freq;
      //printf("** new_amp %f; max_amp = %f\n", new_amp, t->max_amp);
      if (new_amp > t->max_amp) {
        //printf("******** update\n");
        t->max_amp = new_amp;
      }
    }
}

static const char *hetro_usage_txt[] = {
  Str_noop("Usage: hetro [options...] inputSoundfile outputfile"),
  Str_noop("Options:"),
  Str_noop("    -s <samplerate>"),
  Str_noop("    -c <channel>"),
  Str_noop("    -b <beginTime>"),
  Str_noop("    -d <duration>"),
  Str_noop("    -f <fundamental estimate"),
  Str_noop("    -h <harmonic count>"),
  Str_noop("    -M <maximum amplitide>"),
  Str_noop("    -m <minimum amplitide>"),
  Str_noop("    -n <number of output points>"),
  Str_noop("    -l <filter cutoff>"),
  Str_noop("    -X <newformat>"),
  Str_noop("    -x <oldformat>"),
  Str_noop("    -- <log file>"),
    NULL
};

static int32_t quit(CSOUND *csound, char *msg)
{
    int32_t i;
    csound->ErrorMsg(csound, Str("hetro:  %s\n\tanalysis aborted"), msg);
    for (i = 0; hetro_usage_txt[i] != NULL; i++)
      csound->Message(csound, "%s\n", Str(hetro_usage_txt[i]));
    return -1;
}

#define END 32767

/* WRITE OUTPUT FILE in DATA-REDUCED format */

static int32_t filedump(HET *t, CSOUND *csound)
{
    int32_t h, pnt, ofd, nbytes;
    double  scale,x,y;
    int16   **mags, **freqs, *magout, *frqout;
    double  ampsum, maxampsum = 0.0;
    int32   lenfil = 0;
    int16   *TIME;
    MYFLT   timesiz;
    FILE    *ff;

    mags  = (int16 **) csound->Malloc(csound, t->hmax * sizeof(int16*));
    freqs = (int16 **) csound->Malloc(csound, t->hmax * sizeof(int16*));
    for (h = 0; h < t->hmax; h++) {
      mags[h]  = (int16 *)csound->Malloc(csound,
                                        (t->num_pts+2) * sizeof(int16));
      freqs[h] = (int16 *)csound->Malloc(csound,
                                         (t->num_pts+2) * sizeof(int16));
    }

    TIME = (int16 *)csound->Malloc(csound, (t->num_pts+2) * sizeof(int16));
    timesiz = FL(1000.0) * t->input_dur /t->num_pts;
    for (pnt = 0; pnt < t->num_pts; pnt++)
      TIME[pnt] = (int16)(pnt * timesiz);

    /* fullpath else cur dir */
    if (t->newformat) {
      if (UNLIKELY(csound->FileOpen(csound, &ff, CSFILE_STD, t->outfilnam,
                                     "w", "", CSFTYPE_HETROT, 0) == NULL))
      return quit(csound, Str("cannot create output file\n"));
    } else
      if (UNLIKELY(csound->FileOpen(csound, &ofd, CSFILE_FD_W, t->outfilnam,
                                     NULL, "", CSFTYPE_HETRO, 0) == NULL))
        return quit(csound, Str("cannot create output file\n"));

    if (t->newformat)
      fprintf(ff,"HETRO %d\n", t->hmax);        /* Header */
    else {
      if (UNLIKELY(write(ofd, (char*)&t->hmax, sizeof(t->hmax))<0))
        csound->Message(csound,"%s", Str("Write failure\n")); /* Write header */
    }
    for (pnt=1; pnt < t->num_pts+1; pnt++) {
      ampsum = 0.0;
      for (h = 0; h < t->hmax; h++)
        ampsum += t->MAGS[h][pnt];
      if (ampsum > maxampsum)
        maxampsum = ampsum;
    }
    scale = t->m_ampsum / maxampsum;
    csound->Message(csound,Str("scale = %f\n"), scale);

    for (h = 0; h < t->hmax; h++) {
      for (pnt = 0; pnt < t->num_pts+1; pnt++) {
        x = t->MAGS[h][pnt] * scale;
        mags[h][pnt] = (int16)(x*u(x)+0.5);
        y = t->FREQS[h][pnt];
        freqs[h][pnt] = (int16)(y*u(y)+0.5);
        //printf("***mags[%d][%d] = %f (%hd)\tfreqs[%d][%d] = %f (%hd)\n",
        //       h, pnt,x, mags[h][pnt], h, pnt, y, freqs[h][pnt]);
      }
    }

    magout = (int16 *)csound->Malloc(csound,
                                     (t->num_pts+1) * 2 * sizeof(int16));
    frqout = (int16 *)csound->Malloc(csound,
                                     (t->num_pts+1) * 2 * sizeof(int16));

    for (h = 0; h < t->hmax; h++) {
      int16 *mp = magout, *fp = frqout;
      int16 *lastmag, *lastfrq, pkamp = 0;
      int32_t mpoints, fpoints;
      *mp++ = -1;                      /* set brkpoint type codes  */
      *fp++ = -2;
      lastmag = mp;
      lastfrq = fp;
      for (pnt = 0; pnt < t->num_pts; pnt++) {
        int16 tim, mag, frq;
        tim = TIME[pnt];
        frq = freqs[h][pnt];
        if ((mag = mags[h][pnt]) > pkamp)
          pkamp = mag;
        //printf("****(%d,%d)time, mag, frq = %hd / %hd %hd\n",
        //       h, pnt, tim, mag, frq);
        if (mag > t->amp_min) {
#if 0
          if (contig > 2) {        /* if third time this value  */
            if ((mag == *(mp-1) && mag == *(mp-3))
                                   /*    or 2nd time this slope */
                || ((MYFLT)(mag - *(mp-1)) / (tim - *(mp-2)) ==
                    (MYFLT)(*(mp-1) - *(mp-3)) / (*(mp-2) - *(mp-4))))
              mp -= 2;              /* overwrite the previous */
            if ((frq == *(fp-1) && frq == *(fp-3))
                || ((MYFLT)(frq - *(fp-1)) / (tim - *(fp-2)) ==
                    (MYFLT)(*(fp-1) - *(fp-3)) / (*(fp-2) - *(fp-4))))
              fp -= 2;
          }
#endif
          *mp++ = tim;
          *mp++ = mag;
          *fp++ = tim;
          *fp++ = frq;
          lastmag = mp;         /* record last significant seg  */
          lastfrq = fp;
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
      mpoints = (int32_t) ((mp - magout) / 2) - 1;
      nbytes = (int32_t)((mp - magout) * sizeof(int16));
      if (t->newformat) {
        int32_t i;
        for (i=0; i<(mp - magout); i++)
          fprintf(ff,"%hd%c", magout[i], i==(mp-magout-1)?'\n':',');
      }
      else {
        if (UNLIKELY(write(ofd, (char *)magout, nbytes)<0))
          csound->Message(csound, "%s", Str("Write failure\n"));
      }
#ifdef DEBUG
      {
        int32_t i;
        for (i=0; i<(mp-magout); i++)
          csound->Message(csound, "%hd,", magout[i]);
        csound->Message(csound, "\n");
      }
#endif
      lenfil += nbytes;
      fpoints = (int32_t) ((fp - frqout) / 2) - 1;
      nbytes = (int32_t)((fp - frqout) * sizeof(int16));
      if (t->newformat) {
        int32_t i;
        for (i=0; i<fp - frqout; i++)
          fprintf(ff,"%hd%c", frqout[i], i==(fp-frqout-1)?'\n':',');
        fprintf(ff,"\n");
      }
      else {
        if (UNLIKELY(write(ofd, (char *)frqout, nbytes)<0))
          csound->Message(csound, "%s", Str("Write failure\n"));
      }
#ifdef DEBUG
      {
        int32_t i;
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
    csound->Message(csound, "%s%" PRId64 " %s%s\n", Str("wrote "),
                    (int64_t)lenfil, Str("bytes to "), t->outfilnam);
    csound->Free(csound, magout);
    csound->Free(csound, frqout);
    csound->Free(csound, TIME);
    for (h = 0; h < t->hmax; h++) {
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

static int32_t writesdif(CSOUND *csound, HET *t)
{
    int32_t     i,j,h, pnt;
    double      scale;
    double      ampsum, maxampsum = 0.0;
    MYFLT       timesiz;
    SDIFresult  r;
    SDIF_FrameHeader head;
    SDIF_MatrixHeader mh;
    FILE        *sdiffile = NULL;

    if (UNLIKELY(SDIF_Init() != ESDIF_SUCCESS)) {
      csound->Message(csound,
                      "%s", Str("OOPS: SDIF does not work on this machine!\n"));
      return 0;
    }

    /* esential rescaling, from filedump() above  */
    for (pnt=0; pnt < t->num_pts; pnt++) {
      ampsum = 0.0;
      for (h = 0; h < t->hmax; h++)
        ampsum += t->MAGS[h][pnt];
      if (ampsum > maxampsum)
        maxampsum = ampsum;
    }
    scale = t->m_ampsum / maxampsum;
    /* SDIF does not specify a range, 'cos it's too clever for that sort
     * of thing, but this seems consistent with existing examples! */
    scale *= (double) (1.0/csound->Get0dBFS(csound));

    for (h = 0; h < t->hmax; h++) {
      for (pnt = 0; pnt < t->num_pts; pnt++) {
        t->MAGS[h][pnt] *= (MYFLT) scale;
        /* skip code to force positive values, for now */
      }
    }

    if (UNLIKELY((r =
                  SDIF_OpenWrite(t->outfilnam, &sdiffile))!=ESDIF_SUCCESS)) {
      /* can get SDIF error messages, but trickly for CSTRINGS */
      csound->Message(csound,Str("Error creating %s\n"),t->outfilnam);
      fclose(sdiffile);
      return 0;
    }
    csound->NotifyFileOpened(csound, t->outfilnam, CSFTYPE_SDIF, 1, 0);

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
    head.size += 4 * sizeof(sdif_float32) * t->hmax;
    /* no padding bytes */
    /*timesiz = 1000.0f * t->input_dur / t->num_pts;*/
    timesiz = t->input_dur / t->num_pts;
    /* do not even need a TIME array */

    /* main loop to write 1TRC frames */
    for (i=0; i < t->num_pts; i++) {
      sdif_float32 amp,freq,phase = 0.0f;
      /* cannot offer anything interesting with phase! */
      head.time = (sdif_float32) ((MYFLT)i * timesiz);
      if (UNLIKELY((r = SDIF_WriteFrameHeader(&head,sdiffile))!=ESDIF_SUCCESS)) {
        csound->Message(csound,"%s", Str("Error writing SDIF frame header.\n"));
        return 0;
      }
      /*setup data matrix */
      mh.rowCount = t->hmax ;
      mh.columnCount = 4;
      SDIF_Copy4Bytes(mh.matrixType,"1TRC");
      mh.matrixDataType = SDIF_FLOAT32;
      if (UNLIKELY((r = SDIF_WriteMatrixHeader(&mh,sdiffile))!=ESDIF_SUCCESS)) {
        csound->Message(csound,"%s", Str("Error writing SDIF matrix header.\n"));
        return 0;
      }
      for (j=0;j < t->hmax;j++) {
        sdif_float32 index;
        /* zero index not used in SDIF */
        index = (sdif_float32)(j+1);
        amp = (sdif_float32) t->MAGS[j][i];
        freq = (sdif_float32) t->FREQS[j][i];
        if (UNLIKELY(((r = SDIF_Write4(&index,1,sdiffile))!= ESDIF_SUCCESS) ||
                     ((r = SDIF_Write4(&freq,1,sdiffile))!= ESDIF_SUCCESS)  ||
                     ((r = SDIF_Write4(&amp,1,sdiffile))!= ESDIF_SUCCESS)   ||
                     ((r = SDIF_Write4(&phase,1,sdiffile))!= ESDIF_SUCCESS))) {
          csound->Message(csound,"%s", Str("Error writing SDIF data.\n"));
          return 0;
        }
      }
      /* 64-bit alignment can be relied upon here, so no need to calc padding */
    }
    csound->Message(csound,
                    Str("wrote %d 1TRC frames to %s\n"),
                    t->num_pts, t->outfilnam);
    SDIF_CloseWrite(sdiffile);
    return 1;
}

static int32_t is_sdiffile(char *name)
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

int32_t hetro_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "hetro", hetro);
    if (!retval) {
      retval = (csound->GetUtility(csound))->SetUtilityDescription(csound, "hetro",
                                             Str("Soundfile analysis for adsyn"));
    }
    return retval;
}
