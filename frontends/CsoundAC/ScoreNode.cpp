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
#include "ScoreNode.hpp"

namespace csound
{
ScoreNode::ScoreNode() : duration(0.0)
{
}

ScoreNode::~ScoreNode()
{
}

void ScoreNode::produceOrTransform(Score &collectingScore,
        size_t beginAt,
        size_t endAt,
        const Eigen::MatrixXd &compositeCoordinates)
{
    if(importFilename.length() > 0) {
        score.std::vector<Event>::clear();
        score.load(importFilename);
    }
    score.sort();
    if (duration != 0.0) {
        score.setDuration(duration);
    }
    for (int i = 0, n = score.size(); i < n; ++i) {
        Eigen::VectorXd product = compositeCoordinates * score[i];
        collectingScore.push_back(product);
    }

}

Score &ScoreNode::getScore()
{
    return score;
}
}
