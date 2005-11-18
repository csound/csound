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

%ignore csoundInitializeCscore;
%ignore csoundQueryInterface;
%ignore csoundGetHostData;
%ignore csoundSetHostData;
%ignore csoundGetInputBuffer;
%ignore csoundGetOutputBuffer;
%ignore csoundGetSpin;
%ignore csoundGetSpout;
%ignore csoundSetCscoreCallback;
%ignore csoundScoreSort;
%ignore csoundScoreExtract;
%ignore csoundMessageV;
%ignore csoundSetMessageCallback;
%ignore csoundSetInputValueCallback;
%ignore csoundSetOutputValueCallback;
%ignore csoundSetExternalMidiInOpenCallback;
%ignore csoundSetExternalMidiReadCallback;
%ignore csoundSetExternalMidiInCloseCallback;
%ignore csoundSetExternalMidiOutOpenCallback;
%ignore csoundSetExternalMidiWriteCallback;
%ignore csoundSetExternalMidiOutCloseCallback;
%ignore csoundSetExternalMidiErrorStringCallback;
%ignore csoundSetIsGraphable;
%ignore csoundSetMakeGraphCallback;
%ignore csoundSetDrawGraphCallback;
%ignore csoundSetKillGraphCallback;
%ignore csoundSetMakeXYinCallback;
%ignore csoundSetReadXYinCallback;
%ignore csoundSetKillXYinCallback;
%ignore csoundSetExitGraphCallback;
%ignore csoundAppendOpcode;
%ignore csoundSetYieldCallback;
%ignore csoundSetPlayopenCallback;
%ignore csoundSetRtplayCallback;
%ignore csoundSetRecopenCallback;
%ignore csoundSetRtrecordCallback;
%ignore csoundSetRtcloseCallback;
%ignore csoundGetTable;
%ignore csoundCreateThread;
%ignore csoundJoinThread;
%ignore csoundCreateThreadLock;
%ignore csoundWaitThreadLock;
%ignore csoundWaitThreadLockNoTimeout;
%ignore csoundNotifyThreadLock;
%ignore csoundDestroyThreadLock;
%ignore csoundInitTimerStruct;
%ignore csoundGetRealTime;
%ignore csoundGetCPUTime;
%ignore csoundGetRandomSeedFromTime;
%ignore csoundQueryGlobalVariableNoCheck;
%ignore csoundDestroyGlobalVariable;
%ignore csoundGetRtRecordUserData;
%ignore csoundGetRtPlayUserData;
%ignore csoundRegisterSenseEventCallback;
%ignore csoundGetChannelPtr;
%ignore csoundSeedRandMT;
%ignore csoundRandMT;
%ignore csoundChanIASet;
%ignore csoundChanOAGet;
%ignore csoundCreateGlobalConfigurationVariable;
%ignore csoundCopyGlobalConfigurationVariable;
%ignore csoundCopyGlobalConfigurationVariables;
%ignore csoundSetGlobalConfigurationVariable;
%ignore csoundSetConfigurationVariable;
%ignore csoundParseGlobalConfigurationVariable;
%ignore csoundQueryGlobalConfigurationVariable;
%ignore csoundQueryConfigurationVariable;
%ignore csoundListGlobalConfigurationVariables;
%ignore csoundDeleteGlobalConfigurationVariable;
%ignore csoundDeleteConfigurationVariable;
%ignore csoundDeleteAllGlobalConfigurationVariables;
%ignore Csound::InitializeCscore;
%ignore Csound::GetHostData;
%ignore Csound::SetHostData;
%ignore Csound::GetInputBuffer;
%ignore Csound::GetOutputBuffer;
%ignore Csound::GetSpin;
%ignore Csound::GetSpout;
%ignore Csound::SetCscoreCallback;
%ignore Csound::ScoreSort;
%ignore Csound::ScoreExtract;
%ignore Csound::MessageV;
%ignore Csound::SetMessageCallback;
%ignore Csound::SetInputValueCallback;
%ignore Csound::SetOutputValueCallback;
%ignore Csound::SetExternalMidiInOpenCallback;
%ignore Csound::SetExternalMidiReadCallback;
%ignore Csound::SetExternalMidiInCloseCallback;
%ignore Csound::SetExternalMidiOutOpenCallback;
%ignore Csound::SetExternalMidiWriteCallback;
%ignore Csound::SetExternalMidiOutCloseCallback;
%ignore Csound::SetExternalMidiErrorStringCallback;
%ignore Csound::SetIsGraphable;
%ignore Csound::SetMakeGraphCallback;
%ignore Csound::SetDrawGraphCallback;
%ignore Csound::SetKillGraphCallback;
%ignore Csound::SetMakeXYinCallback;
%ignore Csound::SetReadXYinCallback;
%ignore Csound::SetKillXYinCallback;
%ignore Csound::SetExitGraphCallback;
%ignore Csound::AppendOpcode;
%ignore Csound::SetYieldCallback;
%ignore Csound::SetPlayopenCallback;
%ignore Csound::SetRtplayCallback;
%ignore Csound::SetRecopenCallback;
%ignore Csound::SetRtrecordCallback;
%ignore Csound::SetRtcloseCallback;
%ignore Csound::GetTable;
%ignore Csound::QueryGlobalVariableNoCheck;
%ignore Csound::DestroyGlobalVariable;
%ignore Csound::GetRtRecordUserData;
%ignore Csound::GetRtPlayUserData;
%ignore Csound::RegisterSenseEventCallback;
%ignore Csound::GetChannelPtr;
%ignore Csound::ChanIASet;
%ignore Csound::ChanOAGet;
%ignore Csound::CopyGlobalConfigurationVariable;
%ignore Csound::CopyGlobalConfigurationVariables;
%ignore Csound::SetConfigurationVariable;
%ignore Csound::QueryConfigurationVariable;
%ignore Csound::DeleteConfigurationVariable;

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

