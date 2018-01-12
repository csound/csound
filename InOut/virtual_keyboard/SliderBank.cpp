/*
    SliderBank.cpp:

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

#include "SliderBank.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>

int WheelSlider::handle(int event) {
    int res = Fl_Value_Slider::handle(event);
    if (event == FL_MOUSEWHEEL) {
        int dy = Fl::event_dy();
        value(clamp(round(increment(value(), -dy))));
        return 1;
    }
    return res;
}

static void spinnerCallback(Fl_Widget *widget, void *v) {
    Fl_Spinner *spinner = (Fl_Spinner *)widget;
    SliderBank *sliderBank = (SliderBank *)v;

    for(int i = 0; i < 10; i++) {
      if(sliderBank->spinners[i] == spinner) {
        sliderBank->lock();
        SliderData* data = sliderBank->getSliderData();
        data->controllerNumber[i] = (int)spinner->value();
        sliderBank->unlock();
      }
    }
}

static void sliderCallback(Fl_Widget *widget, void *v) {
    Fl_Slider *slider = (Fl_Slider *)widget;
    SliderBank *sliderBank = (SliderBank *)v;

    for(int i = 0; i < 10; i++) {
      if(sliderBank->sliders[i] == slider) {
        sliderBank->lock();
        SliderData* data = sliderBank->getSliderData();
        data->controllerValue[i] = (int)slider->value();
        sliderBank->unlock();
      }
    }
}

SliderBank::SliderBank(CSOUND *csound,
                       int X, int Y, int W, int H)
  : Fl_Group(X, Y, W, H)
{

    this->csound = csound;
    this->mutex = csound->Create_Mutex(0);

    this->channel = 0;

    this->begin();

    for(int i = 0; i < 10; i++) {
      int x, y;

      if(i < 5) {
        x = 10;
        y = 10 + (i * 25);
      } else {
        x = 382;
        y = 10 + ((i - 5) * 25);
      }

      Fl_Spinner *spinner = new Fl_Spinner(x, y, 60, 20);
      spinners[i] = spinner;
      spinner->maximum(127);
      spinner->minimum(0);
      spinner->step(1);
      spinner->value(i + 1);
      spinner->callback((Fl_Callback*)spinnerCallback, this);


      WheelSlider *slider = new WheelSlider(x + 70, y, 292, 20);
      sliders[i] = slider;
      slider->type(FL_HOR_SLIDER);
      slider->maximum(127);
      slider->minimum(0);
      slider->step(1);
      slider->value(0);
      slider->callback((Fl_Callback*)sliderCallback, this);
    }


    this->end();

}

SliderBank::~SliderBank()
{
    if (mutex) {
      csound->DestroyMutex(mutex);
      mutex = (void*) 0;
    }
}

void SliderBank::setChannel(int channel) {
    this->channel = channel;

    SliderData data = sliderData[channel];

    lock();
    for(int i = 0; i < 10; i++) {
      spinners[i]->value(data.controllerNumber[i]);
      sliders[i]->value(data.controllerValue[i]);
    }
    unlock();
}

SliderData * SliderBank::getSliderData() {
    return &sliderData[channel];
}

void SliderBank::incrementSlider(int index, int n) {
  Fl_Slider* slider = sliders[index];
  slider->value(slider->clamp(slider->round(slider->increment(slider->value(), n))));
}

void SliderBank::lock() {
    if(mutex) {
      csound->LockMutex(mutex);
    }
}

void SliderBank::unlock() {
    if(mutex) {
      csound->UnlockMutex(mutex);
    }
}
