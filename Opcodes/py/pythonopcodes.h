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
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#ifndef _pycsound_pythonopcodes_h_
#define _pycsound_pythonopcodes_h_

#include <Python.h>
#include "csdl.h"
#include "pyx.h.auto"
#include "pycall.h.auto"

typedef struct {
  OPDS h;
  MYFLT *function;
  MYFLT *nresult;
  MYFLT *args[VARGMAX];
} PYCALLN;

extern int pycalln_krate(PYCALLN *p);
extern int pylcalln_irate(PYCALLN *p);
extern int pylcalln_krate(PYCALLN *p);
extern int pylcallni_irate(PYCALLN *p);

#endif /* _pycsound_pythonopcodes_h_ */
