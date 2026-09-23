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
Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#pragma once

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"


typedef struct {
    OPDS h;

    cs_float *ires;
    cs_float *imod;
    cs_float *iparm;
    cs_float *imatrix;
    cs_float *inummod;
    cs_float *inumparm;
    cs_float *kupdate;

    FUNC *restab, *modtab, *parmtab, *mattab;
    int32_t nummod, numparm;
    /* Variables for the preprocessed matrix */
    int32_t doscan, scanned;
        AUXCH aux;
    cs_float *proc_mat;
    int32_t *mod_map, *parm_map;
    cs_float *remap_mod, *remap_parm;
    int32_t
    nummod_scanned, numparm_scanned;
} MODMATRIX;

