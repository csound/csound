/*
    winFLTK.c: graphs using FLTK library

    Copyright (C) 2002 John ffitch

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

/*    winFLTK.c         */
/* Csound FLTK/X graphs */
/*   jpff,06Oct02       */

#include "csdl.h"
#include <stdio.h>
#include "cwindow.h"

#ifdef LINUX
#include <X11/Xlib.h>
#endif

extern long MakeWindow_FLTK(char *);
extern void DrawGraph_FLTK(CSOUND *csound, WINDAT *wdptr);
extern int  CsoundYield_FLTK(CSOUND *csound);
extern void kill_graph(int);
extern int  myFLwait(void);

static void MakeGraph_FLTK(CSOUND *csound, WINDAT *wdptr, const char *name)
{
    wdptr->windid = MakeWindow_FLTK((char*) name);
}

static void KillGraph_FLTK(CSOUND *csound, WINDAT *wdptr)
{
    kill_graph(wdptr->windid);
}

/* print click-Exit message in most recently active window */

#if 0
static int ExitGraph_FLTK(CSOUND *csound)
{
    const char *env = csoundGetEnv(csound, "CSNOSTOP");
    if (env == NULL || strcmp(env, "yes") == 0)
      myFLwait();
    return 0;
}
#endif

void set_display_callbacks(CSOUND *csound)
{
#ifdef LINUX
    Display *dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
      csound->SetIsGraphable(csound, 0);
      return;
    }
    XCloseDisplay(dpy);
#endif
    csound->SetIsGraphable(csound, 1);
#ifdef NO_FLTK_THREADS
    csound->SetYieldCallback(csound, CsoundYield_FLTK);
#endif
    if (!csound->oparms->displays)
      return;
    if (csound->oparms->graphsoff || csound->oparms->postscript)
      return;
#ifndef NO_FLTK_THREADS
    csound->SetYieldCallback(csound, CsoundYield_FLTK);
#endif
    csound->SetMakeGraphCallback(csound, MakeGraph_FLTK);
    csound->SetDrawGraphCallback(csound, DrawGraph_FLTK);
    csound->SetKillGraphCallback(csound, KillGraph_FLTK);
 /* csound->SetExitGraphCallback(csound, ExitGraph_FLTK); */
 /* csound->SetMakeXYinCallback(csound, MakeXYin_FLTK); */
 /* csound->SetReadXYinCallback(csound, ReadXYin_FLTK); */
 /* csound->SetKillXYinCallback(csound, KillXYin_FLTK); */
}

