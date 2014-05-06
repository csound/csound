/*
 * C S O U N D   V S T
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
Hocket::Hocket() : modulus(0), startingIndex(0)
{
}

Hocket::~Hocket()
{
}

Eigen::MatrixXd Hocket::traverse(const Eigen::MatrixXd &globalCoordinates,
        Score &collectingScore)
{
    size_t beginAt = collectingScore.size();
    score.std::vector<Event>::clear();
    Eigen::MatrixXd compositeCoordinates = getLocalCoordinates() * globalCoordinates;
    for(std::vector<Node*>::iterator it = children.begin(); it != children.end(); ++it) {
        Node *child = *it;
        child->traverse(compositeCoordinates, score);
    }
    size_t endAt = collectingScore.size();
    produceOrTransform(collectingScore, beginAt, endAt, compositeCoordinates);
    return compositeCoordinates;
}

}
