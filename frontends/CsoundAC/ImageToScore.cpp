/*
* C S O U N D
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
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <cmath>
#include <complex>
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
            event[Event::INSTRUMENT] = (hue * score.scaleTargetRanges[Event::INSTRUMENT]) + score.scaleTargetMinima[Event::INSTRUMENT];
            event[Event::KEY] = int((((image->h() - y) / double(image->h())) * score.scaleTargetRanges[Event::KEY]) + score.scaleTargetMinima[Event::KEY] + 0.5);
            event[Event::VELOCITY] = (value * score.scaleTargetRanges[Event::VELOCITY]) + score.scaleTargetRanges[Event::VELOCITY];
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
            if(image) {
                size_t index = (y * image->w() * image->d()) + (x * image->d());
                const unsigned char *data = (const unsigned char *)image->data()[0];
                double red   = double(*(data + index + 0));
                double green = double(*(data + index + 1));
                double blue  = double(*(data + index + 2));
                rgbToHsv(red / 255.0, green / 255.0, blue / 255.0, hue, saturation, value);
                // System::debug("x=%5d y=%5d  r=%5.1f g=%5.1f b=%5.1f  h=%7.3f s=%7.3f v=%7.3f\n", x, y, red, green, blue, hue, saturation, value);
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

        /**
        * Generate a score with up to N voices,
        * based on the pixels in an image file.
        * The horizontal (x) dimension is time,
        * the vertical (y) dimension is pitch (MIDI key),
        * the value (brightness or v) of the pixel is loudness (MIDI velocity),
        * and the hue (color or h) of the pixel is instrument number.
        * The translation algorithm is:
        * for each column of pixels,
        * compare the value of the pixel in each row
        * to the value in the previous column
        * and the value in the next column.
        * If the current value is greater than the previous value or some minimum value V,
        * begin an event for the pitch of that row;
        * if the current value is less than V, end the event;
        * otherwise, continue the event.
        */
        void ImageToScore::generate()
        {
            System::inform("BEGAN ImageToScore::generate()...\n");
            if (image) {
                delete image;
                image = 0;
            }
            if (imageFilename.find(".jpg") != std::string::npos || imageFilename.find(".JPG") != std::string::npos ||
            imageFilename.find(".jpeg") != std::string::npos || imageFilename.find(".JPEG") != std::string::npos) {
                image = new Fl_JPEG_Image(imageFilename.c_str());
            } else if (imageFilename.find(".png") != std::string::npos || imageFilename.find(".PNG") != std::string::npos) {
                image = new Fl_PNG_Image(imageFilename.c_str());
            } else if (imageFilename.find(".gif") != std::string::npos || imageFilename.find(".GIF") != std::string::npos) {
                image = new Fl_GIF_Image(imageFilename.c_str());
            //} else if(imageFilename.find(".xpm") != std::string::npos || imageFilename.find(".XPM") != std::string::npos) {
            //    image = new Fl_XPM_Image(imageFilename.c_str());
            } else if(imageFilename.find(".pnm") != std::string::npos || imageFilename.find(".PNM") != std::string::npos) {
                image = new Fl_PNM_Image(imageFilename.c_str());
            } else if(imageFilename.find(".bmp") != std::string::npos || imageFilename.find(".BMP") != std::string::npos) {
                image = new Fl_BMP_Image(imageFilename.c_str());
            } else {
                System::error("Image file '%s' not found, or unsupported image format.\n", getImageFilename().c_str());
            }
            if (!image) {
                System::error("Failed to load mage.\n");
                System::inform("ENDED ImageToScore::generate().\n");
                return;
            }
            System::inform("Loaded image file \"%s\".\n", imageFilename.c_str());
            double x = 0.0;
            double y = 0.0;
            double w = image->w();
            double h = image->h();
            System::inform("Image width  = %d\n", int(w));
            System::inform("Image height = %d\n", int(h));
            System::inform("Image depth  = %d\n", image->d());
            System::inform("Image count  = %d\n", image->count());
            if (image->d() < 3) {
                System::error("Image must have depth 3 or greater.\n");
                System::inform("ENDED ImageToScore::generate().\n");
                return;
            }
            double saturation;
            double previousValue;
            double previousHue;
            double currentValue;
            double currentHue;
            double nextValue;
            double nextHue;
            Event startingEvent;
            Event endingEvent;
            // Index is int(velocity) * 1000 + channel.
            std::map<int, Event> startingEvents;
            // Index is int(key).
            std::map<int, Event> pendingEvents;
            for (x = 0.0; x < w; ++x) {
                System::debug("Column %5d\n", int(x));
                startingEvents.clear();
                // Detect starting events.
                for (y = 0; y < h; ++y) {
                    getPixel(size_t(x), size_t(y), currentHue, saturation, currentValue);
                    if (x == 0.0) {
                        previousHue = 0.0;
                        previousValue = 0.0;
                     } else {
                        getPixel(size_t(x - 1.0), size_t(y), previousHue, saturation, previousValue);
                    }
                    if (currentValue >= minimumValue && previousValue < minimumValue) {
                        translate(x, y, currentHue, currentValue, startingEvent);
                        System::debug("Starting event at  (x =%5d, y =%5d, value = %8.2f): %s\n", size_t(x), size_t(y), currentValue, startingEvent.toString().c_str());
                        int startingEventsIndex = int(startingEvent.getVelocityNumber() * 1000.0) + int(startingEvent.getChannel());
                        startingEvents[startingEventsIndex] = startingEvent;
                    }
                }
                // Insert starting events into the pending event list, in order of decreasing loudness,
                // until the pending event list has no more than maximumCount events.
                for (std::map<int, Event>::reverse_iterator startingEventsIterator = startingEvents.rbegin();
                    startingEventsIterator != startingEvents.rend();
                    ++startingEventsIterator) {
                    if (pendingEvents.size() < maximumVoiceCount) {
                        int pendingEventIndex = startingEventsIterator->second.getKeyNumber();
                        // Do not interrupt an existing event.
                        if (pendingEvents.find(pendingEventIndex) == pendingEvents.end()) {
                            System::debug("Pending event at   (x =%5d, y =%5d, value = %8.2f): %s\n", size_t(x), size_t(y), currentValue, startingEventsIterator->second.toString().c_str());
                            pendingEvents[pendingEventIndex] = startingEventsIterator->second;
                        }
                    } else {
                        break;
                    }
                }
                // Remove ending events from the pending event list and put them in the score.
                for (y = 0.0; y < h; ++y) {
                    getPixel(size_t(x), size_t(y), currentHue, saturation, currentValue);
                    if (x < (w - 1.0)) {
                        getPixel(size_t(x + 1.0), size_t(y), nextHue, saturation, nextValue);
                    } else {
                        nextValue = 0;
                        nextHue = 0;
                    }
                    if (currentValue >= minimumValue && nextValue < minimumValue) {
                        translate(x, y, nextHue, nextValue, endingEvent);
                        System::debug("Ending event at    (x =%5d, y =%5d, value = %8.2f): %s\n", size_t(x), size_t(y), currentValue, endingEvent.toString().c_str());
                        int pendingEventIndex = endingEvent.getKeyNumber();
                        if (pendingEvents.find(pendingEventIndex) != pendingEvents.end()) {
                            Event &startingEvent = pendingEvents[pendingEventIndex];
                            startingEvent.setDuration(endingEvent.getTime() - startingEvent.getTime());
                            if (startingEvent.getDuration() > 0.0) {
                                score.push_back(startingEvent);
                                System::inform("Inserting event at (x =%5d, y =%5d):                   %s\n", size_t(x), size_t(y), startingEvent.toString().c_str());
                                System::inform("Events pending=        %5d\n", pendingEvents.size());
                            }
                        }
                        pendingEvents.erase(pendingEventIndex);
                    }
                }
            }
            System::debug("Remaining events...\n");
            // Insert any remaining events.
            double endingTime = score.scaleTargetMinima[Event::TIME] + score.scaleTargetRanges[Event::TIME];
            for (std::map<int, Event>::iterator it = pendingEvents.begin(); it != pendingEvents.end(); ++it) {
                startingEvent = it->second;
                startingEvent[Event::DURATION] = endingTime - startingEvent[Event::TIME];
                score.push_back(startingEvent);
                System::debug("Ending:   %s\n", startingEvent.toString().c_str());
            }
            System::inform("ENDED ImageToScore::generate().\n");
        }
}
