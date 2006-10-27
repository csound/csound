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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef FLTKKEYBOARD_HPP_
#define FLTKKEYBOARD_HPP_

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

class FLTKKeyboard : public Fl_Widget {
public:
  FLTKKeyboard(int X, int Y, int W, int H, const char *L);
  int handle(int event);
  void draw();
  int keyStates[88];
  int whiteKeys[7];
private:

  int getMIDIKey(int x, int y);
  int lastMidiKey;
  int isWhiteKey(int key);
  int getMidiValForWhiteKey(int whiteKeyNum);

  static const int blackKeyHeight = 50;
  static const int whiteKeyHeight = 80;
  static const int blackKeyWidth = 10;
  static const int whiteKeyWidth = 12;
  static const int rightKeyBound = whiteKeyWidth - (blackKeyWidth / 2);
  static const int leftKeyBound = (blackKeyWidth / 2);
};

#endif /*FLTKKEYBOARD_HPP_*/
