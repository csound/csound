/*
* C S O U N D
*
* External language interfaces for the "C" Csound API.
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
%module csnd
%include "typemaps.i"
%include "std_string.i"
%include "std_vector.i"
%feature("autodoc", "1");
%{
    #include "csound.h"
    #include "cfgvar.h"
    #include "csound.hpp"
    #include "CsoundFile.hpp"
    #include "CppSound.hpp"
    #include "filebuilding.h"
%}

%apply int { size_t };
typedef unsigned int uint32_t;
#ifndef MSVC
%apply long long { uint32_t };
#endif

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

%include "exclusions.i"

%include "csound.h"
%include "cfgvar.h"

%apply MYFLT &OUTPUT { MYFLT &dflt, MYFLT &min, MYFLT &max };
%apply MYFLT &OUTPUT { MYFLT &value };

%include "csound.hpp"

%clear MYFLT &dflt;
%clear MYFLT &min;
%clear MYFLT &max;
%clear MYFLT &value;

%include "CsoundFile.hpp"
%include "CppSound.hpp"
%include "filebuilding.h"

