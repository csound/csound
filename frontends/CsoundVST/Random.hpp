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
#ifndef RANDOM_H
#define RANDOM_H
#ifdef _MSC_VER
#pragma warning (disable:4786) 
#endif

#ifdef SWIG
%module CsoundVST
%{
#include "Node.hpp"
#include <boost/random.hpp>
#include <cmath>
%}
#else
#include "Node.hpp"
#include <boost/random.hpp>
#include <cmath>
using namespace boost::numeric;
#endif

namespace csound
{

	/**
	* A random value will be sampled from the specified distribution,
	* translated and scaled as specified,
	* and set in the specified row and column of the local coordinates.
	* The resulting matrix will be used in place of the local coordinates
	* when traversing the music graph.
	* If eventCount is greater than zero, a new event will be created 
	* for each of eventCount samples,
	* which will be transformed by the newly sampled local coordinates.
	*/
	class Random : 
		public Node
	{
	protected:
		boost::uniform_smallint<boost::mt19937> *uniform_smallint;
		boost::uniform_int<boost::mt19937> *uniform_int;
		boost::uniform_01<boost::mt19937> *uniform_01;
		boost::uniform_real<boost::mt19937> *uniform_real;
		boost::bernoulli_distribution<boost::mt19937> *bernoulli_distribution;
		boost::geometric_distribution<boost::mt19937> *geometric_distribution;
		boost::triangle_distribution<boost::mt19937> *triangle_distribution;
		boost::exponential_distribution<boost::mt19937> *exponential_distribution;
		boost::normal_distribution<boost::mt19937> *normal_distribution;
		boost::lognormal_distribution<boost::mt19937> *lognormal_distribution;
	public:
		static boost::mt19937 mersenneTwister;
		std::string distribution;
		int row;
		int column;
		int eventCount;
		bool incrementTime;
		double minimum;
		double maximum;
		double q;
		double a;
		double b;
		double c;
		double Lambda;
		double mean;
		double sigma;	
		Random();
		virtual ~Random();
		virtual double sample() const;
		virtual ublas::matrix<double> getLocalCoordinates() const;
		virtual void createDistribution(std::string distribution);
		virtual void produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &globalCoordinates);
		static void seed(int s);
	};
}
#endif
