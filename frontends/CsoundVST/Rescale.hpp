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
#ifndef RESCALE_H
#define RESCALE_H

#ifdef SWIG
%module CsoundVST
%{
#include "ScoreNode.hpp"
%}
#else
#include "ScoreNode.hpp"
using namespace boost::numeric;
#endif

namespace csound 
{
	/**
	* Rescales all child events to fit a bounding hypercube in music space.
	* No, some, or all dimensions may be rescaled to fit the minimum alone,
	* the range alone, or both the minimum and the range.
	*/
	class Rescale : 
		public ScoreNode
	{
		static bool initialized;
		static std::map<std::string, size_t> dimensions;
	public:
		Rescale();
		virtual ~Rescale();
		virtual void initialize();
		virtual void produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &coordinates);
		virtual void setRescale(int dimension, bool rescaleMinimum, bool rescaleRange, double targetMinimum, double targetRange);
		virtual void getRescale(int dimension, bool &rescaleMinimum, bool &rescaleRange, double &targetMinimum, double &targetRange);
	};
}
#endif


