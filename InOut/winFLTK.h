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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef CSOUND_WINFLTK_H
#define CSOUND_WINFLTK_H

#include "csdl.h"
#ifdef __cplusplus
#include <FL/Fl.H>
#endif

/**
 * FLTK flags is the sum of any of the following values:
 *   1 (input):  disable widget opcodes by setting up dummy opcodes instead
 *   2 (input):  disable FLTK graphs
 *   4 (input):  disable the use of a separate thread for widget opcodes
 *   8 (input):  disable the use of Fl::lock() and Fl::unlock()
 *  16 (input):  disable the use of Fl::awake()
 *  32 (output): widget opcodes are used
 *  64 (output): FLTK graphs are used
 * 128 (input):  disable widget opcodes by not registering any opcodes
 * 256 (input):  disable the use of Fl::wait() (implies no widget thread)
 */

static inline int getFLTKFlags(CSOUND *csound)
{
    return (*((int*) csound->QueryGlobalVariableNoCheck(csound, "FLTK_Flags")));
}

static inline int *getFLTKFlagsPtr(CSOUND *csound)
{
    return ((int*) csound->QueryGlobalVariableNoCheck(csound, "FLTK_Flags"));
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

static inline void Fl_wait(CSOUND *csound, double seconds)
{
    if (!(getFLTKFlags(csound) & 256))
      Fl::wait(seconds);
}

static inline void Fl_wait_locked(CSOUND *csound, double seconds)
{
    int     fltkFlags;

    fltkFlags = getFLTKFlags(csound);
    if (!(fltkFlags & 256)) {
#ifndef NO_FLTK_THREADS
      if (!(fltkFlags & 8))
        Fl::lock();
#endif
      Fl::wait(seconds);
#ifndef NO_FLTK_THREADS
      if (!(fltkFlags & 8))
        Fl::unlock();
#endif
    }
}

#endif  /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

extern  int       CsoundYield_FLTK(CSOUND *);
extern  void      DrawGraph_FLTK(CSOUND *, WINDAT *);
extern  int       ExitGraph_FLTK(CSOUND *);
extern  void      kill_graph(CSOUND *, uintptr_t);
extern  void      KillXYin_FLTK(CSOUND *, XYINDAT *);
extern  uintptr_t MakeWindow_FLTK(CSOUND *, char *);
extern  void      MakeXYin_FLTK(CSOUND *, XYINDAT *, MYFLT, MYFLT);
extern  int       myFLwait(void);
extern  void      ReadXYin_FLTK(CSOUND *, XYINDAT *);
  extern  void      flgraph_init(CSOUND *csound);
extern  void      widget_init(CSOUND *);
extern  int       widget_reset(CSOUND *, void *);

extern const OENTRY widgetOpcodes_[];

#ifdef __cplusplus
}       /* extern "C" */
#endif

#endif  /* CSOUND_WINFLTK_H */

