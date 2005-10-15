/*
* C S O U N D   V S T 
*
* An auto-extensible system for making music on computers by means of software alone.
* Copyright (c) 2001-2003 by Michael Gogins. All rights reserved.
*
* L I C E N S E
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
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#if defined(SWIGPYTHON)

%module(directors="1") CsoundVST
%{
	#include <algorithm>
	#include "Silence.hpp"
%}
%apply int { size_t };

%typemap(in) char ** {
  /* Check if is a list */
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = (char **) malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
	$1[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
	PyErr_SetString(PyExc_TypeError,"list must contain strings");
	free($1);
	return NULL;
      }
    }
    $1[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

%typemap(freearg) char ** {
  free((char *) $1);
}

%feature("director") Node;
%include <csound.h>
%include <cfgvar.h>
%include "Silence.hpp"


%pythoncode
%{
# Create one global instance of CppSound for CsoundVST to grab.
# Create it in the main module, so that scripts for CsoundVST 
# will also work in a standalone Python interpreter.
import sys
sys.modules["__main__"].csound = CppSound()
sys.modules["__main__"].csound.thisown = 0
sys.modules["__main__"].csound.setPythonMessageCallback()
%}

#endif
