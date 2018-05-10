/*
    FLTKKeyboardWidget.hpp:

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

#include "FLTKKeyboardWidget.hpp"

static void allNotesOff(Fl_Widget *widget, void * v) {
  IGN(widget);
    FLTKKeyboardWidget *win = (FLTKKeyboardWidget *)v;
    win->keyboard->allNotesOff();
}

static void channelChange(Fl_Widget *widget, void * v) {
    Fl_Spinner *spinner = (Fl_Spinner *)widget;
    FLTKKeyboardWidget *win = (FLTKKeyboardWidget *)v;

    win->lock();

    int channel = (int)spinner->value() - 1;

    win->keyboardMapping->setCurrentChannel(channel);

    win->bankChoice->value(win->keyboardMapping->getCurrentBank());

    win->setProgramNames();

    win->unlock();
}

static void bankChange(Fl_Widget *widget, void * v) {
    Fl_Choice *choice = (Fl_Choice *)widget;
    FLTKKeyboardWidget *win = (FLTKKeyboardWidget *)v;

    win->lock();

    win->keyboardMapping->setCurrentBank((int)choice->value());

    win->setProgramNames();

    win->unlock();
}

static void programChange(Fl_Widget *widget, void * v) {
    Fl_Choice *choice = (Fl_Choice *)widget;
    FLTKKeyboardWidget *win = (FLTKKeyboardWidget *)v;

    win->lock();

    win->keyboardMapping->setCurrentProgram((int)choice->value());

    win->unlock();
}

static void octaveChange(Fl_Widget *widget, void * v) {
    Fl_Choice *choice = (Fl_Choice *)widget;
    FLTKKeyboardWidget *win = (FLTKKeyboardWidget *)v;

    win->lock();

    win->keyboard->octave = (int)choice->value() + 1;

    win->unlock();
}

FLTKKeyboardWidget::FLTKKeyboardWidget(CSOUND *csound,
          const char *deviceMap,
          int X, int Y, int w, int h)
    : Fl_Group(X, Y, w, h)
{
    char octave[2];
    octave[1] = 0;

    this->csound = csound;
    this->mutex = csound->Create_Mutex(0);

    this->keyboardMapping = new KeyboardMapping(csound, deviceMap);

    this->begin();

    int baseX = this->x();
    int baseY = this->y();

    int row1 = baseY;
    int row2 = row1 + 20;
    int row3 = row2 + 20;

    int x1 = (int)(baseX + ((60 / 754.0) * w));
    int x2 = (int)(baseX + ((180 / 754.0) * w));
    int x3 = (int)(baseX + ((420 / 754.0) * w));
    int x4 = (int)(baseX + ((670 / 754.0) * w));

    int w1 = (int)((80 / 754.0) * w);
    int w2 = (int)((180 / 754.0) * w);
    int w3 = (int)((200 / 754.0) * w);
    int w4 = (int)((80 / 754.0) * w);


    //624, 120

    this->channelSpinner = new Fl_Spinner(x1, row1, w1, 20, "Channel");
    channelSpinner->maximum(16);
    channelSpinner->minimum(1);
    this->channelSpinner->callback((Fl_Callback*) channelChange, this);

    this->bankChoice = new Fl_Choice(x2, row1, w2, 20, "Bank");
    this->programChoice = new Fl_Choice(x3, row1, w3, 20, "Program");
    this->octaveChoice = new Fl_Choice(x4, row1, w4, 20, "Octave");

    bankChoice->clear();

    for(unsigned int i = 0; i < keyboardMapping->banks.size(); i++) {
            bankChoice->add(keyboardMapping->banks[i]->name);
    }

    bankChoice->value(0);

    setProgramNames();

    octaveChoice->clear();

    for(unsigned int i = 1; i < 8; i++) {
            octave[0] = i + 48;
            octaveChoice->add(octave);
    }

    octaveChoice->value(4);

    this->bankChoice->callback((Fl_Callback*)bankChange, this);
    this->programChoice->callback((Fl_Callback*)programChange, this);
    this->octaveChoice->callback((Fl_Callback*)octaveChange, this);

    this->allNotesOffButton = new Fl_Button(baseX, row2, w, 20, "All Notes Off");
    this->allNotesOffButton->callback((Fl_Callback*) allNotesOff, this);

    this->keyboard = new FLTKKeyboard(csound, NULL, baseX, row3, w, h - 40,
                                      "Keyboard");

    this->end();

}

FLTKKeyboardWidget::~FLTKKeyboardWidget() {
    if (mutex) {
        csound->DestroyMutex(mutex);
        mutex = (void*) 0;
    }
    delete keyboardMapping;
}

void FLTKKeyboardWidget::setProgramNames() {

    Bank* bank = keyboardMapping->banks[keyboardMapping->getCurrentBank()];

    programChoice->clear();

    for( vector<Program>::iterator iter = bank->programs.begin();
            iter != bank->programs.end(); iter++ ) {
            programChoice->add((*iter).name);
    }

    programChoice->value(bank->currentProgram);
}

int FLTKKeyboardWidget::handle(int event) {
    switch(event) {
        case FL_KEYDOWN:
                return this->keyboard->handle(event);
        case FL_KEYUP:
                return this->keyboard->handle(event);
//        case FL_DEACTIVATE:
//              this->keyboard->allNotesOff();
//              csound->Message(csound, "Deactivate\n");
//              return 1;
        default:
//        this->csound->Message(this->csound, "Keyboard event: %d\n", event);
            return Fl_Group::handle(event);
    }

}

void FLTKKeyboardWidget::lock() {
    if(mutex) {
        csound->LockMutex(mutex);
    }
}

void FLTKKeyboardWidget::unlock() {
    if(mutex) {
        csound->UnlockMutex(mutex);
    }
}
