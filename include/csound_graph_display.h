/*
  graph_display.h: graphs and displays

  Copyright (C) 2024

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

#ifndef GRAPH_DISPLAY_H
#define GRAPH_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif
  /** @defgroup TABLEDISPLAY Function table display
   *
   *  @{ */
  typedef struct windat_  WINDAT;
  
  /**
   * Tells Csound whether external graphic table display is supported.
   * Returns the previously set value (initially zero).
   */
  PUBLIC int32_t csoundSetIsGraphable(CSOUND *, int32_t isGraphable);

  /**
   * Called by external software to set Csound's MakeGraph function.
   */
  PUBLIC void csoundSetMakeGraphCallback(CSOUND *,
                                         void (*makeGraphCallback_)(CSOUND *,
                                                                    WINDAT *windat,
                                                                    const char *name));

  /**
   * Called by external software to set Csound's DrawGraph function.
   */
  PUBLIC void csoundSetDrawGraphCallback(CSOUND *,
                                         void (*drawGraphCallback_)(CSOUND *,
                                                                    WINDAT *windat));

  /**
   * Called by external software to set Csound's KillGraph function.
   */
  PUBLIC void csoundSetKillGraphCallback(CSOUND *,
                                         void (*killGraphCallback_)(CSOUND *,
                                                                    WINDAT *windat));

  /**
   * Called by external software to set Csound's ExitGraph function.
   */
  PUBLIC void csoundSetExitGraphCallback(CSOUND *,
                                         int32_t (*exitGraphCallback_)(CSOUND *));
/** @}*/

#ifdef __cplusplus
}
#endif
#endif 
