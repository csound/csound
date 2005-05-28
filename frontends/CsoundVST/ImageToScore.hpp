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
#ifndef IMAGETOSCORE_H
#define IMAGETOSCORE_H

#ifdef SWIG
%module CsoundVST
%{
#include "Silence.hpp"
%}
#else
#include "Silence.hpp"
using namespace boost::numeric;
#endif

class Fl_Image;

namespace csound
{
        /**
        * Translates images in various RGB formats to scores.
        * Hue is mapped to instrument, value is mapped to loudness.
        */
        class ImageToScore : public ScoreNode
        {
        protected:
                std::string imageFilename;
                Fl_Image *image;
                size_t maximumVoiceCount;
                double minimumValue;
                static void rgbToHsv(double r, double g, double b, double &h, double &s, double &v);
                virtual void getPixel(size_t x, size_t y, double &hue, double &saturation, double &value) const;
                virtual void translate(double x, double y, double hue, double value, Event &event) const;
        public:
                ImageToScore(void);
                virtual ~ImageToScore(void);
                virtual void setImageFilename(std::string imageFilename);
                virtual std::string getImageFilename() const;
                virtual void setMaximumVoiceCount(size_t maximumVoiceCount);
                virtual size_t getMaximumVoiceCount() const;
                virtual void setMinimumValue(double minimumValue);
                virtual double getMinimumValue() const;
                virtual void generate();
        };
};

#endif
