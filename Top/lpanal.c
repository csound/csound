/*  
    lpanal.c:

    Copyright (C) 1992, 1997 Paul Lansky, Barry Vercoe, John ffitch, Marc Resibois

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

#include "cs.h"                                             /*  LPANAL.C      */
#include "soundio.h"
#include "lpc.h"
#include "cwindow.h"
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* LPC analysis, modified by BV 8'92 for linkage to audio files via soundin.c.
 * Currently set for maximum of 50 poles, & max anal segment of 1000 samples,
 * meaning that the frame slices cannot exceed 500 samples.
 * Program size expands linearly with slice size, and as square of poleCount.
 */

#define DEFpoleCount 34        /* recommended default (max 50 in lpc.h)    */
#define DEFSLICE 200        /* <= MAXWINDIN/2 (currently 1000 in lpc.h) */
#define PITCHMIN        FL(70.0)
#define PITCHMAX        FL(200.0)   /* default limits in Hz for pitch search */

static  int     poleCount, WINDIN, debug=0, verbose=0, doPitch = 1;
static  double  *x;
static  double  (*a)[MAXPOLES];

/* Forward declaration */

static  void    alpol(MYFLT *, double *, double *, double *, double *);
static  void    gauss(double (*)[MAXPOLES], double*, double*);
static  void    quit(char *), lpdieu(char *), usage(void);
extern  void    ptable(MYFLT, MYFLT, MYFLT, int);
extern  MYFLT   getpch(MYFLT*);
extern  int     openout(char*, int);
extern  OPARMS  O;

#ifdef mills_macintosh
#include "MacTransport.h"
#endif

#ifdef TRACE
FILE *trace;
#endif

static  WINDAT   pwindow;

/* Search for an argument and report of not found */
#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || (((s = *++argv)!=0) && *s == '-'))  \
                            lpdieu(MSG);

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

void polyzero(int nmax, int n, double *a, double *zerore, double *zeroim,
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


/*
 *
 * Resynthetize filter coefficients from poles values
 *
 */

void synthetize(int    poleCount,
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

void InvertPoles(int count, double *real, double *imag)
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

void
 DumpPolesF(int poleCount, MYFLT *part1, MYFLT *part2, int isMagn, char *where)
{
#ifdef TRACE_POLES
    int i;

    printf("%s\n", where);
    for (i=0; i<poleCount; i++) {
      if (isMagn)
        printf(Str(X_343,"magnitude: %f   Phase: %f\n"), part1[i], part2[i]);
      else
        printf(Str(X_441,"Real: %f   Imag: %f\n"), part1[i], part2[i]);
    }
#endif
}

void DumpPoles(int poleCount, double *part1, double *part2, int isMagn, char *where)
{
#ifdef TRACE_POLES
    int i;

    printf("%s\n", where);
    for (i=0; i<poleCount; i++) {
      if (isMagn)
        printf(Str(X_343,"magnitude: %f   Phase: %f\n"), part1[i], part2[i]);
      else
        printf(Str(X_441,"Real: %f   Imag: %f\n"), part1[i], part2[i]);
    }
#endif
}

void SortPoles(int poleCount, MYFLT *poleMagn, MYFLT *polePhas)
{
    int i, j;
    MYFLT diff, fTemp;
    int shouldSwap;

    /*  DumpPolesF(poleCount, poleMagn, polePhas, 1, "Before sort"); */

    for (i=1; i<poleCount; i++) {
      for (j=0; j<i; j++) {

        shouldSwap = FALSE;

        diff = (MYFLT)(fabs(polePhas[j])-fabs(polePhas[i]));
        if (diff>1e-10)
          shouldSwap = TRUE;
        else if (diff>-1e-10) {
          diff = poleMagn[j]-poleMagn[i];

          if (diff>1e-10)
            shouldSwap = TRUE;
          else if (diff>-1e-10)
            {
              if (polePhas[j]>polePhas[i])
                shouldSwap = TRUE;
            }
        }
        if (shouldSwap) {
          fTemp = poleMagn[i];
          poleMagn[i] = poleMagn[j];
          poleMagn[j] = fTemp;

          fTemp = polePhas[i];
          polePhas[i] = polePhas[j];
          polePhas[j] = fTemp;
        }
      }
    }
        /*      DumpPolesF(poleCount, poleMagn, polePhas, 1, "After sort"); */
}

int DoPoleInterpolation(int poleCount,
                        MYFLT *pm1, MYFLT *pp1,
                        MYFLT *pm2, MYFLT *pp2,
                        MYFLT factor, MYFLT *outMagn, MYFLT *outPhas)
{
    int i;

    if (poleCount%2!=0) {
      printf (Str(X_208,"Cannot handle uneven pole count yet \n"));
      return (FALSE);
    }

    for (i=0; i<poleCount; i++) {
      if (fabs(fabs(pp1[i])-PI)<1e-5) {
        pm1[i] = -pm1[i];
        pp1[i] = FL(0.0);
      }

      if (fabs(fabs(pp2[i])-PI)<1e-5) {
        pm2[i] = -pm2[i];
        pp2[i] = FL(0.0);
      }
    }

    /* Sort poles according to abs(phase) */

    SortPoles(poleCount, pm1, pp1);
    SortPoles(poleCount, pm2, pp2);

        /*      DumpPolesF(poleCount, pm1, pp1, 1, "Sorted poles 1"); */
        /*      DumpPolesF(poleCount, pm2, pp2, 1, "Sorted poles 2"); */

        /*      printf ("factor=%f\n", factor); */

    for (i=0; i<poleCount; i++) {
      outMagn[i] = pm1[i]+(pm2[i]-pm1[i])*factor;
      outPhas[i] = pp1[i]+(pp2[i]-pp1[i])*factor;
    }

    DumpPolesF(poleCount, outMagn, outPhas, 1, "Interpolated poles");

    return(TRUE);
}

/*
 *
 *      This is where it started
 *
 */

extern  int     SAsndgetset(char*, SOUNDIN **, MYFLT*, MYFLT*, MYFLT*, int);
extern  long    getsndin(int, MYFLT*, long, SOUNDIN*);

int lpanal(int argc, char **argv)
{
        int     slice, analframes, counter, channel;
        MYFLT   *coef, beg_time, input_dur, sr = FL(0.0);
        char    *infilnam, *outfilnam;
        int     infd, ofd;
        double  errn, rms1, rms2, filterCoef[MAXPOLES+1];
        MYFLT   *sigbuf, *sigbuf2;      /* changed from short */
        long    n;
        int     nb, osiz, hsize;
        LPHEADER    *lph;
        char    *lpbuf, *tp;
        MYFLT   pchlow, pchhigh;
        SOUNDIN *p;             /* struct allocated by SAsndgetset */

/* Added by MR to handle pole storage */

        int     i,j,indic,storePoles;
        int     poleFound;
        double  pr, pi, pm, pp, dPI;
        double  polePart1[MAXPOLES], polePart2[MAXPOLES];
        double  z1, workArray1[MAXPOLES];
#ifdef _DEBUG
        double  polyReal[MAXPOLES], polyImag[MAXPOLES];
#endif

        /* must set this for 'standard' behaviour when analysing
           (assume re-entrant Csound) */
        dbfs_init(DFLT_DBFS);

   /* Allocates for analysis result buffer */

        lpbuf = mcalloc((long)LPBUFSIZ);

   /* Header is defined at buffer startup */
        lph = (LPHEADER *) lpbuf;
        tp = lph->text;

   /* Define default values */
        poleCount = DEFpoleCount;         /* DEFAULTS... */
        slice = DEFSLICE;
        channel = 1;
        beg_time = FL(0.0);
        input_dur = FL(0.0);    /* default duration is to length of ip */
        *tp = '\0';
        pchlow = PITCHMIN;
        pchhigh = PITCHMAX;
        O.displays = 0;
        dPI = atan2(0,-1);

        /* Default is to store filter coefficients */
        storePoles = FALSE;

   /* Parse argument until no more found %-( */
        if (!(--argc))
            lpdieu(Str(X_939,"insufficient arguments"));
        do {
            char *s = *++argv;
            if (*s++ == '-')
                switch (*s++) {
                case 'j':       FIND("")
                                while (*s++); s--;
                                break;
                case 's':       FIND(Str(X_1057,"no sampling rate"))
                    sscanf(s,"%f",&sr); break;
                case 'c':       FIND(Str(X_1026,"no channel"))
                    sscanf(s,"%d",&channel); break;
                case 'b':       FIND(Str(X_1025,"no begin time"))
                    sscanf(s,"%f",&beg_time); break;
                case 'd':       FIND(Str(X_1030,"no duration time"))
                    sscanf(s,"%f",&input_dur); break;
                case 'p':       FIND(Str(X_1054,"no poles"))
                    sscanf(s,"%d",&poleCount); break;
                case 'h':       FIND(Str(X_1037,"no hopsize"))
                    sscanf(s,"%d",&slice); break;
                case 'C':       FIND(Str(X_1028,"no comment string"))
                    strncat(tp,s,(LPBUFSIZ - sizeof(LPHEADER) + 4));
                    tp += strlen(tp);
                    break;
                case 'P':       FIND(Str(X_1045,"no low frequency"))
                    sscanf(s,"%f",&pchlow);
                    if (pchlow == 0.) doPitch = 0;     /* -P0 inhibits ptrack */
                    break;
                case 'Q':       FIND(Str(X_1036,"no high frequency"))
                    sscanf(s,"%f",&pchhigh); break;
                case 'v':       FIND(Str(X_1065,"no verbose level"))
                    sscanf(s,"%d",&verbose);
                    if (verbose > 1)  debug = 1;
                    break;
                case 'g':       O.displays = 1;
                    break;
                case 'a':       storePoles=TRUE;
                    break;
                default: sprintf(errmsg,Str(X_1350,"unrecognised flag -%c"), *--s);
                         lpdieu(errmsg);
                }
            else break;
        } while (--argc);

   /* Do some checks on arguments we got */

        if (argc != 2)  lpdieu(Str(X_907,"incorrect number of filenames"));
        infilnam = *argv++;
        outfilnam = *argv;
        if (poleCount > MAXPOLES)
            quit(Str(X_1132,"poles exceeds maximum allowed"));
                                /* Allocate space now */
        coef = (MYFLT*) mmalloc((NDATA+poleCount*2)*sizeof(MYFLT));
                                /* Space allocated */
        if (slice < poleCount * 5)
          if (O.msglevel & WARNMSG)
            printf(Str(X_831,"WARNING: hopsize may be too small, recommend at least poleCount * 5\n"));

        if ((WINDIN = slice * 2) > MAXWINDIN)
          quit(Str(X_915,"input framesize (inter-frame-offset*2) exceeds maximum allowed"));
        if ((input_dur < 0) || (beg_time < 0))
            quit(Str(X_912,"input and begin times cannot be less than zero"));

        if (verbose) {
            err_printf(Str(X_440,"Reading sound from %s, writing lpfile to %s\n"),
                    infilnam, outfilnam);
            err_printf(Str(X_1133,"poles=%d hopsize=%d begin=%4.1f duration=%4.1f\n"),
                    poleCount, slice, beg_time, input_dur);
            err_printf(Str(X_972,"lpheader comment:\n%s\n"), lph->text);
            if (pchlow > 0.)
              err_printf(Str(X_1122,"pch track range: %5.1f - %5.1f Hz\n"),pchlow,pchhigh);
            else err_printf(Str(X_1125,"pitch tracking inhibited\n"));
        }
        if ((input_dur < 0) || (beg_time < 0))
            quit(Str(X_912,"input and begin times cannot be less than zero"));

        if (storePoles)
          err_printf(Str(X_522,"Using pole storage method\n"));
        else
          err_printf(Str(X_521,"Using filter coefficient storage method\n"));

   /* Get information on input sound */
        if (
         (infd = SAsndgetset(infilnam,&p,&beg_time,&input_dur,&sr,channel))<0) {
            sprintf(errmsg,Str(X_735,"error while opening %s"), retfilnam);
            quit(errmsg);
        }

   /* Try to open output file */

        if ((ofd = openout(outfilnam, 1)) < 0)  /* open output file */
            quit(Str(X_632,"cannot create output file"));

   /* Prepare header */

        if (storePoles)
          lph->lpmagic = LP_MAGIC2;
        else
          lph->lpmagic = LP_MAGIC;

        lph->npoles = poleCount;
        lph->nvals = poleCount*(storePoles?2:1) + NDATA;
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
            quit(Str(X_630,"cannot write header"));

/* get buffer size for one analysis frame: filtercoef or poles + freq/rms/... */
        osiz = (poleCount*(storePoles?2:1) + NDATA) * sizeof(MYFLT);

   /* Allocate signal buffer for sound frame */
        sigbuf = (MYFLT *) mmalloc((long)WINDIN * sizeof(MYFLT));
        sigbuf2 = sigbuf + slice;

   /* Try to read first frame in buffer */
        if ((n = getsndin(infd, sigbuf, (long)WINDIN, p)) < WINDIN)
            quit(Str(X_1202,"soundfile read error, could not fill first frame"));

   /* initialize frame pitch table ? */
        if (doPitch)
            ptable(pchlow, pchhigh, (MYFLT) p->sr, WINDIN);

   /* Initialise for analysis */
        counter = 0;
        analframes = (p->getframes - 1) / slice;

   /* Some display stuff */
        dispinit();
        dispset(&pwindow,coef+4,poleCount,"pitch: 0000.00   ",0,"LPC/POLES");
        a = (double (*)[MAXPOLES]) malloc(MAXPOLES * MAXPOLES * sizeof(double));  /* Space for a array */
        x = (double *) malloc(WINDIN * sizeof(double));  /* alloc a double array */
#ifdef TRACE
        trace = fopen("lpanal.trace", "w");
#endif
   /* Do the analysis */
        do {
            MYFLT *fp1;
            double *dfp;

       /* Analyze current frame */
#ifdef TRACE_POLES
            printf ("Starting new frame...\n");
#endif
            counter++;
            alpol(sigbuf, &errn, &rms1, &rms2, filterCoef);
       /* Transfer results */
            coef[0] = (MYFLT)rms2;
            coef[1] = (MYFLT)rms1;
            coef[2] = (MYFLT)errn;
            if (doPitch)
                coef[3] = getpch(sigbuf);
            else coef[3] = FL(0.0);
            if (debug) err_printf("%d\t%9.4f\t%9.4f\t%9.4f\t%9.4f\n",
                               counter, coef[0], coef[1], coef[2], coef[3]);
#ifdef TRACE
            if (debug) fprintf(trace,"%d\t%9.4f\t%9.4f\t%9.4f\t%9.4f\n",
                               counter, coef[0], coef[1], coef[2], coef[3]);
#endif
/*            for (fp1=coef+NDATA, dfp=cc+poleCount, n=poleCount; n--; ) */
/*                *fp1++ = - (MYFLT) *--dfp; */              /* rev coefs & chng sgn */
            sprintf(pwindow.caption, "pitch: %8.2f", coef[3]);
            display(&pwindow);

                /* Prepare buffer for output */

            if (storePoles) {
                          /* Treat (swap) filter coefs for resolution */
              filterCoef[poleCount] = 1.0;
              for (i=0; i<(poleCount+1)/2; i++) {
                j = poleCount-1-i;
                z1 = filterCoef[i];
                filterCoef[i] = filterCoef[j];
                filterCoef[j] = z1;
              }

                           /* Get the Filter Poles */

              polyzero(100,poleCount,filterCoef,polePart1,polePart2,
                       &poleFound,2000,&indic,workArray1);

              if (poleFound!=poleCount) {
                err_printf(Str(X_283,"Found only %d poles...sorry\n"), poleFound);
#if !defined(mills_macintosh) && !defined(CWIN)
                exit(-1);
#else
                return(-1);
#endif
              }
              InvertPoles(poleCount,polePart1,polePart2);

#ifdef TRACE_POLES
              DumpPoles(poleCount,polePart1,polePart2,0,"Extracted Poles");
#endif

#ifdef _DEBUG
                          /* Resynthetize the filter for check */
              InvertPoles(poleCount,polePart1,polePart2);

              synthetize(poleCount,polePart1,polePart2,polyReal,polyImag);

              for (i=0; i<poleCount; i++) {
#ifdef TRACE_FILTER
                printf("filterCoef: %f\n", filterCoef[i]);
#endif
                if (filterCoef[i]-polyReal[poleCount-i]>1e-10)
                  err_printf(Str(X_254,"Error in coef %d : %f <> %f \n"),
                             i, filterCoef[i], polyReal[poleCount-i]);
              }
              err_printf(".");
              InvertPoles(poleCount,polePart1,polePart2);
#endif
                          /* Switch to pole magnitude and phase */

              for (i=0; i<poleCount;i++) {
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

  /*          DumpPoles(poleCount,polePart1,polePart2,1,"About to store"); */

                        /* Store in output buffer */
              fp1 = coef+NDATA;
              for (i=0; i<poleCount;i++) {
                *fp1++ = (MYFLT)polePart1[i];
                *fp1++ = (MYFLT)polePart2[i];
              }
            }
            else {
              /* Move filter data into output buffer */
              dfp = filterCoef+poleCount;
              fp1 = coef+NDATA;
              for (n=0;n<poleCount; n++)
                *fp1++ = - (MYFLT) *--dfp;
            }

        /* Write frame to disk */
            if ((nb = write(ofd, (char *)coef, osiz)) != osiz)
                quit(Str(X_1381,"write error"));
            memcpy(sigbuf, sigbuf2, sizeof(MYFLT)*slice);

        /* Some unused stuff. I think from when all snd was in mem */
/*  ( MYFLT *fp2; for (fp1=sigbuf, fp2=sigbuf2, n=slice; n--; ) */ /* move slice forward */
/*              *fp1++ = *fp2++;} */

        /* Get next sound frame */
            if ((n = getsndin(infd, sigbuf2, (long)slice, p)) == 0)
                break;          /* refil til EOF */

        } while (POLL_EVENTS() && counter < analframes); /* or nsmps done */

   /* clean up stuff */
        dispexit();
        printf(Str(X_35,"%d lpc frames written to %s\n"), counter, outfilnam);
        close(infd);
        close(ofd);
        free(a); free(x);
        mfree(coef);
#if !defined(mills_macintosh) && !defined(CWIN)
        exit(0);
#endif
        return (-1);            /* To quieten compilers */
}

static void quit(char *msg)
{
        printf("lpanal: %s\n", msg);
        die(Str(X_597,"analysis aborted"));
}

static void lpdieu(char *msg)
{
        printf("lpanal: %s\n", msg);
        usage();
        exit(0);
}

/*
 *
 *  This is where the frame analysis is done
 *
 */

static void alpol(MYFLT *sig, double *errn,
                  double *rms1, double *rms2, double b[MAXPOLES])
                                        /* sig now MYFLT */
                                        /* b filled here */
{
    double v[MAXPOLES];
    double *xp;
    double sum, sumx, sumy;
    int i, j, k, limit;

   /* Transfer signal in x array */
    for (xp=x; xp-x < WINDIN;++xp,++sig)/*   &  copy sigin into */
      *xp = (double) *sig;

   /* Build system to be solved */
    for (i=0; i < poleCount;++i)  {
      sum = (double) 0.0;
      for (k=poleCount; k < WINDIN;++k)
        sum += x[k-(i+1)] * x[k];
      v[i] = -sum;
      if (i != poleCount - 1)  {
        limit = poleCount - (i+1);
        for (j=0; j < limit; j++)  {
          sum += x[poleCount-(i+1)-(j+1)] * x[poleCount-(j+1)]
            - x[WINDIN-(i+1)-(j+1)] * x[WINDIN-(j+1)];
          a[(i+1)+j][j] = a[j][(i+1)+j] = sum;
        }
      }
    }
    sum = (double) 0.0;
    for (k=poleCount; k < WINDIN;++k)
      sum +=  x[k]*x[k];
    sumy = sumx = sum;
#ifdef TRACE
    fprintf(trace, "sumy=%f\n", sumy);
#endif
    for (j=0; j < poleCount; j++)  {
      sum += x[poleCount-(j+1)]*x[poleCount-(j+1)] -
        x[WINDIN-(j+1)]*x[WINDIN-(j+1)];
      a[j][j] = sum;
    }

    /* Solves the system */
    gauss(a, v, b);

    /* Compute associted parameters */
    for (i=0; i < poleCount;++i) {
      sumy -= b[i]*v[i];
#ifdef TRACE
      fprintf(trace, "b[i]=%f v[i]=%f sumy=%f\n", b[i], v[i], sumy);
#endif
    }
    *rms1 = sqrt(sumx / (WINDIN - poleCount) );
    *rms2 = sqrt(sumy / (WINDIN - poleCount) );
    /*        sum = (*rms2)/(*rms1);  *errn = sum*sum; */
    *errn = sumy/sumx;
}

/*
 *
 * Perform gauss elemination: Could be replaced by something more robust
 *
 */
static void gauss(double (*a/*old*/)[MAXPOLES], double *bold, double b[])
{
    double amax, dum, pivot;
    double c[MAXPOLES];
    int i, j, k, l, istar=-1, ii, lp;

    /* bold untouched by this subroutine */
    for (i=0; i < poleCount;++i)  {
      c[i] = bold[i];
#ifdef TRACE
      fprintf(trace, "c[%d]=%f\n", i, c[i]);
#endif
    }
    /* eliminate i-th unknown */
    for (i=0; i < poleCount - 1;++i)  {        /* find largest pivot */
      amax = 0.0;
      for (ii=i; ii < poleCount;++ii)  {
        double npq = fabs(a[ii][i]);
        if (npq >= amax)  {
          istar = ii;
          amax = npq;
        }
      }
      if (amax < 1e-20) {
        printf("Row %d or %d have maximum of %g\n", i, poleCount, amax);
        die(Str(X_791,"gauss: ill-conditioned"));
      }
      if (i != istar) {
        for (j=0; j < poleCount;++j)  {    /* switch rows */
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
      for (j=i+1; j < poleCount;++j)  {             /* pivot */
        pivot = a[j][i] / amax;
#ifdef TRACE
        fprintf(trace, "pivot = %f c[%d] = %f\n", pivot, j, c[j]);
#endif
        c[j] = c[j] - pivot * c[i];
#ifdef TRACE
        fprintf(trace, "c[%d]=%f c[%d]=%f\n", j, c[j], i, c[i]);
#endif
        for (k=0; k < poleCount; ++k)
          a[j][k] = a[j][k] - pivot * a[i][k];
      }
    }                               /* return if last pivot is too small */
    if (fabs(a[poleCount-1][poleCount-1]) < 1e-20) {
      printf("Row %d or %d have maximum of %g\n",
             poleCount-1, poleCount, fabs(a[poleCount-1][poleCount-1]));
      die(Str(X_791,"gauss: ill-conditioned"));
    }

    b[poleCount-1] = c[poleCount-1] / a[poleCount-1][poleCount-1];
#ifdef TRACE
    fprintf(trace, "b[poleCount-1]=%f c[poleCount-1]=%f a[][]=%f\n",
            b[poleCount-1], c[poleCount-1], a[poleCount-1][poleCount-1]);
#endif
    for (k=0; k<poleCount-1; ++k)  {   /* back substitute */
      l = poleCount-1 -(k+1);
      b[l] = c[l];
      lp = l + 1;
      for (j = lp; j<poleCount; ++j) {
        b[l] -= a[l][j] * b[j];
      }
      b[l] /= a[l][l];
    }
}

static void usage(void)
{
    err_printf(Str(X_500,"USAGE:\tlpanal [flags] infilename outfilename\n"));
    err_printf(Str(X_580,"\twhere flag options are:\n"));
    err_printf(Str(X_163,
               "-s<srate>\tinput sample rate (defaults to header else %7.1f.)\n"),
               DFLT_SR);
    err_printf(Str(X_140,
                   "-c<chnlreq>\trequested channel of sound (default chan 1)\n"));
    err_printf(Str(X_139,
               "-b<begin>\tbegin time in seconds into soundfile (default 0.0)\n"));
    err_printf(Str(X_142,
         "-d<duration>\tseconds of sound to be analysed (default: to EOF)\n"));
    err_printf(Str(X_158,
                   "-p<npoles>\tnumber of poles for analysis (default %d)\n"),
               DEFpoleCount);
    err_printf(Str(X_148,
                   "-h<hopsize>\toffset between frames in samples (default %d)\n"),
               DEFSLICE);
    err_printf(Str(X_571,"\t\t\t(framesize will be twice <hopsize>)\n"));
    err_printf(Str(X_101,
                   "-C<string>\tcomment field of lp header (default empty)\n"));
    err_printf(Str(X_122,
               "-P<mincps>\tlower limit for pitch search (default %5.1f Hz)\n"),
               PITCHMIN);
    err_printf(Str(X_570,"\t\t\t(-P0 inhibits pitch tracking)\n"));
    err_printf(Str(X_124,
               "-Q<maxcps>\tupper limit for pitch search (default %5.1f Hz)\n"),
               PITCHMAX);
    err_printf(Str(X_167,
               "-v<verblevel>\tprinting verbosity: 0=none, 1=verbose, 2=debug."));
    err_printf(Str(X_14," (default 0)\n"));
    err_printf(Str(X_146,"-g\tgraphical display of results\n"));
    err_printf(Str(X_135,"-a\t\talternate (pole) file storage\n"));
    err_printf(Str(X_91,"-- fname\tLog output to file\n"));
    err_printf(Str(X_1182,"see also:  Csound Manual Appendix\n"));
}
