/*
    lpanal.c:

    Copyright (C) 1992, 1997 Paul Lansky, Barry Vercoe,
                             John ffitch, Marc Resibois

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

#include "csdl.h"                                       /*  LPANAL.C    */
#include "soundio.h"
#include "lpc.h"
#include "cwindow.h"
#include <math.h>

/* LPC analysis, modified by BV 8'92 for linkage to audio files via soundin.c.
 * Currently set for maximum of 50 poles, & max anal segment of 1000 samples,
 * meaning that the frame slices cannot exceed 500 samples.
 * Program size expands linearly with slice size, and as square of poleCount.
 */

#define DEFpoleCount 34        /* recommended default (max 50 in lpc.h)    */
#define DEFSLICE 200        /* <= MAXWINDIN/2 (currently 1000 in lpc.h) */
#define PITCHMIN        FL(70.0)
#define PITCHMAX        FL(200.0)   /* default limits in Hz for pitch search */

typedef struct {
  int     poleCount, WINDIN, debug, verbose, doPitch;
  double  *x;
  double  (*a)[MAXPOLES];
#ifdef TRACE
  FILE *trace;
#endif
  WINDAT   pwindow;
} LPC;

/* Forward declaration */

static  void    alpol(ENVIRON *, LPC *, MYFLT *,
                                 double *, double *, double *, double *);
static  void    gauss(ENVIRON *, LPC *, double (*)[MAXPOLES], double*, double*);
static  void    quit(ENVIRON *, char *), lpdieu(ENVIRON *, char *);
static  void    usage(ENVIRON *);
static  void    ptable(ENVIRON *, MYFLT, MYFLT, MYFLT, int);
static  MYFLT   getpch(ENVIRON *, MYFLT *);

#ifdef mills_macintosh
#include "MacTransport.h"
#endif

/* Search for an argument and report of not found */
#define FIND(MSG)   if (*s == '\0')  \
                      if (!(--argc) || (((s = *++argv)!=0) && *s == '-'))  \
                        lpdieu(csound, MSG);

#include <math.h>
#include <stdio.h>
#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif

/*
 *
 *  This routine was kindly provided by someone on
 *   the sci.math.num-analysis newsgroup
 *
 * Find the zeros of a polynome
 *
 */

static void polyzero(int nmax, int n, double *a, double *zerore, double *zeroim,
                     int *pt, int itmax, int *indic, double *work)
{
    double        u, v, w, k, m, f, fm, fc, xm, ym, xr, yr, xc, yc;
    double        dx, dy, term, factor;
    int           n1, i, j, p, iter;
    unsigned char conv;

    factor = 1.0;

    if (a[0]==0) {
      *pt = 0;
      *indic = -1;
      return;
    }

    for (i=0; i<=n; i++)
      work[i+1] = a[i];

    *indic = 0;
    *pt = 0;
    n1 = n;

    while (n1>0) {
      if (a[n1]==0) {
        zerore[*pt] = 0.0;
        zeroim[*pt] = 0.0;
        *pt += 1;
        n1 -= 1;
      }
      else {
        p = n1-1;
        xc = 0.0;
        yc = 0.0;
        fc = a[n1]*a[n1];
        fm = fc;
        xm = 0.0;
        ym = 0.0;
        dx = pow(fabs(a[n]/a[0]),(1.0/((MYFLT)n1)));
        dy = 0.0;
        iter = 0;
        conv = FALSE;

        while (!conv) {
          iter += 1;
          if (iter>itmax) {
            *indic = -iter;
            for (i=0; i<=n; i++)
              a[i]=work[i+1];
            return;
          }

          for (i=1; i<=4; i++) {
            u = -dy;
            dy = dx;
            dx = u;
            xr = xc+dx;
            yr = yc+dy;
            u = 0.0;
            v = 0.0;
            k = 2.0*xr;
            m = xr*xr+yr*yr;
            for (j=0; j<=p; j++) {
#ifdef THINK_C
              w = k*u; w -= m*v;
              w += a[j];
#else
              w = a[j]+k*u-m*v; /* Fails on Mac compiler */
#endif
              v = u;
              u = w;
            }

            f = pow(a[n1]+u*xr-m*v,2.0)+(u*u*yr*yr) /*(pow(u*yr,2.0)*/;
            if (f<fm) {
              xm = xr;
              ym = yr;
              fm = f;
            }
          }

          if (fm<fc) {
            dx = 1.5*dx;
            dy = 1.5*dy;
            xc = xm;
            yc = ym;
            fc = fm;
          }
          else {
            u = 0.4*dx-0.3*dy;
            dy = 0.4*dy+0.3*dx;
            dx = u;
          }
          u = fabs(xc)+fabs(yc);
          term = u+(fabs(dx)+fabs(dy))*factor;
          if ((u==term)||(fc==0))
            conv = TRUE;
        }
        u = 0.0;
        v = 0.0;
        k = 2.0*xc;
        m = xc*xc /*pow(xc,2.0)*/;

        for (j=0; j<=p; j++) {
#ifdef THINK_C
          w = k*u; w -= m*v;
          w += a[j];
#else
          w = a[j]+k*u-m*v;     /* Fails on the Mac */
#endif
          v = u;
          u = w;
        }

        if (pow(a[n1]+u*xc-m*v,2.0)<=fc) {
          u = 0.0;
          for (j=0; j<=p; j++) {
            a[j] = u*xc+a[j];
            u = a[j];
          }
          zerore[*pt] = xc;
          zeroim[*pt] = 0.0;
          *pt += 1;
        }
        else {
          u = 0.0;
          v = 0.0;
          k = 2.0*xc;
          m = xc*xc + yc*yc /*pow(xc,2.0)+pow(yc,2.0)*/;
          p = n1-2;
          for (j=0; j<=p; j++) {
            a[j] += k*u-m*v;
            w = a[j];
            v = u;
            u = w;
          }
          zerore[*pt] = xc;
          zeroim[*pt] = yc;
          *pt += 1;
          zerore[*pt] = xc;
          zeroim[*pt] = -yc;
          *pt += 1;
        }
        n1 = p;
      }
    }
    for (i=0; i<=n; i++)
      a[i] = work[i+1];
}

#ifdef _DEBUG

/*
 *
 * Resynthetize filter coefficients from poles values
 *
 */

static void synthetize(int    poleCount,
                       double *poleReal,
                       double *poleImag,
                       double *polyReal,
                       double *polyImag)
{
    int    j, k;
    double pr, pi, cr, ci;

    polyReal[0] = 1;
    polyImag[0] = 0;

    for (j=0; j<poleCount; j++) {
      polyReal[j+1] = 1;
      polyImag[j+1] = 0;

      pr = poleReal[j];
      pi = poleImag[j];

      for (k=j; k>=0; k--) {
        cr = polyReal[k];
        ci = polyImag[k];

        polyReal[k] = -(cr*pr-ci*pi);
        polyImag[k] = -(ci*pr+cr*pi);

        if (k>0) {
            polyReal[k] += polyReal[k-1];
            polyImag[k] += polyImag[k-1];
        }
      }
    }

    /* Makes it 1+a1.x+...+anXn */

    pr = polyReal[0];
    for (j=0; j<=poleCount; j++)
      polyReal[j] /= pr;
}

#endif

static void InvertPoles(int count, double *real, double *imag)
{
    int    i;
    double pr,pi,mag;

    for (i=0; i<count; i++) {
      pr = real[i];
      pi = imag[i];
      mag = pr*pr+pi*pi;
      real[i] = pr/mag;
      imag[i] = -pi/mag;
    }
}

#ifdef TRACE_POLES

static void DumpPoles(ENVIRON *csound,
                      int poleCount, double *part1, double *part2,
                      int isMagn, char *where)
{
    int i;

    csound->Message(csound, "%s\n", where);
    for (i=0; i<poleCount; i++) {
      if (isMagn)
        csound->Message(csound, Str("magnitude: %f   Phase: %f\n"), part1[i], part2[i]);
      else
        csound->Message(csound, Str("Real: %f   Imag: %f\n"), part1[i], part2[i]);
    }
}

#endif

/*
 *
 *      This is where it started
 *
 */

static int lpanal(ENVIRON *csound, int argc, char **argv)
{
    SNDFILE *infd;
    int     slice, analframes, counter, channel;
    MYFLT   *coef, beg_time, input_dur, sr = FL(0.0);
    char    *infilnam, *outfilnam;
    int     ofd;
    double  errn, rms1, rms2, filterCoef[MAXPOLES+1];
    MYFLT   *sigbuf, *sigbuf2;      /* changed from short */
    long    n;
    int     nb, osiz, hsize;
    LPHEADER    *lph;
    char    *lpbuf, *tp;
    MYFLT   pchlow, pchhigh;
    SOUNDIN *p;             /* struct allocated by SAsndgetset */
    LPC     lpc;

/* Added by MR to handle pole storage */

    int     i, j, indic, storePoles;
    int     poleFound;
    double  pr, pi, pm, pp, dPI;
    double  polePart1[MAXPOLES], polePart2[MAXPOLES];
    double  z1, workArray1[MAXPOLES];
#ifdef _DEBUG
    double  polyReal[MAXPOLES], polyImag[MAXPOLES];
#endif

    lpc.debug=0;
    lpc.verbose=0;
    lpc.doPitch = 1;

 /* csound->dbfs_to_float = csound->e0dbfs = FL(1.0);   Needed ?    */

   /* Allocates for analysis result buffer */

    lpbuf = csound->Calloc(csound, LPBUFSIZ);

   /* Header is defined at buffer startup */
    lph = (LPHEADER *) lpbuf;
    tp = lph->text;

   /* Define default values */
    lpc.poleCount = DEFpoleCount;         /* DEFAULTS... */
    slice = DEFSLICE;
    channel = 1;
    beg_time = FL(0.0);
    input_dur = FL(0.0);    /* default duration is to length of ip */
    *tp = '\0';
    pchlow = PITCHMIN;
    pchhigh = PITCHMAX;
    dPI = atan2(0,-1);

    /* Default is to store filter coefficients */
    storePoles = FALSE;

   /* Parse argument until no more found %-( */
    if (!(--argc))
      lpdieu(csound, Str("insufficient arguments"));
    do {
      char *s = *++argv;
      if (*s++ == '-')
        switch (*s++) {
        case 'j':       FIND("")
                          while (*s++); s--;
        break;
        case 's':       FIND(Str("no sampling rate"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&sr); break;
#else
        sscanf(s,"%f",&sr); break;
#endif
        case 'c':       FIND(Str("no channel"))
                          sscanf(s,"%d",&channel); break;
        case 'b':       FIND(Str("no begin time"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&beg_time); break;
#else
        sscanf(s,"%f",&beg_time); break;
#endif
        case 'd':       FIND(Str("no duration time"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&input_dur); break;
#else
        sscanf(s,"%f",&input_dur); break;
#endif
        case 'p':       FIND(Str("no poles"))
                          sscanf(s,"%d",&lpc.poleCount); break;
        case 'h':       FIND(Str("no hopsize"))
                          sscanf(s,"%d",&slice); break;
        case 'C':       FIND(Str("no comment string"))
                          strncat(tp,s,(LPBUFSIZ - sizeof(LPHEADER) + 4));
          tp += strlen(tp);
          break;
        case 'P':       FIND(Str("no low frequency"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&pchlow);
#else
          sscanf(s,"%f",&pchlow);
#endif
          if (pchlow == 0.) lpc.doPitch = 0;     /* -P0 inhibits ptrack */
          break;
        case 'Q':       FIND(Str("no high frequency"))
#if defined(USE_DOUBLE)
                          sscanf(s,"%lf",&pchhigh); break;
#else
        sscanf(s,"%f",&pchhigh); break;
#endif
        case 'v':       FIND(Str("no verbose level"))
                          sscanf(s,"%d",&lpc.verbose);
          if (lpc.verbose > 1)  lpc.debug = 1;
          break;
        case 'a':       storePoles=TRUE;
          break;
        default:
          {
            char errmsg[256];
            sprintf(errmsg,Str("unrecognised flag -%c"), *--s);
            lpdieu(csound, errmsg);
          }
        }
      else break;
    } while (--argc);

    /* Do some checks on arguments we got */

    if (argc != 2)
      lpdieu(csound, Str("incorrect number of filenames"));
    infilnam = *argv++;
    outfilnam = *argv;
    if (lpc.poleCount > MAXPOLES)
      quit(csound,Str("poles exceeds maximum allowed"));
    /* Allocate space now */
    coef = (MYFLT*) csound->Malloc(csound, (NDATA+lpc.poleCount*2)*sizeof(MYFLT));
    /* Space allocated */
    if (slice < lpc.poleCount * 5)
      if (csound->oparms->msglevel & WARNMSG)
        csound->Message(csound,Str("WARNING: hopsize may be too small, "
                   "recommend at least poleCount * 5\n"));

    if ((lpc.WINDIN = slice * 2) > MAXWINDIN)
      quit(csound,Str("input framesize (inter-frame-offset*2) exceeds maximum allowed"));
    if ((input_dur < 0) || (beg_time < 0))
      quit(csound,Str("input and begin times cannot be less than zero"));

    if (lpc.verbose) {
      csound->Message(csound,
                      Str("Reading sound from %s, writing lpfile to %s\n"),
                      infilnam, outfilnam);
      csound->Message(csound,
                      Str("poles=%d hopsize=%d begin=%4.1f"
                          " duration=%4.1f\n"),
                      lpc.poleCount, slice, beg_time, input_dur);
      csound->Message(csound,Str("lpheader comment:\n%s\n"), lph->text);
      if (pchlow > 0.)
        csound->Message(csound,
                        Str("pch track range: %5.1f - %5.1f Hz\n"),
                        pchlow,pchhigh);
      else csound->Message(csound,Str("pitch tracking inhibited\n"));
    }
    if ((input_dur < 0) || (beg_time < 0))
      quit(csound,Str("input and begin times cannot be less than zero"));

    if (storePoles)
      csound->Message(csound,Str("Using pole storage method\n"));
    else
      csound->Message(csound,Str("Using filter coefficient storage method\n"));

    /* Get information on input sound */
    if ((infd = csound->SAsndgetset(csound, infilnam, &p, &beg_time,
                                    &input_dur, &sr, channel)) < 0) {
      char errmsg[256];
      sprintf(errmsg,Str("error while opening %s"), infilnam);
      quit(csound, errmsg);
    }

    /* Try to open output file */
    if (csound->FileOpen(csound,
                         &ofd, CSFILE_FD_W, outfilnam, NULL, "") == NULL)
      quit(csound, Str("cannot create output file"));

    /* Prepare header */

    if (storePoles)
      lph->lpmagic = LP_MAGIC2;
    else
      lph->lpmagic = LP_MAGIC;

    lph->npoles = lpc.poleCount;
    lph->nvals = lpc.poleCount*(storePoles?2:1) + NDATA;
    lph->srate = (MYFLT)p->sr;
    lph->framrate = (MYFLT) p->sr / slice;
    lph->duration = input_dur;
    hsize = tp - (char *) lph;              /* header size including text */
    lph->headersize = (hsize + 3) & -4;     /* rounded up to 4 byte bndry */
    if (lph->headersize > LPBUFSIZ)    /* UNNECESSARY ?? */
      lph->headersize = LPBUFSIZ;

    /* Write header to disk */
    if ((nb = write(ofd,(char *)lph,(int)lph->headersize)) <
        lph->headersize)
      quit(csound,Str("cannot write header"));

    /* get buffer size for one analysis frame:
       filtercoef or poles + freq/rms/... */
    osiz = (lpc.poleCount*(storePoles?2:1) + NDATA) * sizeof(MYFLT);

    /* Allocate signal buffer for sound frame */
    sigbuf = (MYFLT *) csound->Malloc(csound, (long)lpc.WINDIN * sizeof(MYFLT));
    sigbuf2 = sigbuf + slice;

    /* Try to read first frame in buffer */
    if ((n = csound->getsndin(csound, infd, sigbuf, lpc.WINDIN, p)) < lpc.WINDIN)
      quit(csound,Str("soundfile read error, could not fill first frame"));

    /* initialize frame pitch table ? */
    if (lpc.doPitch)
      ptable(csound, pchlow, pchhigh, (MYFLT) p->sr, lpc.WINDIN);

    /* Initialise for analysis */
    counter = 0;
    analframes = (p->getframes - 1) / slice;

    /* Some display stuff */
#if 0
    dispinit(csound);
    csound->dispset(csound, &lpc.pwindow, coef + 4, lpc.poleCount,
                    "pitch: 0000.00   ", 0, "LPC/POLES");
#endif
    /* Space for a array */
    lpc.a = (double (*)[MAXPOLES]) csound->Malloc(csound, MAXPOLES * MAXPOLES * sizeof(double));
    lpc.x = (double *) csound->Malloc(csound, lpc.WINDIN * sizeof(double));  /* alloc a double array */
#ifdef TRACE
    csound->FileOpen(csound, &trace, CSFILE_STD, "lpanal.trace", "w", NULL);
#endif
    /* Do the analysis */
    do {
      MYFLT *fp1;
      double *dfp;

      /* Analyze current frame */
#ifdef TRACE_POLES
      csound->Message(csound, "Starting new frame...\n");
#endif
      counter++;
      alpol(csound, &lpc, sigbuf, &errn, &rms1, &rms2, filterCoef);
      /* Transfer results */
      coef[0] = (MYFLT)rms2;
      coef[1] = (MYFLT)rms1;
      coef[2] = (MYFLT)errn;
      if (lpc.doPitch)
        coef[3] = getpch(csound, sigbuf);
      else coef[3] = FL(0.0);
      if (lpc.debug) csound->Message(csound,"%d\t%9.4f\t%9.4f\t%9.4f\t%9.4f\n",
                                 counter, coef[0], coef[1], coef[2], coef[3]);
#ifdef TRACE
      if (lpc.debug) fprintf(trace,"%d\t%9.4f\t%9.4f\t%9.4f\t%9.4f\n",
                         counter, coef[0], coef[1], coef[2], coef[3]);
#endif
  /*  for (fp1=coef+NDATA, dfp=cc+poleCount, n=poleCount; n--; ) */
  /*    *fp1++ = - (MYFLT) *--dfp; */  /* rev coefs & chng sgn */
#if 0
      sprintf(lpc.pwindow.caption, "pitch: %8.2f", coef[3]);
      display(csound, &lpc.pwindow);
#endif

      /* Prepare buffer for output */

      if (storePoles) {
        /* Treat (swap) filter coefs for resolution */
        filterCoef[lpc.poleCount] = 1.0;
        for (i=0; i<(lpc.poleCount+1)/2; i++) {
          j = lpc.poleCount-1-i;
          z1 = filterCoef[i];
          filterCoef[i] = filterCoef[j];
          filterCoef[j] = z1;
        }

        /* Get the Filter Poles */

        polyzero(100,lpc.poleCount,filterCoef,polePart1,polePart2,
                 &poleFound,2000,&indic,workArray1);

        if (poleFound!=lpc.poleCount) {
          csound->Message(csound,
                          Str("Found only %d poles...sorry\n"), poleFound);
          return -1;
        }
        InvertPoles(lpc.poleCount,polePart1,polePart2);

#ifdef TRACE_POLES
        DumpPoles(csound,
                  lpc.poleCount, polePart1, polePart2, 0, "Extracted Poles");
#endif

#ifdef _DEBUG
        /* Resynthetize the filter for check */
        InvertPoles(lpc.poleCount,polePart1,polePart2);

        synthetize(lpc.poleCount,polePart1,polePart2,polyReal,polyImag);

        for (i=0; i<lpc.poleCount; i++) {
#ifdef TRACE_FILTER
          csound->Message(csound, "filterCoef: %f\n", filterCoef[i]);
#endif
          if (filterCoef[i]-polyReal[poleCount-i]>1e-10)
            csound->Message(csound, Str("Error in coef %d : %f <> %f \n"),
                                    i, filterCoef[i], polyReal[poleCount-i]);
        }
        csound->Message(csound,".");
        InvertPoles(lpc.poleCount,polePart1,polePart2);
#endif
        /* Switch to pole magnitude and phase */

        for (i=0; i<lpc.poleCount;i++) {
          /* Store magnitude and phase (PI,-PI) */
          pr = polePart1[i];
          pi = polePart2[i];
          pm = sqrt(pr*pr+pi*pi);
          if (pm!=0) {
            pp = atan2(pi,pr);
            if (pp>dPI)
              pp = 2*dPI-pp;
          }
          else
            pp = 0;
          polePart1[i] = pm;
          polePart2[i] = pp;
        }

  /*    DumpPoles(csound, poleCount,polePart1,polePart2,1,"About to store"); */

        /* Store in output buffer */
        fp1 = coef+NDATA;
        for (i=0; i<lpc.poleCount;i++) {
          *fp1++ = (MYFLT)polePart1[i];
          *fp1++ = (MYFLT)polePart2[i];
        }
      }
      else {
        /* Move filter data into output buffer */
        dfp = filterCoef+lpc.poleCount;
        fp1 = coef+NDATA;
        for (n=0;n<lpc.poleCount; n++)
          *fp1++ = - (MYFLT) *--dfp;
      }

      /* Write frame to disk */
      if ((nb = write(ofd, (char *)coef, osiz)) != osiz)
        quit(csound,Str("write error"));
      memcpy(sigbuf, sigbuf2, sizeof(MYFLT)*slice);

      /* Some unused stuff. I think from when all snd was in mem */
      /*  ( MYFLT *fp2; for (fp1=sigbuf, fp2=sigbuf2, n=slice; n--; ) */
      /* move slice forward */
      /*              *fp1++ = *fp2++;} */

      /* Get next sound frame */
      if ((n = csound->getsndin(csound, infd, sigbuf2, slice, p)) == 0)
        break;          /* refill til EOF */
#if 0
      if (!csoundYield(csound)) break;
#endif
    } while (counter < analframes); /* or nsmps done */
#if 0
    /* clean up stuff */
    dispexit(csound);
#endif
    csound->Message(csound, Str("%d lpc frames written to %s\n"),
                            counter, outfilnam);
    csound->Free(csound, lpc.a);
    csound->Free(csound, lpc.x);
    csound->Free(csound, coef);

    return 0;
}

static void quit(ENVIRON *csound, char *msg)
{
    csound->Message(csound,"lpanal: %s\n", msg);
    csound->Die(csound, Str("analysis aborted"));
}

static void lpdieu(ENVIRON *csound, char *msg)
{
    usage(csound);
    csound->Die(csound, "lpanal: %s\n", msg);
}

/*
 *
 *  This is where the frame analysis is done
 *
 */

static void alpol(ENVIRON *csound, LPC *thislp, MYFLT *sig, double *errn,
                  double *rms1, double *rms2, double b[MAXPOLES])
                                        /* sig now MYFLT */
                                        /* b filled here */
{
    double v[MAXPOLES];
    double *xp;
    double sum, sumx, sumy;
    int i, j, k, limit;

   /* Transfer signal in x array */
    for (xp=thislp->x; xp-thislp->x < thislp->WINDIN;++xp,++sig)/*   &  copy sigin into */
      *xp = (double) *sig;

   /* Build system to be solved */
    for (i=0; i < thislp->poleCount;++i)  {
      sum = (double) 0.0;
      for (k=thislp->poleCount; k < thislp->WINDIN;++k)
        sum += thislp->x[k-(i+1)] * thislp->x[k];
      v[i] = -sum;
      if (i != thislp->poleCount - 1)  {
        limit = thislp->poleCount - (i+1);
        for (j=0; j < limit; j++)  {
          sum += thislp->x[thislp->poleCount-(i+1)-(j+1)] * thislp->x[thislp->poleCount-(j+1)]
            - thislp->x[thislp->WINDIN-(i+1)-(j+1)] * thislp->x[thislp->WINDIN-(j+1)];
          thislp->a[(i+1)+j][j] = thislp->a[j][(i+1)+j] = sum;
        }
      }
    }
    sum = (double) 0.0;
    for (k=thislp->poleCount; k < thislp->WINDIN;++k)
      sum +=  thislp->x[k]*thislp->x[k];
    sumy = sumx = sum;
#ifdef TRACE
    fprintf(trace, "sumy=%f\n", sumy);
#endif
    for (j=0; j < thislp->poleCount; j++)  {
      sum += thislp->x[thislp->poleCount-(j+1)]*thislp->x[thislp->poleCount-(j+1)] -
        thislp->x[thislp->WINDIN-(j+1)]*thislp->x[thislp->WINDIN-(j+1)];
      thislp->a[j][j] = sum;
    }

    /* Solves the system */
    gauss(csound, thislp, thislp->a, v, b);

    /* Compute associted parameters */
    for (i=0; i < thislp->poleCount;++i) {
      sumy -= b[i]*v[i];
#ifdef TRACE
      fprintf(trace, "b[i]=%f v[i]=%f sumy=%f\n", b[i], v[i], sumy);
#endif
    }
    *rms1 = sqrt(sumx / (thislp->WINDIN - thislp->poleCount) );
    *rms2 = sqrt(sumy / (thislp->WINDIN - thislp->poleCount) );
    /*        sum = (*rms2)/(*rms1);  *errn = sum*sum; */
    *errn = sumy/sumx;
}

/*
 *
 * Perform gauss elemination: Could be replaced by something more robust
 *
 */
static void gauss(ENVIRON *csound, LPC* thislp,
                  double (*a/*old*/)[MAXPOLES], double *bold, double b[])
{
    double amax, dum, pivot;
    double c[MAXPOLES];
    int i, j, k, l, istar=-1, ii, lp;

    /* bold untouched by this subroutine */
    for (i=0; i < thislp->poleCount;++i)  {
      c[i] = bold[i];
#ifdef TRACE
      fprintf(trace, "c[%d]=%f\n", i, c[i]);
#endif
    }
    /* eliminate i-th unknown */
    for (i=0; i < thislp->poleCount - 1;++i)  {        /* find largest pivot */
      amax = 0.0;
      for (ii=i; ii < thislp->poleCount;++ii)  {
        double npq = fabs(a[ii][i]);
        if (npq >= amax)  {
          istar = ii;
          amax = npq;
        }
      }
      if (amax < 1e-20) {
        csound->Message(csound,"Row %d or %d have maximum of %g\n",
                        i, thislp->poleCount, amax);
        csound->Die(csound, Str("gauss: ill-conditioned"));
      }
      if (i != istar) {
        for (j=0; j < thislp->poleCount;++j)  {    /* switch rows */
          dum = a[istar][j];
          a[istar][j] = a[i][j];
          a[i][j] = dum;
        }
        dum = c[istar];
        c[istar] = c[i];
        c[i] = dum;
      }
#ifdef TRACE
      fprintf(trace, "c[%d]=%f\n", i, c[i]);
#endif
      amax = a[i][i];           /* To save recalculation */
      for (j=i+1; j < thislp->poleCount;++j)  {             /* pivot */
        pivot = a[j][i] / amax;
#ifdef TRACE
        fprintf(trace, "pivot = %f c[%d] = %f\n", pivot, j, c[j]);
#endif
        c[j] = c[j] - pivot * c[i];
#ifdef TRACE
        fprintf(trace, "c[%d]=%f c[%d]=%f\n", j, c[j], i, c[i]);
#endif
        for (k=0; k < thislp->poleCount; ++k)
          a[j][k] = a[j][k] - pivot * a[i][k];
      }
    }                               /* return if last pivot is too small */
    if (fabs(a[thislp->poleCount-1][thislp->poleCount-1]) < 1e-20) {
      csound->Message(csound,"Row %d or %d have maximum of %g\n",
             thislp->poleCount-1, thislp->poleCount,
             fabs(a[thislp->poleCount-1][thislp->poleCount-1]));
      csound->Die(csound, Str("gauss: ill-conditioned"));
    }

    b[thislp->poleCount-1] = c[thislp->poleCount-1] / a[thislp->poleCount-1][thislp->poleCount-1];
#ifdef TRACE
    fprintf(trace, "b[poleCount-1]=%f c[poleCount-1]=%f a[][]=%f\n",
            b[thislp->poleCount-1], c[thislp->poleCount-1], a[thislp->poleCount-1][thislp->poleCount-1]);
#endif
    for (k=0; k<thislp->poleCount-1; ++k)  {   /* back substitute */
      l = thislp->poleCount-1 -(k+1);
      b[l] = c[l];
      lp = l + 1;
      for (j = lp; j<thislp->poleCount; ++j) {
        b[l] -= a[l][j] * b[j];
      }
      b[l] /= a[l][l];
    }
}

static const char *usage_txt[] = {
    "USAGE:\tlpanal [flags] infilename outfilename",
    "\twhere flag options are:",
    "-s<srate>\tinput sample rate (defaults to header else 44100)",
    "-c<chnlreq>\trequested channel of sound (default chan 1)",
    "-b<begin>\tbegin time in seconds into soundfile (default 0.0)",
    "-d<duration>\tseconds of sound to be analysed (default: to EOF)",
    "-p<npoles>\tnumber of poles for analysis (default 34)",
    "-h<hopsize>\toffset between frames in samples (default 200)",
    "\t\t\t(framesize will be twice <hopsize>)",
    "-C<string>\tcomment field of lp header (default empty)",
    "-P<mincps>\tlower limit for pitch search (default 70 Hz)",
    "\t\t\t(-P0 inhibits pitch tracking)",
    "-Q<maxcps>\tupper limit for pitch search (default 200 Hz)",
    "-v<verblevel>\tprinting verbosity: 0=none, 1=verbose, 2=debug (default 0)",
    "-g\tgraphical display of results",
    "-a\t\talternate (pole) file storage",
    "-- fname\tLog output to file",
    "see also:  Csound Manual Appendix",
    NULL
};

static void usage(ENVIRON *csound)
{
    int i;
    for (i = 0; usage_txt[i] != NULL; i++)
      csound->Message(csound, "%s\n", Str(usage_txt[i]));
}

#define FREQS  50
#define NN     5
#define NP     6  /* NN+1 */
#define HWIN   50 /* MAXWINDIN/20, Max Hwind */

static MYFLT   *tphi[FREQS],*tpsi[FREQS]; /* prv tphi[50][5][25],tpsi[50][6][25] */
static MYFLT   *tgamph[FREQS], *tgamps[FREQS], freq[FREQS];
          /* prv tgamph[50][5],  tgamps[50][6] */
static MYFLT    NYQ10;
static int      Windsiz, Windsiz2;         /* settable windowsize, halfthat */
static int      Dwind, Hwind;              /* settable downsamp10, halfthat */

typedef MYFLT (*phi_typ)[HWIN];
typedef MYFLT (*psi_typ)[HWIN];
static  void   trigpo(MYFLT, phi_typ, psi_typ, MYFLT *, MYFLT *, int);
static  MYFLT  lowpass(MYFLT);
static  MYFLT  search(MYFLT *fm, MYFLT qsum, MYFLT g[], MYFLT h[]);

static void trigpo(MYFLT omega,
                   phi_typ phi, psi_typ psi, MYFLT *gamphi, MYFLT *gampsi, int n)
/* dimensions:   phi[NN][HWIN], psi[NP][HWIN], gamphi[NN], gampsi[NP]  */
{
    int    j=0, k, np;
    double alpha, beta, gamma, wcos[HWIN], wsin[HWIN];
    double p, z, a, b, yy;

    np = n+1;
    for (k=0;  k<Hwind;  ++k) {
      yy = omega * (MYFLT)k;
      wcos[k] = cos(yy);
      wsin[k] = sin(yy);
    }
    beta = gamma = 0.0;
    for (k=0;  k<Hwind;  ++k) {
      p = wsin[k];
      z = p * p;
      beta += z * wcos[k];
      gamma += z;
      phi[0][k] = (MYFLT)p;
    }
    gamphi[0] = (MYFLT)gamma;
    a = 2.0 * beta/gamma;
    alpha = beta = gamma = 0.0;
    for (k=0;  k<Hwind;  ++k) {
      p = (2.0 * wcos[k]-a) * phi[0][k];
      alpha += wcos[k] * p * phi[0][k];
      beta += wcos[k] * ( p * p );
      gamma +=  p * p;
      phi[1][k] = (MYFLT)p;
    }
    gamphi[1] = (MYFLT)gamma;
    a = 2.0 * beta/gamma;
    b = 2.0 *alpha/gamphi[0];
    for (j=2;  j<n;  ++j) {
      alpha = beta = gamma = 0.0;
      for (k=0;  k< Hwind;  ++k)  {
        p = (2.0 * wcos[k] - a ) * phi[j-1][k] - b * phi[j-2][k];
        alpha += wcos[k] * p * phi[j-1][k];
        beta += wcos[k] * (p * p);
        gamma += (p * p);
        phi[j][k] = (MYFLT)p;
      }
      gamphi[j] = (MYFLT)gamma;
      a = 2.0 * beta/gamma;
      b = 2.0 *alpha/gamphi[j-1];
    }
    beta = 0.0;
    gamma = (double) Hwind;
    for ( k=0; k < Hwind;  ++k) {
      beta += wcos[k];
      psi[0][k] = FL(1.0);
    }
    gampsi[0] = (MYFLT)gamma;
    a = beta/gamma;
    alpha = beta = gamma = 0.0;
    for ( k=0;  k < Hwind;  ++k) {
      p = wcos[k]-a;
      alpha += wcos[k] * p*psi[0][k];
      beta += wcos[k] * ( p * p );
      gamma += (p * p);
      psi[1][k] = (MYFLT)p;
    }
    gampsi[1] = (MYFLT)gamma;
    a = 2.0 * beta / gamma;
    b = 2.0 * alpha / gampsi[0];
    for (j=2;  j<np; ++j) {
      alpha = beta = gamma = 0.0;
      for (k=0; k < Hwind;  ++k) {
        p = (2.0 * wcos[k]-a)* psi[j-1][k]-b*psi[j-2][k];
        alpha += wcos[k]*p*psi[j-1][k];
        beta += wcos[k]* (p*p);
        gamma += (p*p);
        psi[j][k] = (MYFLT)p;
      }
      gampsi[j] = (MYFLT)gamma;
      a = 2.0 * beta/gamma;
      b = 2.0 * alpha/gampsi[j-1];
    }
}

static MYFLT search(MYFLT *fm, MYFLT qsum, MYFLT g[], MYFLT h[])
{
    MYFLT fun[FREQS], funmin = FL(1.e10);
    MYFLT sum, f1, f2, f3, x0, x1, x2, x3, a, b, c, ftemp;
    int   i, istar = 0, n, np, j, k;

    for (i=0;  i < FREQS;  ++i) {
      MYFLT (*tphii)[HWIN], (*tpsii)[HWIN];
      MYFLT  *tgamphi, *tgampsi;
      tphii = (MYFLT (*)[HWIN]) tphi[i];    /* dim [][NN][HWIN] */
      tpsii = (MYFLT (*)[HWIN]) tpsi[i];    /* dim [][NP][HWIN] */
      tgamphi = tgamph[i];                  /* dim [][NN]       */
      tgampsi = tgamps[i];                  /* dim [][NP]       */
      n = (int)(NYQ10 / freq[i]);
      if (n > NN)  n = NN;
      np = n+1;
      sum = FL(0.0);
      for (j=0;  j < n;  ++j) {
        c = FL(0.0);
        for (k=0;  k< Hwind;  ++k)
          c += g[k] * tphii[j][k];
        sum += (c*c) / tgamphi[j];
      }
      for (j=0;  j<np;  ++j) {
        c = FL(0.0);
        for (k=0;  k < Hwind;  ++k)
          c += h[k] * tpsii[j][k];
        sum += (c*c) / tgampsi[j];
      }
      fun[i] = ftemp = qsum - sum;      /* store the least sqr vals */
      if (ftemp < funmin) {
        funmin = ftemp;               /*   but remember minimum   */
        istar = i;
      }
    }
    if (istar == 0 || istar == 49) {
      *fm = fun[istar];
      return (freq[istar]);
    }
    x1 = freq[istar-1];
    f1 = fun[istar-1];
    x2 = freq[istar];
    f2 = fun[istar];
    x3 = freq[istar+1];
    f3 = fun[istar+1];
    a = f3/((x3-x1)*(x3-x2));
    b = f2/((x2-x1)*(x2-x3));
    c = f1/((x1-x2)*(x1-x3));
    x0 = FL(0.5)*(a*(x1+x2)+b*(x1+x3)+c*(x2+x3))/(a+b+c);
    *fm = a*(x0-x1)*(x0-x2)+b*(x0-x1)*(x0-x3)+c*(x0-x2)*(x0-x3);
    return (x0);
}

static MYFLT lowpass(MYFLT x)           /* x now MYFLT */
{
#define c FL(0.00048175311)

#define a1 -FL(1.89919924)
#define c1 -FL(1.92324804)
#define d1 FL(0.985720370)

#define a2 -FL(1.86607670)
#define c2 -FL(1.90075003)
#define d2 FL(0.948444690)

#define a3 -FL(1.66423461)
#define c3 -FL(1.87516686)
#define d3 FL(0.896241023)

#define c4 -FL(0.930449120)

static  MYFLT w1 = FL(0.0), w11 = FL(0.0), w12 = FL(0.0);
static  MYFLT w2 = FL(0.0), w21 = FL(0.0), w22 = FL(0.0);
static  MYFLT w3 = FL(0.0), w31 = FL(0.0), w32 = FL(0.0);
static  MYFLT w4 = FL(0.0), w41 = FL(0.0), w42 = FL(0.0);
        MYFLT temp,y;

        w1 = c*x - c1*w11 - d1*w12;
        temp = w1 + a1*w11 + w12;
        w12 = w11;
        w11 = w1;
        w2 = temp - c2*w21 - d2*w22;
        temp = w2 + a2*w21 + w22;
        w22 = w21;
        w21 = w2;
        w3 = temp - c3*w31 - d3*w32;
        temp = w3 + a3*w31 + w32;
        w32 = w31;
        w31 = w3;
        w4 = temp - c4*w41;
        y = w4 + w41;
        w42 = w41;   /* w42 set but not used in lowpass */
        w41 = w4;
        return(y);
}

static MYFLT getpch(ENVIRON *csound, MYFLT *sigbuf)
{
static  int   firstcall = 1, tencount = 0;
static  MYFLT *Dwind_dbuf, *Dwind_end1;    /* double buffer for downsamps   */
static  MYFLT *dbp1, *dbp2;
        MYFLT g[HWIN], h[HWIN], fm, qsum, y, *inp;
        int   n;

        if (firstcall) {                   /* on first call, alloc dbl dbuf  */
          Dwind_dbuf = (MYFLT *) csound->Calloc(csound, (long)Dwind * 2 * sizeof(MYFLT));
          Dwind_end1 = Dwind_dbuf + Dwind;
          dbp1 = Dwind_dbuf;             /*   init the local Dsamp pntrs */
          dbp2 = Dwind_end1;             /*   & process the whole inbuf  */
          for (inp = sigbuf, n = Windsiz; n--; ) {
            y = lowpass(*inp++);            /* lowpass every sample  */
            if (++tencount == 10) {
              tencount = 0;
              *dbp1++ = y;                /*    & save every 10th  */
              *dbp2++ = y;
              if (dbp1 >= Dwind_end1) {
                dbp1 = Dwind_dbuf;
                dbp2 = Dwind_end1;
              }
            }
          }
          firstcall = 0;
        }
        else {                           /* other calls: process only inbuf2  */
          for (inp = sigbuf+Windsiz2, n = Windsiz2; n--; ) {
            y = lowpass(*inp++);            /* lowpass every sample  */
            if (++tencount == 10) {
              tencount = 0;
              *dbp1++ = y;                /*    & save every 10th  */
              *dbp2++ = y;
              if (dbp1 >= Dwind_end1) {
                dbp1 = Dwind_dbuf;
                dbp2 = Dwind_end1;
              }
            }
          }
        }
        {
          MYFLT *gp, *hp, *sp1, *sp2;
          qsum = FL(0.0);
          gp = g; hp = h;
          sp1 = sp2 = dbp1 + Hwind - 1;
          for (n = Hwind; n--; gp++, hp++, sp1++, sp2-- ) {
            *gp = FL(0.5) * (*sp1 - *sp2);        /* get sum & diff pairs */
            *hp = FL(0.5) * (*sp1 + *sp2);
            qsum += *gp * *gp + *hp * *hp;   /* accum sum of squares */
          }
        }
        return ( search(&fm, qsum, g, h) );
}

static void ptable(ENVIRON *csound,
                   MYFLT fmin, MYFLT fmax, MYFLT sr, int windsiz)
{
    int   i, n;
    MYFLT omega, fstep, tpidsrd10;

    if ((n = HWIN * 20) != MAXWINDIN)
      csound->Die(csound, Str("LPTRKFNS: inconsistent MAXWindow defines"));
    NYQ10   = sr/FL(20.0);
    Windsiz = windsiz;              /* current windin size */
    Windsiz2 = windsiz/2;           /* half of that        */
    Dwind   = windsiz/10;           /* downsampled windsiz */
    Hwind   = (Dwind+1)/2;          /* half of that        */
    if (Hwind > HWIN)
      csound->Die(csound, Str("LPTRKFNS: called with excessive Windsiz"));
    tpidsrd10 = TWOPI_F / (sr/FL(10.0));
    fstep = (fmax - fmin) / FREQS;    /* alloc & init each MYFLT array  */
    for (i=0;  i<FREQS; ++i) {        /*   as if MAX dimension of Hwind */
      tphi[i] = (MYFLT *) csound->Calloc(csound, (long)NN * HWIN * sizeof(MYFLT));
      tpsi[i] = (MYFLT *) csound->Calloc(csound, (long)NP * HWIN * sizeof(MYFLT));
      tgamph[i] = (MYFLT *) csound->Calloc(csound, (long)NN * sizeof(MYFLT));
      tgamps[i] = (MYFLT *) csound->Calloc(csound, (long)NP * sizeof(MYFLT));
      freq[i] = fmin + (MYFLT)i * fstep;
      n = (int)(NYQ10 / freq[i]);
      if (n > NN)  n = NN;
      omega = freq[i] * tpidsrd10;
      trigpo(omega,(phi_typ)tphi[i],(psi_typ)tpsi[i],tgamph[i],tgamps[i],n);
    }
}

/* module interface */

PUBLIC int csoundModuleCreate(ENVIRON *csound)
{
    int retval = csound->AddUtility(csound, "lpanal", lpanal);
    if (!retval) {
      retval = csound->SetUtilityDescription(csound, "lpanal",
                    "Linear predictive analysis for lpread");
    }
    return retval;
}

