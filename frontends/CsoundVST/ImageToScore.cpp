/**
* C S O U N D   V S T
*
* A VST plugin version of Csound, with Python scripting.
*
* L I C E N S E
*
* This software is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "CppSound.hpp"
#include "ImageToScore.hpp"
#include "System.hpp"
#include <FL/Fl_Image.H>
#include <FL/Fl_PNM_Image.H>
#include <FL/Fl_XPM_Image.H>
#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_BMP_Image.H>
#include <cmath>
#include <set>
#include <functional>

namespace csound
{

        ImageToScore::ImageToScore(void) : image(0), maximumVoiceCount(4), minimumValue(0)
        {
        }

        ImageToScore::~ImageToScore(void)
        {
        }

        void ImageToScore::setImageFilename(std::string imageFilename)
        {
                this->imageFilename = imageFilename;
        }

        std::string ImageToScore::getImageFilename() const
        {
                return imageFilename;
        }

        void ImageToScore::translate(double x, double y, double hue, double value, Event &event) const
        {
                event[Event::STATUS] = 144;
                event[Event::TIME] = ((x / double(image->w())) * score.scaleTargetRanges[Event::TIME]) + score.scaleTargetMinima[Event::TIME];
                event[Event::INSTRUMENT] = int(hue * score.scaleTargetRanges[Event::INSTRUMENT] + score.scaleTargetMinima[Event::INSTRUMENT] + 0.5);
                event[Event::KEY] = int((((image->h() - y) / double(image->h())) * score.scaleTargetRanges[Event::KEY]) + score.scaleTargetMinima[Event::KEY] + 0.5);
                event[Event::VELOCITY] = value * score.scaleTargetRanges[Event::VELOCITY] + score.scaleTargetRanges[Event::VELOCITY];
                // static char buffer[0x100];
                // sprintf(buffer, "Translate: x %d y %d  %s\n", int(x), int(y), event.toString().c_str());
        }

        void ImageToScore::rgbToHsv(double r, double g, double b, double &h, double &s, double &v)
        {
                double maxc = std::max(std::max(r, g), b);
                double minc = std::min(std::min(r, g), b);
                v = maxc;
                if (minc == maxc)
                {
                        h = 0;
                        s = 0;
                        return;
                }
                s = (maxc-minc) / maxc;
                double rc = (maxc-r) / (maxc-minc);
                double gc = (maxc-g) / (maxc-minc);
                double bc = (maxc-b) / (maxc-minc);
                if (r == maxc)
                {
                        h = bc-gc;
                }
                else if( g == maxc)
                {
                        h = 2.0+rc-bc;
                }
                else
                {
                        h = 4.0+gc-rc;
                }
                h = std::fmod(h/6.0, 1.0);
        }

        void ImageToScore::getPixel(size_t x, size_t y, double &hue, double &saturation, double &value) const
        {
                if(image)
                {
                        unsigned char *data = *(unsigned char **)image->data();
                        unsigned char *line = data + (y * image->w() * image->d());
                        unsigned char *pixel = line + (x * image->d());
                        double red   = pixel[0];
                        double green = pixel[1];
                        double blue  = pixel[2];
                        rgbToHsv(red / 255.0, green / 255.0, blue / 255.0, hue, saturation, value);
                        // System::debug("x %d y %d  pixel %d  r %f g %f b %f  h %f s %f v %f\n", x, y, pixel, red, green, blue, hue, saturation, value);
                }
        }

        void ImageToScore::setMaximumVoiceCount(size_t maximumVoiceCount)
        {
                this->maximumVoiceCount = maximumVoiceCount;
        }

        size_t ImageToScore::getMaximumVoiceCount() const
        {
                return maximumVoiceCount;
        }

        void ImageToScore::setMinimumValue(double minimumValue)
        {
                this->minimumValue = minimumValue;
        }

        double ImageToScore::getMinimumValue() const
        {
                return minimumValue;
        }

        struct EventLess
        {
                bool operator()(const csound::Event& a, const csound::Event& b) const
                {
                        return a[Event::VELOCITY] < b[Event::VELOCITY];
                }
        };

        void ImageToScore::generate()
        {
                if(image)
                {
                        delete image;
                        image = 0;
                }
                if(imageFilename.find(".gif") != std::string::npos || imageFilename.find(".GIF") != std::string::npos)
                {
                        image = new Fl_GIF_Image(imageFilename.c_str());
                }
                else if(imageFilename.find(".xpm") != std::string::npos || imageFilename.find(".XPM") != std::string::npos)
                {
                        image = new Fl_XPM_Image(imageFilename.c_str());
                }
                else if(imageFilename.find(".pnm") != std::string::npos || imageFilename.find(".PNM") != std::string::npos)
                {
                        image = new Fl_PNM_Image(imageFilename.c_str());
                }
                else if(imageFilename.find(".bmp") != std::string::npos || imageFilename.find(".BMP") != std::string::npos)
                {
                        image = new Fl_BMP_Image(imageFilename.c_str());
                }
                else
                {
                        System::error("Image file '%s' not found, or unsupported image format.\n", getImageFilename().c_str());
                }
                if(!image)
                {
                        return;
                }
                double x;
                double y;
                double w = image->w();
                double h = image->h();
                System::inform("Image width  = %d\n", int(w));
                System::inform("Image height = %d\n", int(h));
                System::inform("Image depth  = %d\n", image->d());
                System::inform("Image count  = %d\n", image->count());
                double saturation;
                double hue;
                double value;
                double previousHue;
                double previousValue;
                double nextHue;
                double nextValue;
                Event startingEvent;
                Event endingEvent;
                std::map<size_t, Event> pendingEvents;
                std::multiset<Event, EventLess> startingEvents;
                for(x = 0; x < w; ++x)
                {
                        // Insert starting events into the starting event list,
                        // but only if they are not already there.
                        startingEvents.erase(startingEvents.begin(), startingEvents.end());
                        for(y = 0; y < h; ++y)
                        {
                                getPixel(size_t(x), size_t(y), hue, saturation, value);
                                if(x == 0)
                                {
                                        if(value > minimumValue)
                                        {
                                                previousHue = 0;
                                                previousValue = 0;
                                        }
                                        else
                                        {
                                                previousHue = hue;
                                                previousValue = value;
                                        }
                                }
                                else
                                {
                                        getPixel(size_t(x - 1), size_t(y), previousHue, saturation, previousValue);
                                }
                                // Is the event starting?
                                if(previousValue <= minimumValue && value > minimumValue)
                                {
                                        translate(x, y, hue, value, startingEvent);
                                        startingEvents.insert(startingEvent);
                                }
                        }
                        // Insert starting events into the pending event list, in order of decreasing loudness,
                        // until the pending event list has no more than maximumCount events.
                        for(std::multiset<Event, EventLess>::iterator nt = startingEvents.begin(); nt != startingEvents.end(); ++nt)
                        {
                                if(pendingEvents.size() < maximumVoiceCount)
                                {
                                        startingEvent = *nt;
                                        size_t eventIndex = size_t(startingEvent[Event::INSTRUMENT] * 1000 + startingEvent[Event::KEY]);
                                        if(pendingEvents.find(eventIndex) == pendingEvents.end())
                                        {
                                                pendingEvents[eventIndex] = startingEvent;
                                        }
                                }
                                else
                                {
                                        break;
                                }
                        }
                        // Remove ending events from the pending event list,
                        // and put them in the score.
                        for(y = 0; y < h; ++y)
                        {
                                getPixel(size_t(x), size_t(y), hue, saturation, value);
                                if(x < (w - 1))
                                {
                                        getPixel(size_t(x + 1), size_t(y), nextHue, saturation, nextValue);
                                }
                                else
                                {
                                        nextHue = 0;
                                        nextValue = 0;
                                }
                                //      Is the event ending?
                                if(value > minimumValue && nextValue <= minimumValue)
                                {
                                        translate(x, y, hue, value, endingEvent);
                                        size_t eventIndex = size_t(endingEvent[Event::INSTRUMENT] * 1000 + endingEvent[Event::KEY]);
                                        if(pendingEvents.find(eventIndex) != pendingEvents.end())
                                        {
                                                startingEvent = pendingEvents[eventIndex];
                                                startingEvent[Event::DURATION] = endingEvent[Event::TIME] - startingEvent[Event::TIME];
                                                score.push_back(startingEvent);
                                                System::debug("Inserting: %s\n", startingEvent.toString().c_str());
                                                pendingEvents.erase(eventIndex);
                                        }
                                }
                        }
                }
                System::debug("Remaining events...\n");
                // Insert any remaining events.
                double endingTime = score.scaleTargetMinima[Event::TIME] + score.scaleTargetRanges[Event::TIME];
                for(std::map<size_t, Event>::iterator it = pendingEvents.begin(); it != pendingEvents.end(); ++it)
                {
                        startingEvent = it->second;
                        size_t eventIndex = size_t(startingEvent[Event::INSTRUMENT] * 1000 + startingEvent[Event::KEY]);
                        startingEvent[Event::DURATION] = endingTime - startingEvent[Event::TIME];
                        score.push_back(startingEvent);
                        System::debug("Ending:   %d  %s\n", eventIndex, startingEvent.toString().c_str());
                }
        }
};
