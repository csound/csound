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
#ifndef NODE_HPP
#define NODE_HPP

#ifdef SWIG
%module CsoundVST
%{
#include "Score.hpp"
#include <vector>
#include <boost/numeric/ublas/matrix.hpp>
%}
%include "std_string.i"
%include "std_vector.i"
%template(NodeVector) std::vector<csound::NodePtr>;
#else
#include "Score.hpp"
#include <vector>
#include <boost/numeric/ublas/matrix.hpp>
using namespace boost::numeric;
#endif

namespace csound 
{
	/**
	* Base class for all music graph nodes in the Silence system.
	* Nodes can transform silence::Events produced by child nodes.
	* Nodes can produce silence::Events.
	*/
	class Node 
	{
	protected:
		ublas::matrix<double> localCoordinates;
	public:
		/**
		* Child Nodes, if any.
		*/
		std::vector<Node *> children;
		Node();
		virtual ~Node();
		/**
		* Returns the local transformation of coordinate system.
		*/
		virtual ublas::matrix<double> getLocalCoordinates() const;
		/**
		* The default implementation postconcatenates its own local coordinate system
		* with the global coordinates, then passes the score and the product of coordinate systems
		* to each child, thus performing a depth-first traversal of the music graph.
		*/
		virtual ublas::matrix<double> traverse(const ublas::matrix<double> &globalCoordinates,
			Score &score);
		/**
		* The default implementation does nothing.
		*/
		virtual void produceOrTransform(Score &score, 
			size_t beginAt, 
			size_t endAt, 
			const ublas::matrix<double> &coordinates);
		virtual ublas::matrix<double> Node::createTransform();
		virtual void clear();
		virtual double &element(size_t row, size_t column);
		virtual void setElement(size_t row, size_t column, double value);
		virtual void addChild(Node *node);
	};
	typedef Node* NodePtr;
}
#endif


