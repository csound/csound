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
#include "Hocket.hpp"

namespace csound
{
        Hocket::Hocket()
        {
        }

        Hocket::~Hocket()
        {
        }

        ublas::matrix<double> Hocket::traverse(const ublas::matrix<double> &globalCoordinates, Score &score)
        {
                this->score.std::vector<Event>::clear();
                size_t beginAt = this->score.size();
                ublas::matrix<double> compositeCoordinates = ublas::prod(getLocalCoordinates(), globalCoordinates);
                for(std::vector<Node*>::iterator it = children.begin(); it != children.end(); ++it)
                {
                        Node *child = *it;
                        child->traverse(compositeCoordinates, this->score);
                }
                size_t endAt = this->score.size();
                produceOrTransform(score, beginAt, endAt, compositeCoordinates);
                return compositeCoordinates;
        }

        void Hocket::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &coordinates)
        {
                std::sort(this->score.begin(), this->score.end());
                for(size_t i = startingIndex, n = this->score.size(); i < n; i += modulus)
                {
                        score.push_back(this->score[i]);
                }
        }
}
