/*
    XXX code for

    Copyright (C) 1995 Fabio P. Bertolotti

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

#include "cs.h"                 /*                      WINEPS.C        */
#include "cwindow.h"
#include <math.h>

/*--------------------------------------  winEPS.c ---------------------------
 *
 *  Write Csound's graphics in PostScript format into a file for later viewing.
 *
 *
 *  Fabio P. Bertolotti,
 *  Dec 1995,
 *  Rosdorf, Germany.
 *  email: fabio@ts.go.dlr.de
 *
 *
 *
 *  The PS output file is located in the same directory as the
 *  sound file, and has the name of the sound file with the
 *  extension ``.eps'' appended.
 *
 *  On NeXT running Csnd, the sound file has the extension
 *  ``.snd''.  In this case, the PS file has the name of the
 *  sound file, with the extension ``.eps'' substituting the
 *  ``.snd'' extension.  Such extension replacement also occurs
 *  for output files ending in ``.aiff'', ``.au'' and ``.wav''
 *
 *  If sound output is being piped directly to the DAC, then
 *  there is no PS output.
 *
 *
 *  Each new graph is plotted on a fresh PostScript page
 *  On each page we print the current date, the name of the
 *  score file, the orchestra file, the maximum and minimum values
 *  and the figure caption, as defined by Csound.
 *  Both the vertical and horizontal axes are labeled (unlike
 *  SGI and X11 window graphs, which have no labels).
 *
 *
 *  If you improve this subroutine or the PostScript ``look'' of the
 *  graphs, I would be grateful if you email me the improved version,
 *  at:  fabio@ts.go.dlr.de      Thanks.
 *
 *
  --------------------------------------------------------------------------*/

#ifdef HAVE_STRING_H
# include <string.h>
#elif HAVE_STRINGS_H
# include <strings.h>
#endif
#include <time.h>

          /*
           *   Origin and size of PostScript plot
           *
           */
#define MyPS_XORIG     (FL(100.0))
#define MyPS_YORIG     (FL(130.0))
#define MyPS_WIDTH     (FL(450.0))
#define MyPS_HEIGHT    (FL(400.0))
#define MyPS_FONT      "/Times-Roman"
#define MyPS_FONTSIZE  (FL(20.0))

extern OPARMS O;                         /* Get sound output file name       */

static int   winPSinitialized = 0;
static int   psFileOk = 0;               /* Flag for ``psFile opened O.K.''  */
static FILE *psFile;
static char  ps_date[40];                /* Print time & date on every plot  */
static int   currentPage = 0;            /* Current page number              */

void PS_MakeGraph(WINDAT *wdptr, char *name)
{
    char      *filenam;
    char      *pathnam_;
    char      pathnam[1024];
    char      *date, *t;
    struct tm *date_ptr;
    time_t    lt;

    if (!winPSinitialized) {
      winPSinitialized++;
      filenam = O.outfilename;
      if (filenam == NULL)
        filenam = "test"; /* O.outfilename not set yet */

      /*  If sound output is being piped directly to the DAC, then */
      /*  there is no PS output, (psFileOK remains 0),             */
      /*  otherwise open an encapsulated PostScript output file    */
      /*  with a name related to the sound file's name.            */
      /*                                                           */
      /*  The PS file is located in the same directory as the      */
      /*  sound file, and has the name of the sound file with the  */
      /*  extension ``.eps'' appended.                             */
      /*                                                           */

      /*
       *  Remove extension from sound-file and add ".eps"
       */
      strcpy(pathnam, filenam);
      t = strrchr(pathnam, '.');
      if (t != NULL) *t = '\0';
      strcat(pathnam, ".eps");
      pathnam_ = csoundFindOutputFile(&cenviron, pathnam, "SFDIR");
      psFile = NULL;
      if (pathnam_ != NULL) {
        psFile = fopen(pathnam_, "w");
        mfree(&cenviron, pathnam_);
      }
      if (psFile == NULL)
        printf(Str("** Warning **  PostScript file %s cannot be opened \n"),
               pathnam);
      else {
#ifdef __MACH__
        /* No idea why these are not declared */
        extern struct tm* localtime(const time_t*);
        extern char* asctime(const struct tm*);
#endif
        printf(Str("\n PostScript graphs written to file %s \n \n"), pathnam);
        psFileOk = 1;
        /*
         *  Get the current time and date
         */
        lt = time('\0');
        date_ptr = localtime(&lt);
        date = asctime(date_ptr);
        t = ps_date;
        while (*date != '\n')
          *t++ = *date++;
        *t = '\0';
        /*
         *  Print PostScript file Header
         *  Place every plot on a new page.
         */
        fprintf(psFile,"%s \n","%!PS-Adobe-2.0");
        fprintf(psFile,"%s \n","%%Creator: Csound");
        fprintf(psFile,"%s %s \n","%%CreationDate:",ps_date);
        fprintf(psFile,"%s \n","%%Pages: (atend)");
        fprintf(psFile,"%s \n","%%PageOrder: Ascend");
        fprintf(psFile,"%s \n","%%BoundingBox: 010 010 540 700");
        fprintf(psFile,"%s \n","%%Orientation: Portrait");
        fprintf(psFile,"%s \n","%%EndComments");
        fprintf(psFile,"%s \n","   ");
      }
    }
}

static void
setAxisNumbers(MYFLT *min, MYFLT *max, char *cmin, char *cmax)
{
    double bmin, bmax, big;
    int    i;

        /*
         *  Get most significant digit
         */

    bmin = 0.0000001;
    if (fabs(*min)>bmin) {
      while ( (int) (fabs(*min)/bmin) ) bmin = bmin * 10.0;
    }

    bmax = 0.0000001;
    if (fabs(*max)>bmax) {
      while ( (i = (int)(fabs(*max)/bmax)) ) bmax = bmax * 10.0;
    }
    if (fabs(bmin) > fabs(bmax)) big = fabs(bmin);
    else big = fabs(bmax);

    /*
     *  Set max and minimm to nearest 2nd sig digit
     */

    if (*max == FL(0.0)) i = 0;
    else              i = (int)((*max/big)*100.0) + 1;
    *max = (MYFLT)(i*big/100.0);

    if (*min == FL(0.0)) i = 0;
    else           i = (int)((*min/big)*100.0) - 1;
    *min = (MYFLT)(i*big/100.0);

    if (fabs(*max - *min) < 0.0000001)
      *max = *min + 1.0f;                       /* No zero divide */

    /*
     *  Write characters
     */

    sprintf(cmin,"%g",*min);
    sprintf(cmax,"%g",*max);
}

static void
PS_drawAxes(char *cxmin, char *cxmax, char *cymin, char *cymax)
{
    MYFLT xx, yy, dx, dy;
    MYFLT fnts, swide;
    int i;

    /*
     * Make axes - box
     */
    fprintf(psFile,"          \n");
    fprintf(psFile,"%%   Axes  \n");
    fprintf(psFile,"1 setlinewidth \n");
    fprintf(psFile,"newpath   \n");
    xx = MyPS_XORIG ;
    yy = MyPS_YORIG ;
    fprintf(psFile,"%f  %f  moveto \n",xx, yy);

    xx = MyPS_XORIG + MyPS_WIDTH;
    fprintf(psFile,"%f  %f  lineto \n",xx, yy);

    yy = MyPS_YORIG + MyPS_HEIGHT;
    fprintf(psFile,"%f  %f  lineto \n",xx, yy);

    xx = MyPS_XORIG ;
    fprintf(psFile,"%f  %f  lineto \n",xx, yy);
    fprintf(psFile,"closepath stroke \n");

    /*
     * Make tick marks:   x-axis
     */
    fprintf(psFile,"                \n");
    fprintf(psFile,"%%   x-tickmarks \n");
    fprintf(psFile,"1 setlinewidth  \n");
    xx = MyPS_XORIG;
    dx = MyPS_WIDTH / FL(10.0);
    dy = MyPS_HEIGHT/ FL(10.0);
    for (i = 0; i <= 10; i++) {
      yy = MyPS_YORIG ;
      fprintf(psFile,"%f  %f  moveto \n",xx, yy);
      yy = MyPS_YORIG + dy/FL(6.0) ;
      fprintf(psFile,"%f  %f  lineto stroke \n",xx, yy);
      yy = MyPS_YORIG + MyPS_HEIGHT - dy/FL(6.0) ;
      fprintf(psFile,"%f  %f  moveto \n",xx, yy);
      yy = MyPS_YORIG + MyPS_HEIGHT;
      fprintf(psFile,"%f  %f  lineto stroke \n",xx, yy);
      xx += dx;
    }

    /*
     * Make tick marks:   y-axis
     */
    fprintf(psFile,"                \n");
    fprintf(psFile,"%%   y-tickmarks \n");
    fprintf(psFile,"1 setlinewidth  \n");
    yy = MyPS_YORIG;
    for (i = 0; i <= 10; i++) {
      xx = MyPS_XORIG ;
      fprintf(psFile,"%f  %f  moveto \n",xx, yy);
      xx = MyPS_XORIG + dx/FL(6.0) ;
      fprintf(psFile,"%f  %f  lineto stroke \n",xx, yy);
      xx = MyPS_XORIG + MyPS_WIDTH - dx/FL(6.0);
      fprintf(psFile,"%f  %f  moveto \n",xx, yy);
      xx = MyPS_XORIG + MyPS_WIDTH;
      fprintf(psFile,"%f  %f  lineto stroke \n",xx, yy);
      yy += dy;
    }

    /*
     * Label the axes's max and min
     */
    fnts = MyPS_FONTSIZE;
    fprintf(psFile,"                \n");
    fprintf(psFile,"%s findfont %f scalefont setfont \n",
            MyPS_FONT, fnts);

    swide = FL(0.5)*fnts*(MYFLT)strlen(cxmin);
    xx    = MyPS_XORIG - swide * FL(0.5);
    yy    = MyPS_YORIG - fnts * FL(1.8);
    fprintf(psFile,"%f %f moveto \n",xx, yy);
    fprintf(psFile,"(%s) show \n", cxmin);

    swide = FL(0.5)*fnts*(MYFLT)strlen(cxmax);
    xx    = MyPS_XORIG + MyPS_WIDTH - swide * FL(0.2);
    yy    = MyPS_YORIG - fnts * FL(1.8);
    fprintf(psFile,"%f %f moveto \n",xx, yy);
    fprintf(psFile,"(%s) show \n", cxmax);

    swide = FL(0.5)*fnts*strlen(cymin);
    xx    = MyPS_XORIG - fnts * FL(0.5) - swide;
    yy    = MyPS_YORIG - fnts * FL(0.4);
    fprintf(psFile,"%f %f moveto \n",xx, yy);
    fprintf(psFile,"(%s) show \n", cymin);

    swide = FL(0.5)*fnts*(MYFLT)strlen(cymax);
    xx    = MyPS_XORIG - fnts * FL(0.5) - swide;
    yy    = MyPS_YORIG + MyPS_HEIGHT - fnts * FL(0.4);
    fprintf(psFile,"%f %f moveto \n",xx, yy);
    fprintf(psFile,"(%s) show \n", cymax);
}


void PS_DrawGraph(WINDAT *wdptr)
{
    int   iskip = (wdptr->npts < MyPS_WIDTH) ? 1 :
                  (int)(wdptr->npts / MyPS_WIDTH);
    MYFLT xmin, xmax, ymin, ymax, xx, yy, dx, dy, fnts;
    char  cxmin[20], cxmax[20], cymin[20], cymax[20];
    int   i;

    /*
     *  No action when the output file is not opened
     */

    if (psFileOk) {
        /*
         *  draw current page and start a newpage.
         */
      currentPage++;
      if (currentPage>1) fprintf(psFile,"showpage  \n");
      fprintf(psFile,"  \n");
      fprintf(psFile,"%%%%Page: %d %d \n", currentPage, currentPage);
      fprintf(psFile,"  \n");

        /*
         *  Get labels for axis limits, then draw the axis
         */
      xmin = FL(0.0);
      xmax = FL(1.0)*wdptr->npts;
      sprintf(cxmin,"%d",0);
      sprintf(cxmax,"%ld",wdptr->npts);

      ymin = wdptr->min;
      ymax = wdptr->max;
      setAxisNumbers(&ymin, &ymax, cymin, cymax);

      PS_drawAxes(cxmin, cxmax, cymin, cymax);

        /*
         *  write the plot caption
         */
      fnts = MyPS_FONTSIZE;
      fprintf(psFile,"                \n");
      fprintf(psFile,"%s findfont %f scalefont setfont \n",
                     MyPS_FONT, fnts);
      xx = MyPS_XORIG;
      yy = MyPS_YORIG + MyPS_HEIGHT + FL(7.0)*fnts*FL(1.5);
      fprintf(psFile,"%f  %f  moveto \n", xx, yy);
      fprintf(psFile,"(date: %s ) show \n", ps_date);
      yy = MyPS_YORIG + MyPS_HEIGHT + FL(6.0)*fnts*FL(1.5);
      fprintf(psFile,"%f  %f  moveto \n", xx, yy);
      fprintf(psFile,"(scorefile: %s) show \n", scorename);
      yy = MyPS_YORIG + MyPS_HEIGHT + FL(5.0)*fnts*FL(1.5);
      fprintf(psFile,"%f  %f  moveto \n", xx, yy);
      fprintf(psFile,"(orch_file: %s) show \n", orchname);
      yy = MyPS_YORIG + MyPS_HEIGHT + FL(4.0)*fnts*FL(1.5);
      fprintf(psFile,"%f  %f  moveto \n", xx, yy);
      fprintf(psFile,"(maximum  : %f) show \n", wdptr->max);
      yy = MyPS_YORIG + MyPS_HEIGHT + FL(3.0)*fnts*FL(1.5);
      fprintf(psFile,"%f  %f  moveto \n", xx, yy);
      fprintf(psFile,"(minimum  : %f) show \n", wdptr->min);

      if (wdptr->caption) {
        xx = MyPS_XORIG + MyPS_WIDTH/FL(3.0);
        yy = MyPS_YORIG + MyPS_HEIGHT + fnts;
        fprintf(psFile,"%f  %f  moveto \n0", xx, yy);
        fprintf(psFile,"(%s) show \n", wdptr->caption);
      }

        /*
         * Draw 0 line if inside box
         */
      dy = MyPS_HEIGHT/(ymax - ymin);
      if (ymin<0 && ymax > 0) {
        fprintf(psFile,"  \n");
        fprintf(psFile,"%%   0-line \n");
        yy = MyPS_YORIG + (0 - ymin)*dy;
        dx = (FL(1.0)*MyPS_WIDTH)/FL(100.0);
        for (i=0; i<100; i+=3) {
          xx = MyPS_XORIG + i*dx;
          fprintf(psFile,"%f  %f  moveto \n", xx, yy);
          xx = MyPS_XORIG + (i+1)*dx;
          fprintf(psFile,"%f  %f  lineto stroke \n", xx, yy);
        }
      }

      /*
       *  write the plot data
       */
      fprintf(psFile,"               \n");
      fprintf(psFile,"%% Plot data    \n");
      fprintf(psFile,"1 setlinewidth \n");

      dx = iskip * MyPS_WIDTH/((MYFLT) wdptr->npts);
      xx = MyPS_XORIG;
      yy = MyPS_YORIG + (wdptr->fdata[0] - ymin)*dy;
      fprintf(psFile,"newpath %f  %f  moveto \n", xx, yy);
      for (i = 1; i < wdptr->npts; i+=iskip) {
        xx += dx;
        yy  = MyPS_YORIG + (wdptr->fdata[i] - ymin)*dy;
        fprintf(psFile,"%f  %f  lineto \n", xx, yy);
      }
      fprintf(psFile,"stroke \n");
    }
}


int PS_ExitGraph(void)
{
    /*
     *  No action when the output file is not opened
     */
    if (psFileOk) {
        fprintf(psFile,"         \n");
        fprintf(psFile,"showpage \n");
        fprintf(psFile,"         \n");
        fprintf(psFile,"%%%%Trailer \n");
        fprintf(psFile,"%%%%Pages: %d  \n", currentPage);
        fprintf(psFile,"%%%%EOF\n");
    }
    return 0;
}

