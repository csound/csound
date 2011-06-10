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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef SLIDERBANK_HPP_
#define SLIDERBANK_HPP_

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Spinner.H>
#include "csdl.h"
#include "SliderData.hpp"

class SliderBank : public Fl_Group
{
public:
    SliderBank(CSOUND *csound, int X, int Y, int W, int H);
    virtual ~SliderBank();

    CSOUND *csound;
    void * mutex;

    void setChannel(int channel);
    SliderData *getSliderData();

    void lock();
    void unlock();

    Fl_Value_Slider* sliders[10];
    Fl_Spinner* spinners[10];

private:
    int channel;
    SliderData sliderData[16];
};

#endif /*SLIDERBANK_HPP_*/
