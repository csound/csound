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

#ifndef FLTKKEYBOARDWIDGET_HPP_
#define FLTKKEYBOARDWIDGET_HPP_

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Choice.H>
#include "FLTKKeyboard.hpp"
#include "KeyboardMapping.hpp"
#include "csdl.h"

class FLTKKeyboardWidget  : public Fl_Group {
public:
    FLTKKeyboardWidget(CSOUND * csound, const char *deviceMap,
                                        int x, int y, int w, int h);
    ~FLTKKeyboardWidget();

    FLTKKeyboard *keyboard;
    Fl_Button *allNotesOffButton;
    Fl_Spinner *channelSpinner;
    Fl_Choice *bankChoice;
    Fl_Choice *programChoice;
    Fl_Choice *octaveChoice;

    KeyboardMapping *keyboardMapping;

    int handle(int event);

    void lock();
    void unlock();

    void setSelectedBank();
    void setProgramNames();

private:
    CSOUND* csound;
    void * mutex;

};


#endif /*FLTKKEYBOARDWIDGET_HPP_*/
