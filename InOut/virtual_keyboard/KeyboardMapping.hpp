/*
    KeyboardMapping.hpp:

    Copyright (C) 2006 Steven Yi

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef KEYBOARDMAPPING_HPP_
#define KEYBOARDMAPPING_HPP_

#include "csdl.h"
#include "Bank.hpp"
#include <stdio.h>
#include <string.h>
#include <vector>

using namespace std;

class KeyboardMapping {
public:
        KeyboardMapping(CSOUND *csound, const char *mapFileName);
        ~KeyboardMapping();
        vector<Bank*> banks;

        int getCurrentChannel();
        int getCurrentBank();
        int getPreviousBank();
        int getCurrentProgram();
        int getPreviousProgram();

        void setCurrentChannel(int index);
        void setCurrentBank(int index);
        void setPreviousBank(int index);
        void setCurrentProgram(int index);
        void setPreviousProgram(int index);

        int getCurrentBankMIDINumber();

        int previousChannel;
        int previousProgram;

private:
        void initializeDefaults(CSOUND *);
        void initializeMap(CSOUND *, FILE *);

        int currentChannel;
        int previousBank[16];
        int currentBank[16];
};

#endif /*KEYBOARDMAPPING_HPP_*/
