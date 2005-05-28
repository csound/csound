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
#include "Random.hpp"
#include <boost/numeric/ublas/operation.hpp>

namespace csound
{
  boost::mt19937 Random::mersenneTwister;

  Random::Random() :
    generator_(0),
    row(0),
    column(Event::HOMOGENEITY),
    eventCount(0),
    incrementTime(true),
    minimum(0.0),
    maximum(1.0),
    q(1.0),
    a(0.0),
    b(1.0),
    c(0.0),
    Lambda(1.0),
    mean(0.0),
    sigma(1.0)
  {
    distribution = "uniform_real";
  }

  Random::~Random()
  {
  }

  void Random::createDistribution(std::string distribution)
  {
    if(distribution == "uniform_smallint")
      {
        uniform_smallint_generator =
          new boost::variate_generator<boost::mt19937, boost::uniform_smallint<> >(mersenneTwister, boost::uniform_smallint<>(int(minimum), int(maximum)));
        generator_ = &uniform_smallint_generator;
      }
    else if(distribution == "uniform_int")
      {
        uniform_int_generator =
          new boost::variate_generator<boost::mt19937, boost::uniform_int<> >(mersenneTwister, boost::uniform_int<>(int(minimum), int(maximum)));
        generator_ = &uniform_int_generator;
      }
    else if(distribution == "uniform_real")
      {
        uniform_real_generator =
          new boost::variate_generator<boost::mt19937, boost::uniform_real<> >(mersenneTwister, boost::uniform_real<>(int(minimum), int(maximum)));
        generator_ = &uniform_real_generator;
      }
    else if(distribution == "bernoulli")
      {
        bernoulli_distribution_generator =
          new boost::variate_generator<boost::mt19937, boost::bernoulli_distribution<> >(mersenneTwister, boost::bernoulli_distribution<>(q));
        generator_ = &bernoulli_distribution_generator;
      }
    else if(distribution == "geometric")
      {
        geometric_distribution_generator =
          new boost::variate_generator<boost::mt19937, boost::geometric_distribution<> >(mersenneTwister, boost::geometric_distribution<>(q));
        generator_ = &geometric_distribution_generator;
      }
    else if(distribution == "triangle")
      {
        triangle_distribution_generator =
          new boost::variate_generator<boost::mt19937, boost::triangle_distribution<> >(mersenneTwister, boost::triangle_distribution<>(a, b, c));
        generator_ = &triangle_distribution_generator;
      }
    else if(distribution == "exponential")
      {
        exponential_distribution_generator =
          new boost::variate_generator<boost::mt19937, boost::exponential_distribution<> >(mersenneTwister, boost::exponential_distribution<>(Lambda));
        generator_ = &exponential_distribution_generator;
      }
    else if(distribution == "normal")
      {
        normal_distribution_generator =
          new boost::variate_generator<boost::mt19937, boost::normal_distribution<> >(mersenneTwister, boost::normal_distribution<>(mean, sigma));
        generator_ = &normal_distribution_generator;
      }
    else if(distribution == "lognormal")
      {
        lognormal_distribution_generator =
          new boost::variate_generator<boost::mt19937, boost::lognormal_distribution<> >(mersenneTwister, boost::lognormal_distribution<>(mean, sigma));
        generator_ = &lognormal_distribution_generator;
      }
  }
  double Random::sample() const
  {
    if(generator_ == &uniform_smallint_generator) return (*uniform_smallint_generator)();
    if(generator_ == &uniform_int_generator) return (*uniform_int_generator)();
    if(generator_ == &uniform_real_generator) return (*uniform_real_generator)();
    if(generator_ == &bernoulli_distribution_generator) return (*bernoulli_distribution_generator)();
    if(generator_ == &geometric_distribution_generator) return (*geometric_distribution_generator)();
    if(generator_ == &triangle_distribution_generator) return (*triangle_distribution_generator)();
    if(generator_ == &exponential_distribution_generator) return (*exponential_distribution_generator)();
    if(generator_ == &normal_distribution_generator) return (*normal_distribution_generator)();
    if(generator_ == &lognormal_distribution_generator) return (*lognormal_distribution_generator)();
    return 0;
  }
  ublas::matrix<double> Random::getLocalCoordinates() const
  {
    ublas::matrix<double> transformation = localCoordinates;
    for(int i = 0; i < Event::HOMOGENEITY; i++)
      {
        transformation[i][Event::HOMOGENEITY] *= sample();
      }
    return transformation;
  }
  void Random::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &globalCoordinates)
  {
    createDistribution(distribution);
    if(eventCount > 0)
      {
        double currentTime = 0;
        for (int i = 0; i < eventCount; i++)
          {
            //  Resample for every generated note.
            ublas::matrix<double> localCoordinates = getLocalCoordinates();
            ublas::matrix<double> compositeCoordinates = ublas::prod(localCoordinates, globalCoordinates);
            Event event(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
            Event transformedEvent;
            ublas::axpy_prod(compositeCoordinates, event, transformedEvent);
            if (incrementTime)
              {
                double buffer = fabs(transformedEvent.getTime());
                transformedEvent.setTime(buffer + currentTime);
                currentTime += buffer;
              }
            score.push_back(transformedEvent);
          }
      }
    else
      {
        for (size_t i = beginAt; i < endAt; i++)
          {
            ublas::matrix<double> local = getLocalCoordinates();
            ublas::matrix<double> compositeCoordinates = ublas::prod(local, globalCoordinates);
            score[i] = ublas::prod(compositeCoordinates, score[i]);
          }
      }
  }

  void Random::seed(int s)
  {
    mersenneTwister.seed((unsigned int) s);
  }
}

