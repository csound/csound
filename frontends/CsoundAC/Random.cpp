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
#include "Random.hpp"

namespace csound
{
  std::mt19937 Random::mersenneTwister;

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
        uniform_smallint_generator = std::uniform_int_distribution<std::int32_t>(int(minimum), int(maximum));
        generator_ = &uniform_smallint_generator;
      }
    else if(distribution == "uniform_int")
      {
        uniform_int_generator = std::uniform_int_distribution<std::int64_t>(int(minimum), int(maximum));
        generator_ = &uniform_int_generator;
      }
    else if(distribution == "uniform_real")
      {
        uniform_real_generator = std::uniform_real_distribution<>(minimum, maximum);
        generator_ = &uniform_real_generator;
      }
    else if(distribution == "bernoulli")
      {
        bernoulli_distribution_generator = std::bernoulli_distribution(q);
        generator_ = &bernoulli_distribution_generator;
      }
    else if(distribution == "geometric")
      {
        geometric_distribution_generator = std::geometric_distribution<>(q);
        generator_ = &geometric_distribution_generator;
      }
    else if(distribution == "exponential")
      {
        exponential_distribution_generator = std::exponential_distribution<>(Lambda);
        generator_ = &exponential_distribution_generator;
      }
    else if(distribution == "normal")
      {
        normal_distribution_generator = std::normal_distribution<>(mean, sigma);
        generator_ = &normal_distribution_generator;
      }
    else if(distribution == "lognormal")
      {
        lognormal_distribution_generator = std::lognormal_distribution<>(mean, sigma);
        generator_ = &lognormal_distribution_generator;
      }
  }
  double Random::sample()
  {
    if(generator_ == &uniform_smallint_generator) return uniform_smallint_generator(mersenneTwister);
    if(generator_ == &uniform_int_generator) return uniform_int_generator(mersenneTwister);
    if(generator_ == &uniform_real_generator) return uniform_real_generator(mersenneTwister);
    if(generator_ == &bernoulli_distribution_generator) return bernoulli_distribution_generator(mersenneTwister);
    if(generator_ == &geometric_distribution_generator) return geometric_distribution_generator(mersenneTwister);
    if(generator_ == &exponential_distribution_generator) return exponential_distribution_generator(mersenneTwister);
    if(generator_ == &normal_distribution_generator) return normal_distribution_generator(mersenneTwister);
    if(generator_ == &lognormal_distribution_generator) return lognormal_distribution_generator(mersenneTwister);
    return 0;
  }
  Eigen::MatrixXd Random::getRandomCoordinates()
  {
    Eigen::MatrixXd transformation = getLocalCoordinates();
    for(int i = 0; i < Event::HOMOGENEITY; i++)
      {
        transformation(i, Event::HOMOGENEITY) *= sample();
      }
    return transformation;
  }
  void Random::produceOrTransform(Score &score,
                                  size_t beginAt,
                                  size_t endAt,
                                  const Eigen::MatrixXd &compositeCoordinates)
  {
    createDistribution(distribution);
    if(eventCount > 0)
      {
        double currentTime = 0;
        for (int i = 0; i < eventCount; i++)
          {
            //  Resample for every generated note.
            Event event(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
            Event transformedEvent;
            //ublas::axpy_prod(getRandomCoordinates(), event, transformedEvent);
            transformedEvent = getRandomCoordinates() * event;
            if (incrementTime)
              {
                double buffer = fabs(transformedEvent.getTime());
                transformedEvent.setTime(buffer + currentTime);
                currentTime += buffer;
              }
            score.push_back(transformedEvent);
          }
        // Apply the global transformation of coordinate system
        // to all child events produced by this node.
        size_t finalEndAt = score.size();
        for (size_t i = endAt; i < finalEndAt; i++) {
          score[i] = compositeCoordinates * score[i];
        }
      }
    else
      {
        for (size_t i = beginAt; i < endAt; i++)
          {
            score[i] = getRandomCoordinates() * score[i];
          }
      }
  }

  void Random::seed(int s)
  {
    mersenneTwister.seed((unsigned int) s);
  }
}
