/*
    winascii.c: graphs in ascii text

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csoundCore.h"                         /*      winascii.c           */
#include "cwindow.h"                            /*  teletype csound graphs   */

#define HOR     80
#define VER     20
#define YOFF    10
#define YOFF4   40

void MakeAscii(CSOUND *csound, WINDAT *wdptr, const char *n)
{
    IGN(n);
    IGN(csound);
    wdptr->windid = ~((uintptr_t) 0);           /* just so it's not null */
}

void KillAscii(CSOUND *csound, WINDAT *wdptr)
{
    IGN(csound);
    wdptr->windid = 0;          /* just to make out that it's dead */
}

/* display an n-pnt float array using simple ascii chars */

static CS_NOINLINE void DrawAscii_(CSOUND *csound, WINDAT *wdptr, char *points)
{
    long    npts    = wdptr->npts;
    MYFLT   absmax  = wdptr->absmax;
    char    *s;
    MYFLT   *fp = wdptr->fdata, *fplim = fp + npts;
    int     n, vscale4, vpos, vmin = VER, vmax = 0, incr;
    MYFLT   scalefactor;

    scalefactor = YOFF4 / absmax;                   /*   get normalizing */
    incr = (npts-1)/HOR + 1;                        /*   & sampling facs */
    for (s = points + (YOFF * HOR), n = 0; fp < fplim; n++, fp += incr) {
      s[n] = '_';                                   /* now write x-axis  */
      vscale4 = (int) (*fp * scalefactor + YOFF4);
      vpos = vscale4 >> 2;  /* and sampled pnts (with 1/4 line resolution) */
      if ((unsigned int) vpos > (unsigned int) VER)
        continue;
      if (vpos < vmin)  vmin = vpos;
      if (vpos > vmax)  vmax = vpos;
      points[vpos * HOR + n] = "_.-'"[vscale4 & 3]; /* into dsplay array */
    }
    for (vpos = vmax; vpos >= vmin; vpos--) {       /* for all lines:    */
      s = points + (vpos * HOR);
      for (n = (HOR - 1); n >= 0 && s[n] == ' '; n--)
        ;                                           /*  find last char & */
      csoundMessage(csound, "%.*s\n", n + 1, s);    /*  putline to there */
    }
}

void DrawAscii(CSOUND *csound, WINDAT *wdptr)
{
    csoundMessage(csound, Str("%s\t%ld points, scalemax %5.3f\n"),
                  wdptr->caption, (long) wdptr->npts, wdptr->absmax);
    if (wdptr->absmax) {                              /* for non-triv fn:   */
      char    points[(VER + 1) * HOR];            /* alloc the 2-Dim array  */
      memset(&(points[0]), ' ', ((VER + 1) * HOR));   /* blank out all pts  */
      DrawAscii_(csound, wdptr, &(points[0]));
    }
}
