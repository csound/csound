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
    wdptr->windid = MakeWindow_FLTK(csound,(char*)name);
}

static void KillGraph_FLTK(CSOUND *csound, WINDAT *wdptr)
{
    kill_graph(csound, wdptr->windid);
}

static int dummyWidgetOpcode(CSOUND *csound, void *p)
{
    const char  *opname;

    opname = csound->GetOpcodeName(p);
    csound->Die(csound, Str("%s: widget opcodes have been disabled by "
                            "the host application"), opname);
    return NOTOK;
}

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    (void)csound;
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    const OENTRY  *ep = &(widgetOpcodes_[0]);
    int           initFlags = 0;
    int           *fltkFlags;
    int           enableDisplays = 0;
    OPARMS oparms;
     csound->GetOParms(csound, &oparms);

    if (csound->QueryGlobalVariable(csound,
                                    "FLTK_Flags") == (void*) 0) {
      if (csound->CreateGlobalVariable(csound,
                                       "FLTK_Flags", sizeof(int)) != 0)
        csound->Die(csound, "%s",
                    Str("widgets.cpp: error allocating FLTK flags"));
      initFlags = 1;
    }
    fltkFlags = getFLTKFlagsPtr(csound);
    if (((*fltkFlags) & 2) == 0 &&
        !(oparms.graphsoff || oparms.postscript)) {
#ifdef LINUX
      Display *dpy = XOpenDisplay(NULL);
      if (dpy != NULL) {
        XCloseDisplay(dpy);
#endif
        if (csound->SetIsGraphable(csound, 1) == 0) {
          enableDisplays = 1;
          (*fltkFlags) |= 64;

          if (!((*fltkFlags) & 256))
            csound->SetInternalYieldCallback(csound, CsoundYield_FLTK);
          flgraph_init(csound); /* Create space */
          csound->SetMakeGraphCallback(csound, MakeGraph_FLTK);
          csound->SetDrawGraphCallback(csound, DrawGraph_FLTK);
          csound->SetKillGraphCallback(csound, KillGraph_FLTK);
          csound->SetExitGraphCallback(csound, ExitGraph_FLTK);
           /* seemed to crash, but not anymore... */
          csound->RegisterResetCallback(csound, NULL, widget_reset);
          csound->Message(csound, "graph init... \n");

        }
#ifdef LINUX
      }
#endif
    }
    if (initFlags) {
#ifndef __MACH__
      if (enableDisplays)
#endif
        (*fltkFlags) |= 28;
    }
    if (!((*fltkFlags) & 129))
      for ( ; ep->opname != NULL; ep++) {
        if (csound->AppendOpcode(csound, ep->opname,
                                 (int)ep->dsblksiz, (int)ep->flags, (int)ep->thread,
                                 ep->outypes, ep->intypes,
                                 ep->iopadr, ep->kopadr, ep->aopadr) != 0) {
          csound->ErrorMsg(csound, Str("Error registering opcode '%s'"),
                                   ep->opname);
          return -1;
        }
      }
    else if (!((*fltkFlags) & 128)) {
      for ( ; ep->opname != NULL; ep++) {
        if (csound->AppendOpcode(
                                 csound, ep->opname, (int)ep->dsblksiz,
                                 (int)ep->flags,(int)ep->thread,
                ep->outypes, ep->intypes,
                (((int)ep->thread & 1) ? dummyWidgetOpcode : (SUBR) 0),
                (((int)ep->thread & 2) ? dummyWidgetOpcode : (SUBR) 0),
                (((int)ep->thread & 4) ? dummyWidgetOpcode : (SUBR) 0)) != 0) {
          csound->ErrorMsg(csound, Str("Error registering opcode '%s'"),
                                   ep->opname);
          return -1;
        }
      }
    }

    widget_init(csound);
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int)sizeof(MYFLT));
}
