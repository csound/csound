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
#ifdef _MSC_VER
#pragma warning (disable:4786) 
#endif
#include "Node.hpp"

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

	ublas::matrix<double> Node::createTransform()
	{
		ublas::matrix<double> matrix = ublas::identity_matrix<double>(Event::ELEMENT_COUNT, Event::ELEMENT_COUNT);
		return matrix;
	}

	ublas::matrix<double> Node::getLocalCoordinates() const
	{
		return localCoordinates;
	}

	ublas::matrix<double> Node::traverse(const ublas::matrix<double> &globalCoordinates, Score &score)
	{
		// Obtain the composite transformation of coordinate system
		// by post-concatenating the local transformation of coordinate system
		// with the global, or enclosing, transformation of coordinate system.
		ublas::matrix<double> compositeCoordinates = ublas::prod(getLocalCoordinates(), globalCoordinates);
		// Make a bookmark for the current end of the score.
		size_t beginAt = score.size();
		// Descend into each of the child nodes.
		for(std::vector<Node*>::iterator i = children.begin(); i != children.end(); ++i)
		{
			(*i)->traverse(compositeCoordinates, score);
		}
		// Make a bookmark for the new end of the score, 
		// thus enclosing all Events that may have been produced or transformed
		// by all the child nodes.
		size_t endAt = score.size();
		// Take the score and optionally transform Events between the bookmarks,
		// or append new Events.
		produceOrTransform(score, beginAt, endAt, compositeCoordinates);
		// Return the composite transformation of coordinate system.
		return compositeCoordinates;
	}

	void Node::clear()
	{
		Node *node = 0;
		for(std::vector<Node*>::iterator i = children.begin(); i != children.end(); ++i)
		{
			node = *i;
			node->clear();
		}
		children.clear();
	}

	void Node::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &globalCoordinates)
	{
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
}
