/*
    control.h:

    Copyright (C) 2000 John ffitch

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

/********************************************/
/* Controls                                 */
/********************************************/

#pragma once
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
typedef struct CONTROL_GLOBALS_ {
    CSOUND  *csound;
    char    cmd[100];
    int32_t    wish_pid;
    int32_t    pip1[2];
    int32_t    pip2[2];
    FILE    *wish_cmd, *wish_res;
    int32_t    *values;
    int32_t    *minvals;
    int32_t    *maxvals;
    int32_t    max_sliders;
    int32_t    *buttons;
    int32_t    *checks;
    int32_t    max_button;
    int32_t    max_check;
} CONTROL_GLOBALS;

typedef struct {
    OPDS    h;
    MYFLT   *kdest, *kcntl;
    CONTROL_GLOBALS *p;
} CNTRL;

typedef struct {
    OPDS    h;
    MYFLT   *kcntl, *val, *which;
    CONTROL_GLOBALS *p;
} SCNTRL;

typedef struct {
    OPDS    h;
    MYFLT   *kcntl, *val;
    CONTROL_GLOBALS *p;
} TXTWIN;

