/*
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
#ifndef SILENCEMCRM_H
#define SILENCEMCRM_H
#ifdef SWIG
%module CsoundVST
%include "std_string.i"
%include "std_vector.i"
%{
#include "Silence.hpp"
#include <vector>
%}
#else
#include "Silence.hpp"
#endif

namespace csound
{
	class MCRM : 
		public ScoreNode
	{
		// Hutchinson operator.
		std::vector< ublas::matrix<double> > transformations;
		// Pseudo-Markov operator.
		ublas::matrix<double> weights;
		// Depth of recursion.
		int depth;
		// Recursive iteration.
		void iterate(int depth, size_t p, const Event &event, double weight);
	public:
		MCRM();
		virtual ~MCRM();
		void setDepth(int depth);
		void resize(size_t transformations);
		void setTransformationElement(size_t index, size_t row, size_t column, double value);
		void setWeight(size_t precursor, size_t successor, double weight);
		void generate();
		// Node overrides.
		virtual void produceOrTransform(Score &score, 
			size_t beginAt, 
			size_t endAt, 
			const ublas::matrix<double> &coordinates);
	};
}
#endif
