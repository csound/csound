/*
    SliderBank.hpp:

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

#ifndef SLIDERBANK_HPP_
#define SLIDERBANK_HPP_

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Spinner.H>
#include "csdl.h"
#include "SliderData.hpp"

class WheelSlider: public Fl_Value_Slider
{
    int handle(int);
public:
    WheelSlider(int x, int y, int w, int h, const char *l=0) :
      Fl_Value_Slider (x,y,w,h,l) {}
};

class SliderBank : public Fl_Group
{
public:
    SliderBank(CSOUND *csound, int X, int Y, int W, int H);
    virtual ~SliderBank();

    CSOUND *csound;
    void * mutex;

    void setChannel(int channel);
    SliderData *getSliderData();

    void incrementSlider(int index, int n);

    void lock();
    void unlock();

    WheelSlider* sliders[10];
    Fl_Spinner* spinners[10];

private:
    int channel;
    SliderData sliderData[16];
};
#endif /*SLIDERBANK_HPP_*/
