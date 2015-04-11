/*
    virtual_keyboard.cpp:

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

#include "csdl.h"
#include "csGblMtx.h"
#include "midiops.h"
#include "winFLTK.h"
#include "FLTKKeyboardWindow.hpp"
#include "FLTKKeyboardWidget.hpp"
#include "KeyboardMapping.hpp"
#include "SliderData.hpp"
#include <map>

static std::map<CSOUND *, FLTKKeyboardWidget *> keyboardWidgets;

static FLTKKeyboardWindow *createWindow(CSOUND *csound, const char *dev) {
    return new FLTKKeyboardWindow(csound, dev,
                                  754, 270, "Csound Virtual Keyboard");
}

static FLTKKeyboardWidget *createWidget(CSOUND *csound, const char *dev,
        int w, int h, int x, int y) {
    return new FLTKKeyboardWidget(csound, dev, x, y, w, h);
}

static void deleteWindow(CSOUND *csound, FLTKKeyboardWindow * keyWin) {
    if(keyWin == NULL) {
      return;
    }
    Fl_lock(csound);

    keyWin->hide();
    delete keyWin;

    Fl_awake(csound);
    Fl_wait(csound, 0.0);
    Fl_unlock(csound);
}

extern "C"
{

typedef struct {
    OPDS    h;
    STRINGDAT   *mapFileName;
    MYFLT *iwidth, *iheight, *ix, *iy;
} FLVKEYBD;


static int OpenMidiInDevice_(CSOUND *csound, void **userData, const char *dev)
{

    if(keyboardWidgets.find(csound) != keyboardWidgets.end()) {
        return 0;
    }

    FLTKKeyboardWindow *keyboard = createWindow(csound, dev);
    *userData = (void *)keyboard;

    Fl_lock(csound);
    keyboard->show();
    Fl_wait(csound, 0.0);
    Fl_unlock(csound);

    /* report success */
    return 0;
}

static int OpenMidiOutDevice_(CSOUND *csound, void **userData, const char *dev)
{
    /* report success */
    return 0;
}

static int ReadMidiWindow(CSOUND *csound, FLTKKeyboardWindow *keyWin,
                         unsigned char *mbuf, int nbytes)
{
    int i;
    Fl_lock(csound);
    Fl_awake(csound);
    Fl_wait(csound, 0.0);
    Fl_unlock(csound);

    if(!keyWin->visible()) {
        return 0;
    }

    int count = 0;


    keyWin->lock();

    KeyboardMapping* keyboardMapping = keyWin->keyboardMapping;

    int channel = keyboardMapping->getCurrentChannel();

    if(keyboardMapping->getCurrentBank() !=
       keyboardMapping->getPreviousBank()) {

      int bankNum = keyboardMapping->getCurrentBankMIDINumber();
      unsigned char msb = (unsigned char)(bankNum >> 7) &
        (unsigned char)0x7F;
      unsigned char lsb = (unsigned char)bankNum &
        (unsigned char)0x7F;

      *mbuf++ = (unsigned char)(0xB0 + channel); // MSB
      *mbuf++ = (unsigned char)0;
      *mbuf++ = msb;

      *mbuf++ = (unsigned char)(0xB0 + channel); // LSB
      *mbuf++ = (unsigned char)32;
      *mbuf++ = lsb;

      *mbuf++ = (unsigned char)(0xC0 + channel); // Program Change
      *mbuf++ = (unsigned char)keyboardMapping->getCurrentProgram();

      count += 8;

      keyboardMapping->setPreviousBank(keyboardMapping->getCurrentBank());
      keyboardMapping->setPreviousProgram(keyboardMapping->getCurrentProgram());

    } else if(keyboardMapping->getCurrentProgram() !=
              keyboardMapping->getPreviousProgram()) {

      *mbuf++ = (unsigned char)(0xC0 + channel); // Program Change
      *mbuf++ = (unsigned char)keyboardMapping->getCurrentProgram();

      count += 2;

      keyboardMapping->setPreviousProgram(keyboardMapping->getCurrentProgram());
    }

    keyWin->sliderBank->lock();
    SliderData *sliderData = keyWin->sliderBank->getSliderData();

    for(i = 0; i < 10; i++) {
      if(sliderData->controllerNumber[i] !=
         sliderData->previousControllerNumber[i]) {

        *mbuf++ = (unsigned char)(0xB0 + channel);
        *mbuf++ = (unsigned char)sliderData->controllerNumber[i];
        *mbuf++ = (unsigned char)sliderData->controllerValue[i];

        count += 3;

        sliderData->previousControllerNumber[i] =
          sliderData->controllerNumber[i];

        sliderData->previousControllerValue[i] =
          sliderData->controllerValue[i];


      } else if(sliderData->controllerValue[i] !=
                sliderData->previousControllerValue[i]) {

        *mbuf++ = (unsigned char)(0xB0 + channel);
        *mbuf++ = (unsigned char)sliderData->controllerNumber[i];
        *mbuf++ = (unsigned char)sliderData->controllerValue[i];

        count += 3;

        sliderData->previousControllerValue[i] =
          sliderData->controllerValue[i];
      }

    }

    keyWin->sliderBank->unlock();
    keyWin->unlock();


    keyWin->keyboard->lock();

    int *changedKeyStates = keyWin->keyboard->changedKeyStates;
    int *keyStates = keyWin->keyboard->keyStates;



    for(i = 0; i < 88; i++) {
        if(keyStates[i] == -1) {
                *mbuf++ = (unsigned char)0x90 + channel;
            *mbuf++ = (unsigned char)i + 21;
            *mbuf++ = (unsigned char)0;

            count += 3;
            keyStates[i] = 0;
        } else if(changedKeyStates[i] != keyStates[i]) {

                if(keyStates[i] == 1) {
                    *mbuf++ = (unsigned char)0x90 + channel;
                    *mbuf++ = (unsigned char)i + 21;
                    *mbuf++ = (unsigned char)127;

                    count += 3;

                } else {
                    *mbuf++ = (unsigned char)0x90 + channel;
                    *mbuf++ = (unsigned char)i + 21;
                    *mbuf++ = (unsigned char)0;

                    count += 3;
                }
        }

                changedKeyStates[i] = keyStates[i];
    }

    if(keyWin->keyboard->aNotesOff == 1) {
        keyWin->keyboard->aNotesOff = 0;
        *mbuf++ = (unsigned char)0xB0;
        *mbuf++ = (unsigned char)123;
        *mbuf++ = (unsigned char)0;

        count += 3;
    }

        keyWin->keyboard->unlock();

    return count;
}

static int ReadMidiWidget(CSOUND *csound, FLTKKeyboardWidget *widget,
                         unsigned char *mbuf, int nbytes)
{

    int i;

    if(!widget->visible()) {
        return 0;
    }

    int count = 0;

    widget->lock();

    KeyboardMapping* keyboardMapping = widget->keyboardMapping;

    int channel = keyboardMapping->getCurrentChannel();

    if(keyboardMapping->getCurrentBank() !=
       keyboardMapping->getPreviousBank()) {

      int bankNum = keyboardMapping->getCurrentBankMIDINumber();
      unsigned char msb = (unsigned char)(bankNum >> 7) &
        (unsigned char)0x7F;
      unsigned char lsb = (unsigned char)bankNum &
        (unsigned char)0x7F;

      *mbuf++ = (unsigned char)(0xB0 + channel); // MSB
      *mbuf++ = (unsigned char)0;
      *mbuf++ = msb;

      *mbuf++ = (unsigned char)(0xB0 + channel); // LSB
      *mbuf++ = (unsigned char)32;
      *mbuf++ = lsb;

      *mbuf++ = (unsigned char)(0xC0 + channel); // Program Change
      *mbuf++ = (unsigned char)keyboardMapping->getCurrentProgram();

      count += 8;

      keyboardMapping->setPreviousBank(keyboardMapping->getCurrentBank());
      keyboardMapping->setPreviousProgram(keyboardMapping->getCurrentProgram());

    } else if(keyboardMapping->getCurrentProgram() !=
              keyboardMapping->getPreviousProgram()) {

      *mbuf++ = (unsigned char)(0xC0 + channel); // Program Change
      *mbuf++ = (unsigned char)keyboardMapping->getCurrentProgram();

      count += 2;

      keyboardMapping->setPreviousProgram(keyboardMapping->getCurrentProgram());
    }

    widget->unlock();

    widget->keyboard->lock();

    int *changedKeyStates = widget->keyboard->changedKeyStates;
    int *keyStates = widget->keyboard->keyStates;



    for(i = 0; i < 88; i++) {
        if(keyStates[i] == -1) {
                *mbuf++ = (unsigned char)0x90 + channel;
            *mbuf++ = (unsigned char)i + 21;
            *mbuf++ = (unsigned char)0;

            count += 3;
            keyStates[i] = 0;
        } else if(changedKeyStates[i] != keyStates[i]) {

            if(keyStates[i] == 1) {
                *mbuf++ = (unsigned char)0x90 + channel;
                *mbuf++ = (unsigned char)i + 21;
                *mbuf++ = (unsigned char)127;

                count += 3;

            } else {
                *mbuf++ = (unsigned char)0x90 + channel;
                *mbuf++ = (unsigned char)i + 21;
                *mbuf++ = (unsigned char)0;

                count += 3;
            }
        }

        changedKeyStates[i] = keyStates[i];
    }

    if(widget->keyboard->aNotesOff == 1) {
        widget->keyboard->aNotesOff = 0;
        *mbuf++ = (unsigned char)0xB0;
        *mbuf++ = (unsigned char)123;
        *mbuf++ = (unsigned char)0;

        count += 3;
    }

    widget->keyboard->unlock();

    return count;
}

static int ReadMidiData_(CSOUND *csound, void *userData,
                         unsigned char *mbuf, int nbytes)
{

    if(keyboardWidgets.find(csound) == keyboardWidgets.end()) {
        return ReadMidiWindow(csound,
                (FLTKKeyboardWindow *)userData, mbuf, nbytes);
    }

    return ReadMidiWidget(csound,
        keyboardWidgets[csound], mbuf, nbytes);
}

static int WriteMidiData_(CSOUND *csound, void *userData,
                          const unsigned char *mbuf, int nbytes)
{
    /* return the number of bytes written */
    return 0;
}

static int CloseMidiInDevice_(CSOUND *csound, void *userData)
{

    deleteWindow(csound, (FLTKKeyboardWindow *)userData);

    return 0;
}

static int CloseMidiOutDevice_(CSOUND *csound, void *userData)
{
    return 0;
}

/* FLvkeybd Opcode */

static int fl_vkeybd(CSOUND *csound, FLVKEYBD *p) {
    if(keyboardWidgets.find(csound) != keyboardWidgets.end()) {
        csound->ErrorMsg(csound,
            "FLvkeybd may only be used once in a project.\n");
        return -1;
    }

    char *mapFileName = new char[MAXNAME];

    strncpy(mapFileName, p->mapFileName->data, MAXNAME-1);

    FLTKKeyboardWidget *widget = createWidget(csound, mapFileName,
        (int)*p->iwidth, (int)*p->iheight, (int)*p->ix, (int)*p->iy);

    keyboardWidgets[csound] = widget;

    delete[] mapFileName;
    return OK;
}

#define S(x)    sizeof(x)

const OENTRY widgetOpcodes_[] = {
  { (char*)"FLvkeybd", S(FLVKEYBD), 0, 1,  (char*)"", (char*)"Siiii",
         (SUBR) fl_vkeybd, (SUBR) NULL, (SUBR) NULL },
  { NULL, 0, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL,(SUBR) NULL }
};


/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    /* nothing to do, report success */
    csound->Message(csound, "virtual_keyboard real time MIDI plugin for Csound\n");
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *drv;
    const OENTRY  *ep = &(widgetOpcodes_[0]);

    if (csound->QueryGlobalVariable(csound,
                                    "FLTK_Flags") == (void*) 0) {
      if (csound->CreateGlobalVariable(csound,
                                       "FLTK_Flags", sizeof(int)) != 0)
        csound->Die(csound, "%s",
                    Str("virtual_keyboard.cpp: error allocating FLTK flags"));
    }

    for ( ; ep->opname != NULL; ep++) {
        if (csound->AppendOpcode(csound, ep->opname,
                                 (int)ep->dsblksiz, (int)ep->flags,
                                 (int)ep->thread, ep->outypes, ep->intypes,
                                 ep->iopadr, ep->kopadr, ep->aopadr) != 0) {
            csound->ErrorMsg(csound, Str("Error registering opcode '%s'"),
                                   ep->opname);
            return -1;
        }
    }

    drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "virtual") == 0))
      return 0;
    csound->Message(csound, "rtmidi: virtual_keyboard module enabled\n");
    csound->SetExternalMidiInOpenCallback(csound, OpenMidiInDevice_);
    csound->SetExternalMidiReadCallback(csound, ReadMidiData_);
    csound->SetExternalMidiInCloseCallback(csound, CloseMidiInDevice_);
    csound->SetExternalMidiOutOpenCallback(csound, OpenMidiOutDevice_);
    csound->SetExternalMidiWriteCallback(csound, WriteMidiData_);
    csound->SetExternalMidiOutCloseCallback(csound, CloseMidiOutDevice_);

    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    /* does not depend on MYFLT type */
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8));
}

} // END EXTERN C

