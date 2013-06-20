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

/* module is now csnd6 */
%{
    #include <stdint.h>
%}
%module(directors="1") csnd6

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
    #include "cfgvar.h"
    #include "csound.hpp"
    #include "cs_glue.hpp"
    #include "csPerfThread.hpp"
    #include "CsoundFile.hpp"
    #include "CppSound.hpp"
    #include "Soundfile.hpp"
%}

%apply int { size_t };

typedef unsigned int uint32_t;

/* %typemap(freearg) char ** {
  free((char *) $1);
} */

// Enable the JNI class to load the required native library.
%pragma(java) jniclasscode=%{
  static {
    try {
        java.lang.System.loadLibrary("_jcsound6");
    } catch (UnsatisfiedLinkError e) {
        java.lang.System.err.println("_jcsound6 native code library failed to load.\n" + e);
        java.lang.System.exit(1);
    }
  }
%}


%apply char **STRING_ARRAY { char **argv };

%include "exclusions.i"

%include "csound.h"
%include "cfgvar.h"


%apply MYFLT *OUTPUT { MYFLT *dest };
%apply MYFLT *INPUT { MYFLT *src };
%ignore Csound::SetCscoreCallback(void (*cscoreCallback_)(CSOUND *));
//%ignore Csound::SetOutputValueCallback(void (*)(CSOUND *, const char *, MYFLT));
//%ignore Csound::SetInputValueCallback(void (*)(CSOUND *, const char *, MYFLT *));
%ignore Csound::SetExternalMidiInOpenCallback(int (*)(CSOUND *, void *, const char*));
%ignore Csound::SetExternalMidiReadCallback(int (*)(CSOUND *, void *, unsigned char *, int));
%ignore Csound::SetExternalMidiInCloseCallback(int (*)(CSOUND *, void *));
%ignore Csound::SetHostData(void *);
%ignore Csound::GetHostData();
%ignore Csound::SetMessageCallback(void (*)(CSOUND *, int attr,const char *format, va_list valist));
%ignore Csound::CreateConfigurationVariable;
%ignore Csound::SetConfigurationVariable;
%ignore Csound::QueryConfigurationVariable;
%ignore Csound::ListConfigurationVariables;
%ignore Csound::DeleteConfigurationVariable;
%ignore Csound::ParseConfigurationVariable;
%ignore Csound::GetHostData();
//%ignore Csound::setPydata;
//%ignore Csound::getPydata;

%include "csound.hpp"


%ignore CsoundPerformanceThread::SetProcessCallback(void (*Callback)(void *), void *cbdata);

%include "cs_glue.hpp"
%include "csPerfThread.hpp"
%include "CsoundFile.hpp"
%include "CppSound.hpp"
%include "Soundfile.hpp"

