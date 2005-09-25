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
#include "MCRM.hpp"
#include <boost/numeric/ublas/operation.hpp>

namespace csound
{
        MCRM::MCRM()
        {
        }

        MCRM::~MCRM()
        {
        }

        void MCRM::setDepth(int depth)
        {
                this->depth = depth;
        }

        void MCRM::resize(size_t count)
        {
                for(size_t i = 0; i < count; i++)
                {
                        transformations.push_back(createTransform());
                }
                weights.resize(transformations.size(), transformations.size());
                for(size_t precursor = 0; precursor < transformations.size(); precursor++)
                {
                        for(size_t successor = 0; successor < transformations.size(); successor++)
                        {
                                weights(precursor,successor) = 1.0;
                        }
                }
        }

        void MCRM::setTransformationElement(size_t index, size_t row, size_t column, double value)
        {
                transformations[index](row,column) = value;
        }

        void MCRM::setWeight(size_t precursor, size_t successor, double weight)
        {
                weights(precursor,successor) = weight;
        }

        void MCRM::iterate(int d, size_t p, const Event &event, double weight)
        {
                d--;
                if(d < 0)
                {
                        double velocity = event.getVelocity() * weight;
                        if(velocity > 0.0)
                        {
                                score.push_back(event);
                        }
                }
                else
                {
                        for(size_t s = 0; s < transformations.size(); s++)
                        {
                                const ublas::matrix<double> &t = transformations[s];
                                Event e;
                                ublas::axpy_prod(t, event, e);
                                double w = 0.0;
                                if(weight == -1.0)
                                {
                                        w = 1.0;
                                }
                                else
                                {
                                        w = weights(p,s) * weight;
                                }
                                iterate(d, s, e, w);
                        }
                }
        }

        void MCRM::generate()
        {
                Event event;
                event.setStatus(144);
                event.setVelocity(64);
                event.setKey(60);
                event.setDuration(1);
                double weight = -1;
                iterate(depth, 0, event, weight);
        }

        void MCRM::produceOrTransform(Score &score,
                size_t beginAt,
                size_t endAt,
                const ublas::matrix<double> &coordinates)
        {
                generate();
                ScoreNode::produceOrTransform(score, beginAt, endAt, coordinates);
        }
};
