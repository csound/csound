/*
   iOSCsound.cpp
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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA

*/

#include "iOSCsound.hpp"

extern "C" void aunit_setup(CSOUND *csound);
void iOSCsound::setAunitCallbacks() {
  initControls();
  aunit_setup(csound);
}

int iOSCsound::SetGlobalEnv(const char* name, const char* variable) {
    return csoundSetGlobalEnv(name, variable);
}

void iOSCsound::Pause(bool pause){
   int *p = ((int *)csoundQueryGlobalVariable(csound,"::paused::"));
   if(p) *p = pause ?  1  : 0;
   else csoundMessage(csound, "pause control not set up\n");
}



