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

#include "cs.h"                                          /*  HETRO.C   */
#include "soundio.h"
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/*RWD need to set this to prevent sdif.h including windows.h */
#define _WINDOWS_
/* CNMAT sdif library, subject to change..... */
#include "sdif.h"
#include "sdif-mem.h"

#ifdef mills_macintosh
#include "MacTransport.h"
#endif

/*  #define PIE 3.14159265358979323846 */
/*  #define TWOPI   6.28318530717958647692 */
#define SQRTOF3 1.73205080756887729352
#define SQUELCH 0.5     /* % of max ampl below which delta_f is frozen */
#define HMAX    50


typedef struct {
    sdif_float32 index, freq, amp, phase;
} SDIF_RowOf1TRC;

static int is_sdiffile(char *name);
static int writesdif(void);


/* Authors:   Tom Sullivan, Nov'86, Mar'87;  bv revised Jun'92, Aug'92  */
/* Function:  Fixed frequency heterodyne filter analysis.                    */
/* Simplifications and partial recoding by John Fitch Dec 1994 */
/* SDIF extensions by Richard Dobson, Aug 2000 */

/* lowest heterodyne freq = sr/bufsiz */

static MYFLT    x1,x2,yA,y2,y3,         /* lpf coefficients*/
        cur_est,                        /* current freq. est.*/
        freq_est = FL(0.0), max_frq, max_amp,/* harm freq. est. & max vals found */
        fund_est = FL(100.0),           /* fundamental est.*/
        t,                              /* fundamental period est.*/
        delta_t, outdelta_t,            /* sampling period, outpnt period */
        sr = FL(0.0),                   /* sampling rate */
        freq_c = FL(0.0),               /* filter cutoff freq.*/
        beg_time = FL(0.0), input_dur = FL(0.0),/* begin time & sample input duration*/
        **MAGS, **FREQS;                /* magnitude and freq. output buffers*/

static double *cos_mul, *sin_mul,       /* quad. term buffers*/
        *a_term, *b_term,               /*real & imag. terms*/
        *r_ampl,                        /* pt. by pt. amplitude buffer*/
        *r_phase,                       /* pt. by pt. phase buffer*/
        *a_avg,                         /* output dev. freq. buffer*/
        new_ph,                         /* new phase value*/
        old_ph = 0.0,                   /* previous phase value*/
        jmp_ph = 0.0,                   /* for phase unwrap*/
        *ph_av1, *ph_av2, *ph_av3,      /*tempor. buffers*/
        *amp_av1, *amp_av2, *amp_av3,   /* same for ampl.*/
        m_ampsum = 32767.0;             /* maximum amplitude at output*/

static long    windsiz;                 /* # of pts. in one per. of sample*/

static short    hmax = 10;              /* max harmonics requested */
static int      num_pts = 256,          /* breakpoints per harmonic */
                amp_min = 64;           /* amplitude cutout threshold */

static int     skip,                    /* flag to stop analysis if zeros*/
        bufsiz = 1;                     /* circular buffer size */

static long     smpsin;                 /* num sampsin */

static long    midbuf,                  /* set to bufsiz / 2   */
        bufmask;                        /* set to bufsiz - 1   */

static char    *infilnam,               /* input file name */
        *outfilnam;                     /* output file name */
static MYFLT    *auxp;                  /* pointer to input file */

static MYFLT    *adp;           /*pointer to front of sample file*/
static double   *c_p,*s_p;      /* pointers to space for sine and cos terms */

static  double  GETVAL(double *, long);
static  double  sq(double);
static  void    PUTVAL(double *, long, double);
static  void    hetdyn(int), lpinit(void);
static  void    lowpass(double *, double *, long);
static  void    average(long,double *,double *,long);
static  void    output(long, int, int);
static  void    output_ph(long), filedump(void), quit(char *);
extern  int     close(int);
extern int csoundYield(void *);

#define sgn(x)  (x<0.0 ? -1 : 1)
#define u(x)    (x>0.0 ? 1 : 0)

#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-'))  \
                            quit(MSG);

int hetro(int argc, char **argv)  /* called from main.c or anal/adsyn/main.c */
{
        int      i, hno, infd, channel = 1;
        long     nsamps, smpspc, bufspc, mgfrspc;
        char     *dsp, *dspace, *mspace;
        double   *begbufs, *endbufs;

        SOUNDIN  *p;      /* space allocated by SAsndgetset() */
extern  int      SAsndgetset(char *, SOUNDIN**, MYFLT*, MYFLT*, MYFLT*, int);
extern  long     getsndin(int, MYFLT*, long, SOUNDIN*);
        /* must set this for 'standard' behaviour when analysing
           (assume re-entrant Csound) */
        dbfs_init(DFLT_DBFS);

        if (!(--argc)) {
          quit(Str("no arguments"));
        }
        do {
          char *s = *++argv;
          if (*s++ == '-')
            switch (*s++) {
            case 's':   FIND(Str("no sampling rate"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&sr);
#else
                          sscanf(s,"%f",&sr);
#endif
              break;
            case 'c':   FIND(Str("no channel"))
                          sscanf(s,"%d",&channel);
              break;
            case 'b':   FIND(Str("no begin time"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&beg_time);
#else
                          sscanf(s,"%f",&beg_time);
#endif
              break;
            case 'd':   FIND(Str("no duration time"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&input_dur);
#else
                          sscanf(s,"%f",&input_dur);
#endif
              break;
            case 'f':   FIND(Str("no fundamental estimate"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&fund_est);
#else
                          sscanf(s,"%f",&fund_est);
#endif
              break;
            case 'h':   FIND(Str("no harmonic count"))
                          sscanf(s,"%hd",&hmax);
              if (hmax > HMAX)
                err_printf(Str("over %d harmonics but continuing"),
                           HMAX);
              if (hmax < 1) {
                err_printf(Str("h of %d too low, reset to 1\n"),
                           hmax);
                hmax = 1;
              }
              break;
            case 'M':   FIND(Str("no amplitude maximum"))
                          sscanf(s,"%lf",&m_ampsum);
              break;
            case 'm':   FIND(Str("no amplitude minimum"))
                          sscanf(s,"%d",&amp_min);
              break;
            case 'n':   FIND(Str("no number of output points"))
                          sscanf(s,"%d",&num_pts);
              break;
            case 'l':   FIND(Str("no filter cutoff"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&freq_c);
#else
                          sscanf(s,"%f",&freq_c);
#endif
              break;
            case '-':   FIND(Str("no log file"));
              while (*s++); s--;
              break;
            default:   quit(Str("Invalid switch option"));
            }
          else break;
        } while (--argc);

        if (argc != 2)  quit(Str("incorrect number of filenames"));
        infilnam = *argv++;
        outfilnam = *argv;

        if (freq_c > 1)
            fprintf (stderr,Str("Filter cutoff freq. = %f\n"),freq_c);

        if ((input_dur < 0) || (beg_time < 0))
            quit(Str("input and begin times cannot be less than zero"));
                                           /* open sndfil, do skiptime */
        if (
         (infd = SAsndgetset(infilnam,&p,&beg_time,&input_dur,&sr,channel))<0) {
            sprintf(errmsg,Str("Cannot open %s"), retfilnam);
            quit (errmsg);
        }
        nsamps = p->getframes;
        auxp = (MYFLT*)mmalloc(nsamps * sizeof(MYFLT));   /* alloc for MYFLTs */
        if ((smpsin = getsndin(infd,auxp,nsamps,p)) <= 0) { /* & read them in */
          printf("smpsin = %d\n", smpsin);
            sprintf(errmsg,Str("Read error on %s\n"), retfilnam);
            quit(errmsg);
        }
        close(infd);

        sr = (MYFLT)p->sr;                              /* sr now from open  */
        windsiz = (long)(sr / fund_est + FL(0.5));      /* samps in fund prd */
        /*RWD no limit for SDIF files! */
        if (is_sdiffile(outfilnam)) {
          if (num_pts >= nsamps - windsiz)
            quit(Str("number of output points is too great"));
        }
        else
          if (num_pts > 32767 || num_pts >= nsamps - windsiz)            quit(Str("number of output points is too great"));
        delta_t = FL(1.0)/sr;
        t = FL(1.0)/fund_est;
        outdelta_t = (MYFLT)num_pts / (smpsin - windsiz);

        while (bufsiz < (sr/fund_est + FL(0.5)))
            bufsiz *= 2;
        midbuf = bufsiz/2;
        bufmask = bufsiz - 1;

        smpspc = smpsin * sizeof(double);
        bufspc = bufsiz * sizeof(double);

        dsp = dspace = mmalloc(smpspc * 2 + bufspc * 13);
        c_p = (double *) dsp;           dsp += smpspc;  /* space for the    */
        s_p = (double *) dsp;           dsp += smpspc;  /* quadrature terms */
        begbufs = (double *) dsp;
        cos_mul = (double *) dsp;       dsp += bufspc;  /* bufs that will be */
        sin_mul = (double *) dsp;       dsp += bufspc;  /* refilled each hno */
        a_term = (double *) dsp;        dsp += bufspc;
        b_term = (double *) dsp;        dsp += bufspc;
        r_ampl = (double *) dsp;        dsp += bufspc;
        ph_av1 = (double *) dsp;        dsp += bufspc;
        ph_av2 = (double *) dsp;        dsp += bufspc;
        ph_av3 = (double *) dsp;        dsp += bufspc;
        r_phase = (double *) dsp;       dsp += bufspc;
        amp_av1 = (double *) dsp;       dsp += bufspc;
        amp_av2 = (double *) dsp;       dsp += bufspc;
        amp_av3 = (double *) dsp;       dsp += bufspc;
        a_avg = (double *) dsp;         dsp += bufspc;
        endbufs = (double *) dsp;

        mgfrspc = num_pts * sizeof(MYFLT);
        dsp = mspace = mmalloc(mgfrspc * hmax * 2);
        MAGS = (MYFLT **) mmalloc(hmax * sizeof(MYFLT*));
        FREQS = (MYFLT **) mmalloc(hmax * sizeof(MYFLT*));
        for (i = 0; i < hmax; i++) {
            MAGS[i] = (MYFLT *) dsp;    dsp += mgfrspc;
            FREQS[i] = (MYFLT *) dsp;   dsp += mgfrspc;
        }
        lpinit();                               /* calculate LPF coeffs.  */
        adp = auxp;                     /* point to beg sample data block */
        for (hno = 0; hno < hmax; hno++) {      /* for requested harmonics*/
            double *dblp;
            freq_est += fund_est;               /*   do analysis */
            cur_est = freq_est;
            dblp = begbufs;
            do {
              *dblp++ = FL(0.0);              /* clear all refilling buffers*/
            } while (dblp < endbufs);
            max_frq = FL(0.0);
            max_amp = FL(0.0);

            err_printf(Str("analyzing harmonic #%d\n"),hno);
            err_printf(Str("freq est %6.1f,"), cur_est);
            hetdyn(hno);                /* perform actual computation */
            if (!csoundYield(NULL)) exit(1);
            err_printf(Str(" max found %6.1f, rel amp %6.1f\n"), max_frq, max_amp);
        }
        mfree(dspace);
#ifdef mills_macintosh
        if (!(transport.state & kGenKilled))
#endif
          {                     /* Note section bracket neded from if above */
            /*RWD if extension is .sdif, write as 1TRC frames */
            if (is_sdiffile(outfilnam)) {
              if (!writesdif()) {
                err_printf(Str("Unable to write to SDIF file\n"));
                mfree(mspace);
                exit(1);
              }
            }
            else
              filedump();                       /* write output to adsyn file */
          }
        mfree(mspace);
#if !defined(mills_macintosh)
        exit(0);
#endif
        return (-1);            /* To keep compiler quiet */
}

static double
GETVAL(double *inb, long smpl)    /* get value at position smpl in array inb */
{
    return(*(inb + ((smpl + midbuf) & bufmask)));
}

static void
PUTVAL(double *outb, long smpl, double value)  /* put value in array outb at postn smpl */
{
    *(outb + ((smpl + midbuf) & bufmask)) = value;
}

static void hetdyn(int hno)                           /* HETERODYNE FILTER */
{
    long    smplno;
    double  temp_a, temp_b, tpidelest;
    double *cos_p, *sin_p, *cos_wp, *sin_wp;
    long    n;
    int     outpnt, lastout = -1;
    MYFLT *ptr;

    jmp_ph = 0;                     /* set initial phase to 0 */
    temp_a = temp_b = 0;
    cos_p = c_p;
    sin_p = s_p;
    tpidelest = TWOPI * cur_est * delta_t;
    for (smplno = 0; smplno < smpsin; smplno++) {
      double phase = smplno * tpidelest;     /* do all quadrature calcs */
      ptr = adp + smplno;                    /* at once and point to it */
      *cos_p++ = (double)(*ptr) * cos(phase);
      *sin_p++ = (double)(*ptr) * sin(phase);
    }

    cos_p = cos_wp = c_p;
    sin_p = sin_wp = s_p;
    for (smplno = 0; smplno < smpsin - windsiz; smplno++) {
      if (smplno == 0 && smpsin >= windsiz) {   /* for first smplno */
        n = windsiz;
        do {
          temp_a += *cos_wp++;     /* sum over windsiz = nsmps in */
          temp_b += *sin_wp++;     /*    1 period of fund. freq.  */
        } while (--n);
      }
      else {      /* if more than 1 fund. per. away from file end */
                  /* remove front value and add on new rear value */
                  /* to obtain summation term for new sample! */
        if (smplno <= smpsin - windsiz) {
          temp_a += (*cos_wp++ - *cos_p++);  /* _wp = _p + windsiz */
          temp_b += (*sin_wp++ - *sin_p++);
        }
        else {
          skip = 1;
          temp_a = temp_b = 0;
        }
      }
      PUTVAL(cos_mul,smplno,temp_a);     /* store values into buffers*/
      PUTVAL(sin_mul,smplno,temp_b);
      if ((freq_c <= 1) || (smplno < 3)) {
        average(windsiz,cos_mul,a_term,smplno); /* average over previous */
        average(windsiz,sin_mul,b_term,smplno); /* values 1 fund prd ago */
      }
      else {
        lowpass(a_term,cos_mul,smplno);
        lowpass(b_term,sin_mul,smplno);
      }
      output_ph(smplno);            /* calculate mag. & phase for sample */
      if ((outpnt = (int)(smplno * outdelta_t)) > lastout) { /* if next out-time */
        output(smplno, hno, outpnt);           /*     place in     */
        lastout = outpnt;                      /*     output array */
      }
      if (skip) {
        skip = 0;       /* quit if no more samples in file */
        break;
      }
    }
}

static void lpinit(void) /* lowpass coefficient ititializer */
{               /* 3rd order butterworth LPF coefficients calculated using */
                /* impulse invariance */
    MYFLT costerm,sinterm;
    double omega_c;

    omega_c = freq_c*TWOPI;
    costerm = (MYFLT)cos(SQRTOF3*omega_c*delta_t/2.0);
    sinterm = (MYFLT)sin(SQRTOF3*omega_c*delta_t/2.0);
    x1 = (MYFLT)(omega_c*delta_t*(exp(-omega_c*delta_t) +
                                  exp(-omega_c*delta_t/2.0)
                                  * (-costerm + sinterm/SQRTOF3)));
    x2 = (MYFLT)(omega_c*delta_t*(exp(-omega_c*delta_t) -
                                  exp(-3*omega_c*delta_t/2)
                                  * (costerm + sinterm/SQRTOF3)));
    yA = (-((MYFLT)exp(-omega_c*delta_t) +
            FL(2.0)*(MYFLT)exp(-omega_c*delta_t/2)*costerm));
    y2 = FL(2.0) * (MYFLT)exp(-3.0*omega_c*delta_t/2.0)*costerm +
            (MYFLT)exp(-omega_c*delta_t);
    y3 = (-(MYFLT)exp(-2.0*omega_c*delta_t));
}

static void lowpass(double *out, double *in, long smpl)    /* call with x1,x2,yA,y2,y3 initialised  */
                /* calls LPF function */
{
    PUTVAL(out, smpl, (x1 *
                       GETVAL(in,smpl-1) + x2 * GETVAL(in,smpl-2) -
                       yA * GETVAL(out,smpl-1) - y2 *
                       GETVAL(out,smpl-2) - y3 * GETVAL(out,smpl-3)));
}

static void average(long window,double *in,double *out, long smpl)  /* AVERAGES OVER 'WINDOW' SAMPLES */
  /* this is actually a comb filter with 'Z' */
  /* transform of (1/w *[1 - Z**-w]/[1 - Z**-1]) */
  /* ie. zeros at all harmonic frequencies except*/
  /* the current one where the pole cancels it */
{
    PUTVAL(out, smpl,
           (double)(GETVAL(out,smpl-1) +
                    (1/(double)window) * (GETVAL(in,smpl) - GETVAL(in,smpl-window))));
}

                                 /* update phase counter */
static void output_ph(long smpl)/* calculates magnitude and phase components */
                                /* for each samples quadrature components, & */
                                /* and unwraps the phase.  A phase difference*/
{                               /* is taken to represent the freq. change.   */
    double      delt_temp;      /* the pairs are then comb filtered.         */
    double      temp_a;

    if ((temp_a=GETVAL(a_term,smpl)) == 0)
            new_ph=(-PI/FL(2.0))*sgn(GETVAL(b_term,smpl));
    else new_ph= -atan(GETVAL(b_term,smpl)/temp_a) - PI*u(-temp_a);

    if (fabs((double)new_ph - old_ph)>PI)
      jmp_ph -= TWOPI*sgn(temp_a);

    old_ph = new_ph;
    PUTVAL(r_phase,smpl,old_ph+jmp_ph);
    delt_temp = ((GETVAL(r_phase,smpl) - GETVAL(r_phase,smpl-1))/
                 (TWOPI*delta_t));
    if ((freq_c <= 1) || (smpl < 3)) {
      PUTVAL(amp_av1,smpl,(MYFLT)sqrt(sq(GETVAL(a_term,smpl))
                                      + sq(GETVAL(b_term,smpl))));
      average(windsiz,amp_av1,amp_av2,smpl);
      average(windsiz,amp_av2,amp_av3,smpl);
      average(windsiz,amp_av3,r_ampl,smpl);
      PUTVAL(ph_av1,smpl,delt_temp);
      average(windsiz,ph_av1,ph_av2,smpl);
      average(windsiz,ph_av2,ph_av3,smpl);
      average(windsiz,ph_av3,a_avg,smpl);
    }
    else {
      PUTVAL(r_ampl,smpl,(MYFLT)sqrt(sq(GETVAL(a_term,smpl))
                                     + sq(GETVAL(b_term,smpl))));
      PUTVAL(a_avg,smpl,delt_temp);
    }
}

static void output(long smpl, int hno, int pnt)    /* output one freq_mag pair */
                        /* when called, gets frequency change */
                        /* and adds it to current freq. stores*/
{                       /* current amp and new freq in arrays */
    double delt_freq;
    MYFLT  new_amp, new_freq;

    if (pnt < num_pts) {
      delt_freq = GETVAL(a_avg,smpl);        /* 0.5 for rounding ? */
      FREQS[hno][pnt] = new_freq = (MYFLT)(delt_freq + cur_est);
      MAGS[hno][pnt] = new_amp = (MYFLT)GETVAL(r_ampl,smpl);
      if (new_freq > max_frq)
        max_frq = new_freq;
      if (new_amp > max_amp)
        max_amp = new_amp;
      /*printf("A=%f\tF=%f\t%f\n",MAGS[hno][pnt],FREQS[hno][pnt],delt_freq);*/
    }
}

/* If this function worthwhile?  Need to coinsider recalculation */
static double sq(double num)     /* RETURNS SQUARE OF ARGUMENT */
{
    return(num*num);
}

static void quit (char *msg)
{
#ifdef mills_macintosh
    char temp[256];
    sprintf(temp,Str("hetro:  %s\n\tanalysis aborted\n"),msg);
    die(temp);
#else
    err_printf(Str("hetro:  %s\n\tanalysis aborted\n"),msg);
#endif
    exit(1);
}

#define END  32767

static void filedump(void)     /* WRITE OUTPUT FILE in DATA-REDUCED format */
{
    int     h, pnt, ofd, nbytes;
    double  scale,x,y;
    short   **mags, **freqs, *magout, *frqout;
    double  ampsum, maxampsum = 0.0;
    long    lenfil = 0;
    short   *TIME;
    MYFLT   timesiz;
    extern  int     openout(char *, int);

    mags = (short **) mmalloc(hmax * sizeof(short*));
    freqs = (short **) mmalloc(hmax * sizeof(short*));
    for (h = 0; h < hmax; h++) {
      mags[h] = (short *)mmalloc((long)num_pts * sizeof(short));
      freqs[h] = (short *)mmalloc((long)num_pts * sizeof(short));
    }
 
    TIME = (short *)mmalloc((long)num_pts * sizeof(short));
    timesiz = FL(1000.0) * input_dur / num_pts;
    for (pnt = 0; pnt < num_pts; pnt++)
      TIME[pnt] = (short)(pnt * timesiz);

    if ((ofd = openout(outfilnam, 1)) < 0)     /* fullpath else cur dir */
      quit(Str("cannot create output file\n"));

    write(ofd, (char*)&hmax, sizeof(hmax)); /* Write header */

    for (pnt=0; pnt < num_pts; pnt++) {
      ampsum = 0.0;
      for (h = 0; h < hmax; h++)
        ampsum += MAGS[h][pnt];
      if (ampsum > maxampsum)
        maxampsum = ampsum;
    }
    scale = m_ampsum / maxampsum;
    err_printf(Str("scale = %f\n"), scale);

    for (h = 0; h < hmax; h++) {
      for (pnt = 0; pnt < num_pts; pnt++) {
        x = MAGS[h][pnt] * scale;
        mags[h][pnt] = (short)(x*u(x));
        y = FREQS[h][pnt];
        freqs[h][pnt] = (short)(y*u(y));
      }
    }

    magout = (short *)mmalloc((long)(num_pts + 1) * 2 * sizeof(short));
    frqout = (short *)mmalloc((long)(num_pts + 1) * 2 * sizeof(short));
    for (h = 0; h < hmax; h++) {
      short *mp = magout, *fp = frqout;
      short *lastmag, *lastfrq, pkamp = 0;
      int mpoints, fpoints, contig = 0;
      *mp++ = -1;                      /* set brkpoint type codes  */
      *fp++ = -2;
      lastmag = mp;
      lastfrq = fp;
      for (pnt = 0; pnt < num_pts; pnt++) {
        short tim, mag, frq;
        tim = TIME[pnt];
        frq = freqs[h][pnt];
        if ((mag = mags[h][pnt]) > pkamp)
          pkamp = mag;
        if (mag > amp_min) {
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
      nbytes = (mp - magout) * sizeof(short);
      write(ofd, (char *)magout, nbytes);
#ifdef DEBUG
      {
        int i;
        for (i=0; i<(mp-magout); i++)
          err_printf( "%hd,", magout[i]);
        err_printf( "\n");
      }
#endif
      lenfil += nbytes;
      fpoints = ((fp - frqout) / 2) - 1;
      nbytes = (fp - frqout) * sizeof(short);
      write(ofd, (char *)frqout, nbytes);
#ifdef DEBUG
      {
        int i;
        for (i=0; i<(fp-frqout); i++)
          err_printf( "%hd,", frqout[i]);
        err_printf( "\n");
      }
#endif
      lenfil += nbytes;
      printf(Str("harmonic #%d:\tamp points %d, \tfrq points %d,\tpeakamp %d\n"),
             h,mpoints,fpoints,pkamp);
    }
    err_printf(Str("wrote %ld bytes to %s\n"), lenfil, outfilnam);
    close(ofd);
    mfree(magout);
    mfree(frqout);
    mfree(TIME);
    for (h = 0; h < hmax; h++) {
      mfree(mags[h]);
      mfree(freqs[h]);
    }
    mfree(mags);
    mfree(freqs);
}

/* simply writes the number of frames generated - no data reduction, no interpolation */
static int writesdif(void)
{
    int         i,j,h, pnt;
    double      scale;
    double      ampsum, maxampsum = 0.0;
/*    long        lenfil = 0; */
    MYFLT       timesiz;
/*    sdif_int32  frame_id = 1; */
    SDIFresult  r;
    SDIF_FrameHeader head;
    SDIF_MatrixHeader mh;
    FILE        *sdiffile = NULL;

    if (SDIF_Init() != ESDIF_SUCCESS) {
      err_printf(Str("OOPS: SDIF does not work on this machine!\n"));
      return 0;
    }

    /* esential rescaling, from filedump() above  */
    for (pnt=0; pnt < num_pts; pnt++) {
      ampsum = 0.0;
      for (h = 0; h < hmax; h++)
        ampsum += MAGS[h][pnt];
      if (ampsum > maxampsum)
        maxampsum = ampsum;
    }
    scale = m_ampsum / maxampsum;
    /* SDIF does not specify a range, 'cos it's too clever for that sort
     * of thing, but this seems consistent with existing examples! */
    scale *= (double)dbfs_to_float;
/*     scale /= 32768.0; */

    for (h = 0; h < hmax; h++) {
      for (pnt = 0; pnt < num_pts; pnt++) {
        MAGS[h][pnt] *= (MYFLT) scale;
        /* skip code to force positive values, for now */
      }
    }

    if ((r = SDIF_OpenWrite(outfilnam, &sdiffile))!=ESDIF_SUCCESS) {
      /* can get SDIF error messages, but trickly for CSTRINGS */
      err_printf(Str("Error creating %s\n"),outfilnam);
      fclose(sdiffile);
      return 0;
    }

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
    head.size += 4 * sizeof(sdif_float32) * hmax;
    /* no padding bytes */
    /*timesiz = 1000.0f * input_dur / num_pts;*/
    timesiz = input_dur / num_pts;
    /* don't even need a TIME array */

    /* main loop to write 1TRC frames */
    for (i=0;i < num_pts;i++) {
      sdif_float32 amp,freq,phase = 0.0f; /* cannot offer anything interesting with phase! */
      head.time = (sdif_float32) ((MYFLT)i * timesiz);
      if ((r = SDIF_WriteFrameHeader(&head,sdiffile))!=ESDIF_SUCCESS) {
        err_printf(Str("Error writing SDIF frame header.\n"));
        return 0;
      }
      /*setup data matrix */
      mh.rowCount = hmax ;
      mh.columnCount = 4;
      SDIF_Copy4Bytes(mh.matrixType,"1TRC");
      mh.matrixDataType = SDIF_FLOAT32;
      if ((r = SDIF_WriteMatrixHeader(&mh,sdiffile))!=ESDIF_SUCCESS) {
        err_printf(Str("Error writing SDIF matrix header.\n"));
        return 0;
      }
      for (j=0;j < hmax;j++) {
        sdif_float32 index;
        /* zero index not used in SDIF */
        index = (sdif_float32)(j+1);
        amp = (sdif_float32) MAGS[j][i];
        freq = (sdif_float32) FREQS[j][i];
        if (((r = SDIF_Write4(&index,1,sdiffile))!= ESDIF_SUCCESS) ||
            ((r = SDIF_Write4(&freq,1,sdiffile))!= ESDIF_SUCCESS)  ||
            ((r = SDIF_Write4(&amp,1,sdiffile))!= ESDIF_SUCCESS)   ||
            ((r = SDIF_Write4(&phase,1,sdiffile))!= ESDIF_SUCCESS)) {
          err_printf(Str("Error writing SDIF data.\n"));
          return 0;
        }
      }
      /* 64-bit alignment can be relied upon here, so no need to calc padding */
    }
    err_printf(Str("wrote %ld 1TRC frames to %s\n"), num_pts, outfilnam);
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

