/*
    winX11.c: graphs in X11

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "cs.h"
#include <X11/X.h>                              /*    winX11.c          */
#include <X11/Xlib.h>                           /* Csound Xwin graphs   */
                                                /*   dpwe,15may90       */
#include <stdio.h>
#ifndef MYFLT
#include "sysdep.h"
#endif
#include "cwindow.h"
#include "text.h"

#define XINIT    10      /* set default window location */
#define YINIT    50
#define WIDTH    450     /* start off small - user resizes */
#define HEIGHT   120
#define BDR      2
#define MSGX     16      /* offset for message in window */
#define MSGY     16
#define TXHGHT   14      /* baseline offset for text */

#define MAXLSEGS 2048    /* X can only deal with so many linesegs .. */

#define MAXXPTS  MAXLSEGS /* maximum size of XPoint array */

#define NUMOFWINDOWS (20)

static Display *xdisp = NULL;
static int     xscrn;
static GC      xdfgc;
static GContext     xgcon;
static Window       xlwin = (Window)NULL;
static Font         xfont, xbfont;
static XFontStruct  *xfsp, *xbfsp;
static XPoint       xpts[MAXXPTS];

static int winstance = 0; /* counts windows to offset across screen */
#define STACKDEPTH 4      /* number of windows down to go */

/* static function prototypes */
static void myXprintLines(Window,char *,int,int,int *,int *);
static void myXwinWait(Window,char *);
static int  myXwait(char *);
static void myXWaitForExpose(Window);
static Window MakeWindow(int, char *);    /* 0 => graph, 1 => xyinp */
static void KillWin(Window *);

static void myXprintLines( /* prints str with embdd '\n's */
    Window win,            /*           on multiple lines */
    char   *msg,           /* returns overall width & depth of area printed */
    int    x,
    int    y,
    int    *pw,
    int    *ph)
{
    int    lin = 0, w, slen;
    char   *s1,*s2;
    XTextItem    txitem[1];

    *pw = 0;   *ph = 0;
    txitem[0].delta = 1;
    txitem[0].font  = xbfont;
    txitem[0].chars = errmsg;
    s1 = msg;
    do {
      if ((s2 = strchr(s1, '\n')) != NULL)
        slen = s2 - s1 - 1;
      else slen = strlen(s1);
      strncpy(errmsg, s1, slen);
      errmsg[slen] = '\0';
      txitem[0].nchars = slen;
      XDrawText(xdisp,win,xdfgc,MSGX,MSGY+lin*TXHGHT,txitem,1);
      if ( (w=XTextWidth(xbfsp, s1, slen)) > *pw)
        *pw = w;
      *ph += TXHGHT;
      s1 = s2 + 1;
      lin++;
    } while (s2 != NULL);
}

static void myXwinWait(Window win, char   *msg)
{
    XEvent       event;
    int          msgW,msgH;

    myXprintLines(win,msg,MSGX,MSGY, &msgW, &msgH);
    do {
      XWindowEvent(xdisp, win,
                   ButtonPressMask|ExposureMask|ButtonMotionMask, &event);
      if (event.type == Expose)
        myXprintLines(win,msg,MSGX,MSGY, &msgW, &msgH);
    } while (!(event.type == ButtonPress));
    do {
      XWindowEvent(xdisp, win,
                   ButtonReleaseMask|ExposureMask|ButtonMotionMask, &event);
      if (event.type == Expose)
        myXprintLines(win,msg,MSGX,MSGY, &msgW, &msgH);
    } while (!(event.type == ButtonRelease));
    XClearArea(xdisp,win,MSGX,MSGY-TXHGHT+4,msgW,msgH,0);
    XFlush(xdisp);                    /* make sure wdw is cleared NOW */
}

static int myXwait(char *msg)
{
    if (xlwin != (Window)NULL) {
      myXwinWait(xlwin, msg);
      return(0);
    }
    else return(1);
}

static void myXWaitForExpose(Window win)    /* wait until it's ready to draw */
{
    XEvent    event;

    while ( (XCheckWindowEvent(xdisp, win, ExposureMask, &event)));
    while (!(XCheckWindowEvent(xdisp, win, ExposureMask, &event)));
}

int Graphable_(void)     /* called during program initialisation */
{               /* decides whether to use X or not; initializes X if so */
    int         rc = 0;         /* default : don't use X, use tty ascii */

    if (xdisp != NULL)  return 1;               /* already open - signal ok  */
    if ((xdisp = XOpenDisplay(NULL)) != NULL) { /* if this OK, X is possible */
      xscrn = XDefaultScreen(xdisp);
      xdfgc = XDefaultGC(xdisp, xscrn);
      xgcon = XGContextFromGC(xdfgc);
      rc = 1;                                 /*       so set return code  */
    }
    return(rc);
}

static Window MakeWindow(int type /* 0 => graph, 1 => xyinp */, char *name)
{
    char   *fontname;
    Window xwin;
    XSetWindowAttributes watts;
    unsigned long        wmask;

    watts.background_pixel = XWhitePixel(xdisp, xscrn);
    watts.backing_store = WhenMapped;
    watts.event_mask    = ExposureMask | ButtonPressMask | ButtonReleaseMask;
    if (type == 1)      /* input windows need more events */
        watts.event_mask |= ButtonMotionMask;
    wmask = CWBackingStore | CWEventMask | CWBackPixel;
    xwin = XCreateWindow(xdisp, XDefaultRootWindow(xdisp),
                         XINIT + (winstance / STACKDEPTH)*(int)(1.2*WIDTH),
                         YINIT + (winstance % STACKDEPTH)*(int)(1.2*HEIGHT),
                         WIDTH, HEIGHT, BDR,
                         CopyFromParent, InputOutput, CopyFromParent,
                         wmask, &watts);

    ++winstance;
    XStoreName(xdisp, xwin, name);
    XMapWindow(xdisp, xwin);   /* map the window to the screen */
    myXWaitForExpose(xwin);    /* wait until it's ready to draw */
    xlwin = xwin;                   /* keep track of latest window for msgs */
    fontname = "6x10";                             /* set up font info... */
    if ((xfsp = XLoadQueryFont(xdisp,fontname)) == NULL)
        xfsp = XLoadQueryFont(xdisp, "fixed");
    xfont = xfsp->fid;
    fontname = "6x13";                             /* set up font info... */
    if ((xbfsp = XLoadQueryFont(xdisp,fontname)) == NULL)
        xbfsp = XLoadQueryFont(xdisp, "fixed");
    xbfont = xbfsp->fid;
    myXwait(Str(X_358,"New window: \nPosition & size, \nclick to go on"));
    xlwin = (Window)NULL;               /* first draw to win has no wait */
    return(xwin);
}

void MakeGraph_(WINDAT *wdptr, char *name)
{
    wdptr->windid = MakeWindow(0,name);
}

#define GUTTERH 20           /* space for text at top & bottom */
#define BORDERW 10           /* inset from L & R edge */

void MakeXYin_(XYINDAT *wdptr, MYFLT x, MYFLT y) /* initial proportions */
{
    Window      xwin;
    XWindowAttributes info;
    int     b;
    short   win_x, win_y, win_w, win_h;
    short   gra_x, gra_y, gra_w, gra_h;

    wdptr->windid = xwin = MakeWindow(1,Str(X_544,"XY input"));
    wdptr->down = 0;    /* by def released after Make */

    XGetWindowAttributes(xdisp,xwin,&info);
    win_w = info.width;  win_h = info.height;
    win_x = 0;  win_y = 0;              /* window pixels addressed relative */

    /* set new width and height so we leave a 20% border around the plot */
    gra_w = win_w - 2*BORDERW;          gra_h = win_h - 2*GUTTERH;
    gra_x = win_x + BORDERW;            gra_y = win_y + GUTTERH;
    wdptr->m_x = gra_x + (int)(x * (MYFLT)gra_w);
    wdptr->m_y = gra_y + (int)(y * (MYFLT)gra_h);
                /* draw up new xhairs */
    XDrawLine(xdisp,xwin,xdfgc,gra_x,wdptr->m_y,(gra_x+gra_w),wdptr->m_y);
    XDrawLine(xdisp,xwin,xdfgc,wdptr->m_x,gra_y, wdptr->m_x,(gra_y+gra_h));
    wdptr->x = x;       wdptr->y = y;
}

void DrawGraph_(WINDAT *wdptr)
{
    MYFLT       *fdata = wdptr->fdata;
    long        npts   = wdptr->npts;
    char        *msg   = wdptr->caption;

/*    XPoint    *xpts;          now a static global */
    XWindowAttributes info;
    XTextItem   txitem[1];
    Window      xwin;
    short       win_x, win_y, win_w, win_h;     /* window rect */
    short       gra_x, gra_y, gra_w, gra_h;     /* graph rect is inset */
    short       y_axis;
    int         lsegs,pts_pls;
    int         pol;
    char        string[80];

    xwin = wdptr->windid;
    pol  = wdptr->polarity;

    if (wdptr->waitflg)
      myXwait(Str(X_220,"Click here to continue.."));
    xlwin = xwin;                   /* keep track of latest window for msgs */
    /* setting xlwin here rather than in MakeWin avoids first pause */

    XClearWindow(xdisp, xwin);
    XGetWindowAttributes(xdisp,xwin,&info);
    win_w = info.width;  win_h = info.height;
    win_x = 0;  win_y = 0;              /* window pixels addressed relative */

    /* set new width and height so we leave a 20% border around the plot */
    gra_w = win_w - 2*BORDERW;
    gra_h = win_h - 2*GUTTERH;
    gra_x = win_x + BORDERW;
    gra_y = win_y + GUTTERH;
    /* figure height of Y axis - top, middle or bottom */
    if (pol == (short)BIPOL)
      y_axis = gra_y + (gra_h/2);
    else if (pol == (short)NEGPOL)
      y_axis = gra_y;
    else                /* POSPOL */
      y_axis = gra_y + gra_h;

/*    if (npts < gra_w) */
/*      lsegs = npts; */
/*    else */
/*      lsegs = gra_w; */         /* max one datum per w pixel */
    if (npts < MAXLSEGS) {
      lsegs = npts;                   /* one lineseg per datum */
      pts_pls = 1;
    }
    else {
      pts_pls = npts/MAXLSEGS;
      if (npts%MAXLSEGS) pts_pls++;
      lsegs = npts/pts_pls;
    }

    /* THIS ASSIGNMENT ASSUMES LONG INT FOR NPTS > 32767 */

    /* alloc point array */
    /*      xpts = (XPoint *) mmalloc((long)lsegs * sizeof(XPoint));    */

    {       /* take scale factors out of for-loop for faster run-time */
      MYFLT x_scale = gra_w / (MYFLT)(lsegs-1);
      MYFLT y_scale = gra_h / wdptr->oabsmax; /* unipolar default */
      XPoint *ptptr = xpts;
      MYFLT  f,ma,mi,*fdptr = fdata;
      int c,i = 0, j = lsegs;

      if (pol == (short)BIPOL)
        y_scale /= 2.0;             /* max data scales to h/2 */
      /* put x-y pairs into a point list for XDraw */
      while(j--) {
        ptptr->x = gra_x + (short)((MYFLT)i++ * x_scale);
        if (pts_pls == 1)
          f = *fdptr++;
        else {
          ma = mi = *fdptr++;
          for (c = 1; c < pts_pls; ++c)
            if ((f = *fdptr++) > ma)    ma = f;
            else if ( f<mi )            mi = f;
          if (ma < 0)             f = mi;
          else if (mi > 0)        f = ma;
          else if (ma > -mi)      f = ma;
          else f = mi;
        }
        (ptptr++)->y = y_axis - (short)(f * y_scale);
      }
    }

    XDrawLines(xdisp, xwin, xdfgc, xpts, lsegs, CoordModeOrigin);
/*    free(pts);                                /* ! return data array */

    /* now draw axes: y-axis is always on the left edge,
       x-axis height is determined by the case we're in */
    XDrawLine(xdisp, xwin, xdfgc, gra_x, y_axis, (gra_x + gra_w), y_axis);
    XDrawLine(xdisp, xwin, xdfgc, gra_x, gra_y, gra_x, (gra_y + gra_h));

    if (wdptr->danflag) {       /* flag to add dotted divider */
      /*      SetDottedXorPen();      */
      XDrawLine(xdisp, xwin, xdfgc, win_x+win_w/2, win_y+GUTTERH,
                win_x+win_w/2, win_y+win_h-GUTTERH);
      /*      RestoreSolidCopyPen();  */
    }
    sprintf(string,Str(X_49,"%s  %ld points, max %5.3f"),msg,npts,wdptr->oabsmax);
    txitem[0].chars  = string;          /* draw the label under the curve */
    txitem[0].nchars = strlen(string);
    txitem[0].delta  = 1;
    txitem[0].font   = xfont;
    XDrawText(xdisp, xwin, xdfgc, gra_x, (gra_y + gra_h + TXHGHT), txitem, 1);
    XFlush(xdisp);                    /* finally, flush output to the screen */
}

void ReadXYin_(XYINDAT *wdptr)
{
    XEvent      theEv;
    Window      xwin;
    XWindowAttributes info;
    int         b;
    short       win_x, win_y, win_w, win_h;
    short       gra_x, gra_y, gra_w, gra_h;
    short       m_x, m_y;

    xwin = wdptr->windid;
    m_x  = wdptr->m_x;
    m_y  = wdptr->m_y;
    while ( XCheckWindowEvent(xdisp, xwin,
                              ButtonReleaseMask|ButtonMotionMask|ButtonPressMask,
                              &theEv) )
      /* there was an event.. */
      switch(theEv.type) {
      case ButtonPress:
        m_x = theEv.xbutton.x;
        m_y = theEv.xbutton.y;
        wdptr->down = 1;
        break;
      case ButtonRelease:
        m_x = theEv.xbutton.x;
        m_y = theEv.xbutton.y;
        wdptr->down = 0;
        break;
      case MotionNotify:
        m_x = theEv.xmotion.x;
        m_y = theEv.xmotion.y;
        break;
      default:
        break;
      }
    /* cycle until no events pending */
    XGetWindowAttributes(xdisp,xwin,&info);
    win_w = info.width;  win_h = info.height;
    win_x = 0;  win_y = 0;      /* window pixels addressed relative */

    /* set new width and height so we leave a 20% border around the plot */
    gra_w = win_w - 2*BORDERW;          gra_h = win_h - 2*GUTTERH;
    gra_x = win_x + BORDERW;            gra_y = win_y + GUTTERH;

    /* clip mouse position */
    if (m_x < gra_x)                            m_x = gra_x;
    else if (m_x > gra_x+gra_w)         m_x = gra_x+gra_w;
    if (m_y < gra_y)                                    m_y = gra_y;
    else if (m_y > gra_y+gra_h)         m_y = gra_y+gra_h;

    if (m_x != wdptr->m_x || m_y != wdptr->m_y) { /* only redo if changed */
      /* undraw old crosshairs */
      XSetForeground(xdisp, xdfgc, XWhitePixel(xdisp, xscrn));
      XDrawLine(xdisp, xwin, xdfgc, gra_x, wdptr->m_y,
                (gra_x + gra_w), wdptr->m_y);
      XDrawLine(xdisp, xwin, xdfgc, wdptr->m_x, gra_y, wdptr->m_x,
                (gra_y + gra_h));
      /* draw up new xhairs */
      XSetForeground(xdisp, xdfgc, XBlackPixel(xdisp, xscrn));
      XDrawLine(xdisp, xwin, xdfgc, gra_x, m_y, (gra_x + gra_w), m_y);
      XDrawLine(xdisp, xwin, xdfgc, m_x, gra_y, m_x, (gra_y + gra_h));
      wdptr->m_x = m_x;       wdptr->m_y = m_y;
      wdptr->x = ((MYFLT)m_x-gra_x)/(MYFLT)gra_w;
      wdptr->y = ((MYFLT)m_y-gra_y)/(MYFLT)gra_h;
    }
}

static void KillWin(Window *pxwin)
{
    XUnmapWindow(xdisp, *pxwin);
    XDestroyWindow(xdisp, *pxwin);
    *pxwin = (Window)NULL;
}

void KillGraph_(WINDAT *wdptr)
{
    KillWin((Window *)&(wdptr->windid));
}

void KillXYin_(XYINDAT *wdptr)
{
    KillWin((Window *)&(wdptr->windid));
}

int ExitGraph_(void) /* print click-Exit message in most recently active window */
{
    char *env = getenv("CSNOSTOP");
    if (env==NULL || strcmp(env,"yes")==0)
      myXwait(Str(X_662,"click here to EXIT"));
    return 0;
}



