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
#if defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning (disable:4786)
#endif
#include "Rescale.hpp"

namespace csound
{
        bool Rescale::initialized = false;
        std::map<std::string, size_t> Rescale::dimensions;

        Rescale::Rescale()
        {
                initialize();
                for(size_t i = 0; i < Event::ELEMENT_COUNT; i++)
                {
                        score.rescaleMinima[i] = false;
                        score.rescaleRanges[i] = false;
                }
        }

        Rescale::~Rescale()
        {
        }

        void Rescale::initialize()
        {
                if(!initialized)
                {
                        for(int i = 0; i < Event::ELEMENT_COUNT; i++)
                        {
                                dimensions[std::string("rescaleMinimum") + Event::labels[i]] = i;
                                dimensions[std::string("rescaleRange") + Event::labels[i]] = i;
                                dimensions[std::string("targetMinimum") + Event::labels[i]] = i;
                                dimensions[std::string("targetRange") + Event::labels[i]] = i;
                        }
                        initialized = true;
                }
        }

        void Rescale::produceOrTransform(Score &score_, size_t beginAt, size_t endAt, const ublas::matrix<double> &globalCoordinates)
        {
                for(int i = 0; i < Event::ELEMENT_COUNT; i++)
                {
                        score_.setScale(score_, i, score.rescaleMinima[i], score.rescaleRanges[i], beginAt, endAt, score.scaleTargetMinima[i], score.scaleTargetRanges[i]);
                }
        }

        void Rescale::setRescale(int dimension, bool rescaleMinimum, bool rescaleRange, double targetMinimum, double targetRange)
        {
                score.rescaleMinima[dimension] = rescaleMinimum;
                score.rescaleRanges[dimension] = rescaleRange;
                score.scaleTargetMinima[dimension] = targetMinimum;
                score.scaleTargetRanges[dimension] = targetRange;
        }

        void Rescale::getRescale(int dimension, bool &rescaleMinimum, bool &rescaleRange, double &targetMinimum, double &targetRange)
        {
                rescaleMinimum = score.rescaleMinima[dimension];
                rescaleRange = score.rescaleRanges[dimension];
                targetMinimum = score.scaleTargetMinima[dimension];
                targetRange = score.scaleTargetRanges[dimension];
        }
}
