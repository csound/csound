#ifndef KEYBOARDMAPPING_HPP_
#define KEYBOARDMAPPING_HPP_

#include "csdl.h"
#include "Bank.hpp"
#include <stdio.h>
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
