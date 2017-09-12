/**
 * S C O R E   G E N E R A T O R   V S T
 *
 * A VST plugin for writing score generators in Python.
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
#include "ScoreGenerator.hpp"
#include "ScoreGeneratorVst.hpp"
#include "Event.hpp"
#include <vector>

ScoreGenerator::ScoreGenerator() : scoreGeneratorVst(0)
{
}

ScoreGenerator::~ScoreGenerator()
{
}

void ScoreGenerator::setScoreGeneratorVst(PyObject *pyObject)
{
  scoreGeneratorVst = (ScoreGeneratorVst *)PyCObject_AsVoidPtr(pyObject);
}

PyObject *ScoreGenerator::getScoreGeneratorVst()
{
  return PyCObject_FromVoidPtr(scoreGeneratorVst, 0);
}

size_t ScoreGenerator::event(double time, double duration, double status, double channel, double key, double velocity)
{
  if (scoreGeneratorVst) {
    return scoreGeneratorVst->event(time, duration, status, channel, key, velocity);
  } else {
    return 0;
  }
}

void ScoreGenerator::write(char *message)
{
  if (scoreGeneratorVst) {
    scoreGeneratorVst->log(message);
  }
}

void ScoreGenerator::score(csound::Score *score)
{
    if (scoreGeneratorVst) {
        scoreGeneratorVst->clearEvents();
        for (std::vector<csound::Event>::iterator it = score->begin(); it != score->end(); ++it) {
            double time = it->getTime();
            double duration = it->getDuration();
            double status = it->getStatus();
            double channel = it->getChannel();
            double key = it->getKey();
            double velocity = it->getVelocity();
            scoreGeneratorVst->event(time, duration, status, channel, key, velocity);
        }
    }
}

