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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/********************************************/
/* Controls                                 */
/********************************************/

#include "csdl.h"

typedef struct CONTROL_GLOBALS_ {
    CSOUND  *csound;
    char    cmd[100];
    int     wish_pid;
    int     pip1[2];
    int     pip2[2];
    FILE    *wish_cmd, *wish_res;
    int     *values;
    int     *minvals;
    int     *maxvals;
    int     max_sliders;
    int     *buttons;
    int     *checks;
    int     max_button;
    int     max_check;
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

