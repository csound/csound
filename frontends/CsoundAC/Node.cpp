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
#include "Node.hpp"
#include <set>

namespace csound
{
Node::Node()
{
    localCoordinates.resize(Event::ELEMENT_COUNT, Event::ELEMENT_COUNT);
    localCoordinates = createTransform();
}

Node::~Node()
{
}

Eigen::MatrixXd Node::createTransform()
{
    Eigen::MatrixXd matrix = Eigen::MatrixXd::Identity(Event::ELEMENT_COUNT, Event::ELEMENT_COUNT);
    return matrix;
}

Eigen::MatrixXd Node::getLocalCoordinates() const
{
    return localCoordinates;
}

Eigen::MatrixXd Node::traverse(const Eigen::MatrixXd &globalCoordinates,
        Score &collectingScore)
{
    // Make a bookmark for the current end of the collecting score.
    size_t beginAt = collectingScore.size();
    // Obtain the composite transformation of coordinate system
    // by post-concatenating the local transformation of coordinate system
    // with the global, or enclosing, transformation of coordinate system.
    Eigen::MatrixXd compositeCoordinates = getLocalCoordinates() * globalCoordinates;
    // Descend into each of the child nodes.
    for(std::vector<Node*>::iterator i = children.begin(); i != children.end(); ++i) {
        (*i)->traverse(compositeCoordinates, collectingScore);
    }
    // Make a bookmark for the new end of the collecting score,
    // thus enclosing all Events that may have been produced
    // by all the child Nodes.
    size_t endAt = collectingScore.size();
    // Take the score and optionally transform Events between the bookmarks,
    // or append new Events.
    produceOrTransform(collectingScore, beginAt, endAt, compositeCoordinates);
    // Return the composite transformation of coordinate system.
    return compositeCoordinates;
}

void Node::produceOrTransform(Score &collectingScore,
        size_t beginAt,
        size_t endAt,
        const Eigen::MatrixXd &globalCoordinates)
{
}

void Node::clear()
{
    Node *node = 0;
    for(std::vector<Node*>::iterator i = children.begin(); i != children.end(); ++i) {
        node = *i;
        node->clear();
    }
    children.clear();
}

double &Node::element(size_t row, size_t column)
{
    return localCoordinates(row, column);
}

void Node::setElement(size_t row, size_t column, double value)
{
    localCoordinates(row, column) = value;
}

void Node::addChild(Node *node)
{
    children.push_back(node);
}

void RemoveDuplicates::produceOrTransform(Score &collectingScore,
    size_t beginAt,
    size_t endAt,
    const Eigen::MatrixXd &globalCoordinates)
{
    std::set<std::string> uniqueEvents;
    Score newScore;
    for (size_t i = 0, n = collectingScore.size(); i < n; ++i) {
        const Event &event = collectingScore[i];
        std::string istatement = event.toCsoundIStatement();
        if (uniqueEvents.find(istatement) == uniqueEvents.end()) {
            newScore.push_back(event);
            uniqueEvents.insert(istatement);
        }
    }
    collectingScore = newScore;
}

}
