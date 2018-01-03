/*
    FLTKKeyboard.hpp:

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

#ifndef FLTKKEYBOARD_HPP_
#define FLTKKEYBOARD_HPP_

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include "csdl.h"
#include "SliderBank.hpp"

class FLTKKeyboard : public Fl_Widget {
public:
  FLTKKeyboard(CSOUND *csound, SliderBank *sliderBank,
               int X, int Y, int W, int H, const char *L);
  ~FLTKKeyboard();
  int handle(int event);
  void draw();
  int keyStates[88];
  int changedKeyStates[88];
  int whiteKeys[7];
  void lock();
  void unlock();
  void allNotesOff();
  int aNotesOff;
  int octave;
private:
  int getMIDIKey(int x, int y);
  int lastMidiKey;
  int isWhiteKey(int key);
  int getMidiValForWhiteKey(int whiteKeyNum);
  void handleKey(int key, int value);
  void handleControl(int key);

  CSOUND *csound;
  void * mutex;
  SliderBank *sliderBank;
};

#endif /*FLTKKEYBOARD_HPP_*/
