/*
  * pythonopcodes.h
  *
  * Copyright (C) 2002 Maurizio Umberto Puxeddu
  *
  * This software is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This software is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this software; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

#ifndef _pycsound_pythonopcodes_h_
#define _pycsound_pythonopcodes_h_

#ifdef _DEBUG
# undef _DEBUG
#  include <Python.h>
# define _DEBUG
#else
# include <Python.h>
#endif
#include "csdl.h"
#include "pyx.auto.h"
#include "pycall.auto.h"

typedef struct {
  OPDS h;
} PYINIT;

typedef struct {
  OPDS h;
  STRINGDAT *function;
  MYFLT *nresult;
  MYFLT *args[VARGMAX];
} PYCALLN;

#endif /* _pycsound_pythonopcodes_h_ */

