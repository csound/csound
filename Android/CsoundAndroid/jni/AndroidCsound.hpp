/*
   AndroidCsound.hpp
   Android Csound class

   Copyright (C) 2011-2025 Steven Yi, Victor Lazzarini.

   This file is part of Csound.

   The Csound Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   Csound is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Csound; if not, write to the Free Software
   Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA

*/

#ifdef SWIG
%module csnd
#endif
#include "csound.hpp"
#include "csound_misc.h"
extern "C" long csoundGetKcounter(CSOUND *csound);
class PUBLIC AndroidCsound : public Csound {
  int asyncProcess;
  void initControls() {
    // set up pause controls
     if(csoundQueryGlobalVariable(csound,"::paused::") == NULL) {
      if (csoundCreateGlobalVariable(csound,"::paused::", sizeof(int)) == 0) {
        int *p = ((int *)csoundQueryGlobalVariable(csound,"::paused::"));
        *p = 0;
      }
    }
  }
 public:
  AndroidCsound(bool async=true) : Csound::Csound(){
    asyncProcess = async;
  }
  void setOpenSlCallbacks();
  void setAAudioCallbacks(); 
  int SetGlobalEnv(const char* name, const char* variable);
  unsigned long getStreamTime();
  void Pause(bool pause);
  long GetKcount(){ return csoundGetKcounter(csound); }
};
