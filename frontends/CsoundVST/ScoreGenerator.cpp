#include "ScoreGenerator.hpp"
#include "ScoreGeneratorVst.hpp"

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
