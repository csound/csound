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
#include "oload.h"
#include "winFLTK.h"
#include "FLTKKeyboardWindow.hpp"


static FLTKKeyboardWindow *createWindow(CSOUND *csound) {
       return new FLTKKeyboardWindow(csound, 624, 80, "Csound Virtual Keyboard");
}

static void deleteWindow(FLTKKeyboardWindow * keyWin) {
    delete keyWin;
}

extern "C"
{

static int OpenMidiInDevice_(CSOUND *csound, void **userData, const char *dev)
{
    FLTKKeyboardWindow *keyboard = createWindow(csound);
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

static int ReadMidiData_(CSOUND *csound, void *userData,
                         unsigned char *mbuf, int nbytes)
{

    FLTKKeyboardWindow *keyWin = (FLTKKeyboardWindow *)userData;

    if(!keyWin->visible()) {
        return 0;
    }

    Fl_lock(csound);
	Fl_awake(csound);
	Fl_wait(csound, 0.0);
	Fl_unlock(csound);


	keyWin->keyboard->lock();
 	int *keyStates = keyWin->keyboard->keyStates;

    int count = 0;

    for(int i = 0; i < 88; i++) {
        if(keyStates[i] == 1) {
            //csound->Message(csound, "key down: %d\n", i);
            keyStates[i] = 0;

            *mbuf++ = (unsigned char)0x90;
            *mbuf++ = (unsigned char)i + 21;
            *mbuf++ = (unsigned char)127;

            count += 3;

        } else if (keyStates[i] == 2) {
            keyStates[i] = 0;
            //csound->Message(csound, "key up: %d\n", i);

            *mbuf++ = (unsigned char)0x90;
            *mbuf++ = (unsigned char)i + 21;
            *mbuf++ = (unsigned char)0;

            count += 3;
        }

    }
	keyWin->keyboard->unlock();

    return count;
}

static int WriteMidiData_(CSOUND *csound, void *userData,
                          const unsigned char *mbuf, int nbytes)
{
    /* return the number of bytes written */
    return 0;
}

static int CloseMidiInDevice_(CSOUND *csound, void *userData)
{

    deleteWindow((FLTKKeyboardWindow *)userData);

    return 0;
}

static int CloseMidiOutDevice_(CSOUND *csound, void *userData)
{
    return 0;
}

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

    if (csound->QueryGlobalVariable(csound,
                                    "FLTK_Flags") == (void*) 0) {
      if (csound->CreateGlobalVariable(csound,
                                       "FLTK_Flags", sizeof(int)) != 0)
        csound->Die(csound,
                    Str("virtual_keyboard.cpp: error allocating FLTK flags"));
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
