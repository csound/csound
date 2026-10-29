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
* Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

%module(directors="1") csnd7

%feature("director") CsoundCallbackWrapper;
%feature("nodirector") Csound;
%include "typemaps.i"
%include "arrays_java.i"
%include "various.i" 
%include "std_string.i"
%include "std_vector.i"
%feature("autodoc", "1");
%{
    #include "csound.h"
    #include "csound.hpp"
    #include "cs_glue.hpp"
    #include "csPerfThread.hpp"
%}

%apply int { size_t };

%include "stdint.i"

/* %typemap(freearg) char ** {
  free((char *) $1);
} */

// Enable the JNI class to load the required native library.
%pragma(java) jniclasscode=%{
  static {
    System.loadLibrary("_jcsound7");
  }
%}

%apply char **STRING_ARRAY { char **argv, const char **argv };

%include "exclusions.i"

%include "csound_types.h"
%include "csound.h"
%include "cfgvar.h"

%apply cs_float *OUTPUT { cs_float *dest };
%apply cs_float *INPUT { cs_float *src };
%include "csound.hpp"


%ignore CsoundPerformanceThread::SetProcessCallback(void (*Callback)(void *), void *cbdata);

%ignore CsoundMYFLTArray::SetStringValue;
%include "cs_glue.hpp"
%include "csPerfThread.hpp"



