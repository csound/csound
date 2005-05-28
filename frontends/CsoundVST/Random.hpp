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
#if defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning (disable:4786)
#endif

#ifdef SWIG
%module CsoundVST
%{
#include "Node.hpp"
#include <boost/random.hpp>
#include <boost/random/variate_generator.hpp>
#include <cmath>
  %}
#else
#include "Node.hpp"
#include <boost/random.hpp>
#include <boost/random/variate_generator.hpp>
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
    void *generator_;
    boost::variate_generator<boost::mt19937, boost::uniform_smallint<> > *uniform_smallint_generator;
    boost::variate_generator<boost::mt19937, boost::uniform_int<> > *uniform_int_generator;
    boost::variate_generator<boost::mt19937, boost::uniform_real<> > *uniform_real_generator;
    boost::variate_generator<boost::mt19937, boost::bernoulli_distribution<> > *bernoulli_distribution_generator;
    boost::variate_generator<boost::mt19937, boost::geometric_distribution<> > *geometric_distribution_generator;
    boost::variate_generator<boost::mt19937, boost::triangle_distribution<> > *triangle_distribution_generator;
    boost::variate_generator<boost::mt19937, boost::exponential_distribution<> > *exponential_distribution_generator;
    boost::variate_generator<boost::mt19937, boost::normal_distribution<> > *normal_distribution_generator;
    boost::variate_generator<boost::mt19937, boost::lognormal_distribution<> > *lognormal_distribution_generator;
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
