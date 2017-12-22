/*
* C S O U N D A C
*
* Copyright (c) 2001-2003 by Michael Gogins. All rights reserved.
*
* CsoundAC is a Python extension module for doing algorithmic
* composition, in one which one writes music by programming in
* Python. Musical events are points in music space with dimensions
* {time, duration, event type, instrument, pitch as MIDI key,
* loudness as MIDI velocity, phase, pan, depth, height, pitch-class
* set, 1}, and pieces are composed by assembling a hierarchical tree
* of nodes in music space. Each node has its own local transformation
* of coordinates in music space. Nodes can be empty, contain scores
* or fragments of scores, generate scores, or transform
* scores. CsoundAC also contains a Python interface to the Csound
* API, making it easy to render CsoundAC compositions using Csound.
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

%begin %{
#ifdef _MSC_VER
#define SWIG_PYTHON_INTERPRETER_NO_DEBUG
#endif
%}

/* This function already gets exported in other form */
/* Don't export this to avoid build failures on amd64 */
%ignore Counterpoint::message(const char*, va_list);
%ignore csound::print(const char*, va_list);


%module(directors="1") CsoundAC
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
%include <Silence.hpp>
%include <Conversions.hpp>

%pythoncode
%{
# Create one global instance of CppSound for CsoundAC to grab.
# Create it in the main module, so that scripts for CsoundAC
# will also work in a standalone Python interpreter.
import sys
import csnd6
sys.modules["__main__"].csound = csnd6.CppSound()
sys.modules["__main__"].csound.thisown = 0
sys.modules["__main__"].csound.setPythonMessageCallback()
%}



#endif
