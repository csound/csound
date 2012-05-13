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
#include "Sequence.hpp"
#include "System.hpp"

namespace csound
{
Sequence::Sequence()
{
}

Sequence::~Sequence()
{
}

Eigen::MatrixXd Sequence::traverse(const Eigen::MatrixXd &globalCoordinates,
        Score &collectingScore)
{
    size_t beginAt = collectingScore.size();
    // Obtain the composite transformation of coordinate system
    // by post-concatenating the local transformation of coordinate system
    // with the global, or enclosing, transformation of coordinate system.
    Eigen::MatrixXd compositeCoordinates = getLocalCoordinates() * globalCoordinates;
    // Make a bookmark for the current end of the score.
    //    size_t beginAt = score.size();
    // Descend into each of the child nodes.
    // Keep track of each child's relative time and
    // place the child nodes in strict temporal sequence.
    Score childScore;
    double deltaTime = 0.0;
    for(size_t i = 0, n = children.size(); i < n; i++) {
        childScore.clear();
        children[i]->traverse(compositeCoordinates, childScore);
        System::message("Sequence node at time %f: child %d of %d has %d notes.\n", deltaTime, i, n, childScore.size());
        for(size_t j = 0, k = childScore.size(); j < k; j++) {
            Event event = childScore[j];
            event.setTime(event.getTime() + deltaTime);
            collectingScore.push_back(event);
        }
        deltaTime = deltaTime + childScore.getDuration();
    }
    size_t endAt = collectingScore.size();
    produceOrTransform(collectingScore, beginAt, endAt, compositeCoordinates);
    // Return the composite transformation of coordinate system.
    return compositeCoordinates;
}
}
