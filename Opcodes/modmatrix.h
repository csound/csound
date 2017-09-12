/*
Partikkel - a granular synthesis module for Csound 5
Copyright (C) 2009 Øyvind Brandtsegg, Thom Johansen

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

//#include "csdl.h"
#include "interlocks.h"
#include "csoundCore.h"

typedef struct {
    OPDS h;

    MYFLT *ires;
    MYFLT *imod;
    MYFLT *iparm;
    MYFLT *imatrix;
    MYFLT *inummod;
    MYFLT *inumparm;
    MYFLT *kupdate;

    FUNC *restab, *modtab, *parmtab, *mattab;
    int nummod, numparm;
    /* Variables for the preprocessed matrix */
    int doscan, scanned;
        AUXCH aux;
    MYFLT *proc_mat;
    int *mod_map, *parm_map;
    MYFLT *remap_mod, *remap_parm;
    int nummod_scanned, numparm_scanned;
} MODMATRIX;

