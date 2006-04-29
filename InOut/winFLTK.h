/*
    winFLTK.h:

    Copyright (C) 2006 Istvan Varga

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

#ifndef CSOUND_WINFLTK_H
#define CSOUND_WINFLTK_H

#include "csdl.h"
#ifdef __cplusplus
#include <FL/Fl.H>
#endif

/**
 * FLTK flags is the sum of any of the following values:
 *   1 (input):  disable widget opcodes
 *   2 (input):  disable FLTK graphs
 *   4 (input):  disable the use of a separate thread for widget opcodes
 *   8 (input):  disable the use of Fl::lock() and Fl::unlock()
 *  16 (input):  disable the use of Fl::awake()
 *  32 (output): widget opcodes are used
 *  64 (output): FLTK graphs are used
 */

static inline int getFLTKFlags(CSOUND *csound)
{
    return (*((int*) csound->QueryGlobalVariableNoCheck(csound, "FLTK_Flags")));
}

#ifdef __cplusplus

static inline void Fl_lock(CSOUND *csound)
{
#ifdef NO_FLTK_THREADS
    (void) csound;
#else
    if (!(getFLTKFlags(csound) & 8)) {
      Fl::lock();
    }
#endif
}

static inline void Fl_unlock(CSOUND *csound)
{
#ifdef NO_FLTK_THREADS
    (void) csound;
#else
    if (!(getFLTKFlags(csound) & 8)) {
      Fl::unlock();
    }
#endif
}

static inline void Fl_awake(CSOUND *csound)
{
#ifdef NO_FLTK_THREADS
    (void) csound;
#else
    if (!(getFLTKFlags(csound) & 16)) {
      Fl::awake();
    }
#endif
}

#endif  /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

extern  int       CsoundYield_FLTK(CSOUND *);
extern  void      DrawGraph_FLTK(CSOUND *, WINDAT *);
extern  int       ExitGraph_FLTK(CSOUND *);
extern  void      kill_graph(uintptr_t);
extern  void      KillXYin_FLTK(CSOUND *, XYINDAT *);
extern  uintptr_t MakeWindow_FLTK(char *);
extern  void      MakeXYin_FLTK(CSOUND *, XYINDAT *, MYFLT, MYFLT);
extern  int       myFLwait(void);
extern  void      ReadXYin_FLTK(CSOUND *, XYINDAT *);
extern  void      set_display_callbacks(CSOUND *);

#ifdef __cplusplus
}       /* extern "C" */
#endif

#endif  /* CSOUND_WINFLTK_H */

