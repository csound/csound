/*
    KeyboardMapping.cpp:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "KeyboardMapping.hpp"
#include <stdio.h>
#include <string.h>

#define LINELENGTH (300)

static char *my_fgets(char *s, int n, FILE *stream)
{
    char *a = s;
    if (n <= 1) return NULL;                 /* best of a bad deal */
    do {
      int ch = getc(stream);
      if (ch == EOF) {                       /* error or EOF       */
        if (s == a) return NULL;             /* no chars -> leave  */
        if (ferror(stream)) a = NULL;
        break; /* add NULL even if ferror(), spec says 'indeterminate' */
      }
      if (ch == '\n' || ch == '\r') {   /* end of line ? */
        *(s++) = '\n';                  /* convert */
        if (ch == '\r') {
          ch = getc(stream);
          if (ch != '\n')               /* Mac format */
            ungetc(ch, stream);
        }
        break;
      }
      *(s++) = ch;
    } while (--n > 1);
    *s = '\0';
    return a;
}


KeyboardMapping::KeyboardMapping(CSOUND *csound, const char *mapFileName)
{
    FILE  *mapFile;

    char *mapf = strdup(mapFileName);

    void *fd = csound->FileOpen2(csound, &mapFile, CSFILE_STD, mapf,
                                 (void *)"r", "INCDIR", CSFTYPE_OTHER_TEXT, 0);

    if (fd == NULL) {
        initializeDefaults(csound);
    } else {
      initializeMap(csound, mapFile);
      csound->FileClose(csound, fd);
    }



    currentChannel = 0;
    previousChannel = 0;


    for(int i = 0; i < 16; i++) {
        currentBank[i] = 0;
        previousBank[i] = -1;
    }
    previousProgram = -1;
}

KeyboardMapping::~KeyboardMapping()
{
    for(unsigned int i = 0; i < banks.size(); i++) {
        delete banks[i];
    }
}

void KeyboardMapping::initializeMap(CSOUND * csound, FILE *file) {
    char buff[LINELENGTH];
    char *p;

    /* 0 = general search
     * 1 = bank found but error in bank def, ignore all until next bank
     */
    int state = 0;

    Bank *bank = NULL;

    while (my_fgets(buff, LINELENGTH, file)!=NULL) {
      p = buff;
      while (*p == ' ' || *p == '\t') p++;

      if(*p == '#') { // line comment
        continue;
      } if(*p == '[') {
        p++;

        // cleanup current bank
        if(bank != NULL) {
          if(bank->programs.size() == 0) {
            bank->initializeGM();
          }
        }

        char *q = strstr(p, "=");
        char *z = strstr(p, "]");

        if(q == NULL || z == NULL) {
          state = 1;
          continue;
        }

        *q = '\0';
        *z = '\0';

        int bankNum = atoi(p) - 1;

        char *name = (char *)csound->Calloc(csound, strlen(q +1) + 1);
        memcpy(name, q + 1, strlen(q + 1) + 1);

        if(bankNum >= 0 && bankNum < 16384) {
          bank = new Bank(csound, name);
          bank->bankNum = bankNum;
          banks.push_back(bank);

          //bank->initializeGM();
          state = 0;
        } else {
          state = 1;
        }

      } else {
        if(state == 1 || bank == NULL) {
          continue;
        }

        char *q = strstr(p, "=");

        if(q == NULL) {
          continue;
        }

        *q = '\0';

        int programNum = atoi(p) - 1;

        char *name = (char *)csound->Calloc(csound, strlen(q +1) + 1);
        memcpy(name, q + 1, strlen(q + 1) + 1);

        if(programNum >= 0 && programNum < 128) {
          Program program(programNum, name);
          bank->programs.push_back(program);
        }
      }
    }

}

void KeyboardMapping::initializeDefaults(CSOUND *csound) {
    for(int i = 0; i < 128; i++) {

        char * name = (char *)csound->Calloc(csound, 9);

        sprintf(name, "Bank %d", i + 1);

        Bank *temp = new Bank(csound, name);
        temp->initializeGM();

        banks.push_back(temp);

    }
}

int KeyboardMapping::getCurrentChannel() {
    return currentChannel;
}

int KeyboardMapping::getCurrentBank() {
    return currentBank[currentChannel];
}

int KeyboardMapping::getPreviousBank() {
    return previousBank[currentChannel];
}

int KeyboardMapping::getCurrentProgram() {
    return banks[getCurrentBank()]->currentProgram;
}

int KeyboardMapping::getPreviousProgram() {
    return banks[getCurrentBank()]->previousProgram;
}


void KeyboardMapping::setCurrentChannel(int index) {
    currentChannel = index;
}

void KeyboardMapping::setCurrentBank(int index) {
    currentBank[currentChannel] = index;
}

void KeyboardMapping::setPreviousBank(int index) {
    previousBank[currentChannel] = index;
}

void KeyboardMapping::setCurrentProgram(int index) {
    banks[currentBank[currentChannel]]->currentProgram = index;
}

void KeyboardMapping::setPreviousProgram(int index) {
    banks[currentBank[currentChannel]]->previousProgram = index;
}


int KeyboardMapping::getCurrentBankMIDINumber() {
    return banks[getCurrentBank()]->bankNum;
}
