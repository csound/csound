/*
   iOSCsound.hpp
   iOS Csound class

   Copyright (C) 2025 Victor Lazzarini.

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

#include "csound.hpp"
#include "csound_misc.h"

class PUBLIC iOSCsound : public Csound {
  void initControls() {
    // set up pause controls
     if(csoundQueryGlobalVariable(csound,"::paused::") == NULL) {
      if (csoundCreateGlobalVariable(csound,"::paused::", sizeof(int)) == 0) {
        int *p = ((int *)csoundQueryGlobalVariable(csound,"::paused::"));
        *p = 0;
      }
    }
  }
  void setAunitCallbacks();
 public:
  iOSCsound() : Csound::Csound(){
    setAunitCallbacks();
  }
  int SetGlobalEnv(const char* name, const char* variable);
  void Pause(bool pause);
  long GetKcount(){ return csoundGetKcounter(csound); }
};
