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

#include "winFLTK.h"

static void MakeGraph_FLTK(CSOUND *csound, WINDAT *wdptr, const char *name)
{
    wdptr->windid = MakeWindow_FLTK((char*) name);
}

static void KillGraph_FLTK(CSOUND *csound, WINDAT *wdptr)
{
    kill_graph(wdptr->windid);
}

void set_display_callbacks(CSOUND *csound)
{
    if (getFLTKFlags(csound) & 2)
      return;
#ifdef LINUX
    {
      Display *dpy = XOpenDisplay(NULL);
      if (dpy == NULL)
        return;
      XCloseDisplay(dpy);
    }
#endif
    if (csound->SetIsGraphable(csound, 1) != 0)
      return;
    if (!csound->oparms->displays)
      return;
    if (csound->oparms->graphsoff || csound->oparms->postscript)
      return;
    *((int*) csound->QueryGlobalVariableNoCheck(csound, "FLTK_Flags")) |= 64;
    csound->SetYieldCallback(csound, CsoundYield_FLTK);
    csound->SetMakeGraphCallback(csound, MakeGraph_FLTK);
    csound->SetDrawGraphCallback(csound, DrawGraph_FLTK);
    csound->SetKillGraphCallback(csound, KillGraph_FLTK);
    csound->SetExitGraphCallback(csound, ExitGraph_FLTK);
    csound->SetMakeXYinCallback(csound, MakeXYin_FLTK);
    csound->SetReadXYinCallback(csound, ReadXYin_FLTK);
    csound->SetKillXYinCallback(csound, KillXYin_FLTK);
}

