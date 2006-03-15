#ifndef SCOREGENERATOR_H
#define SCOREGENERATOR_H

#include <Python.h>
#ifdef SWIG
%module scoregen
%include "std_vector.i"
%{
#include <vector>
#include "audioeffectx.h"
%}
%template(VstMidiEventVector) std::vector<VstMidiEvent>;
#else
#include <vector>
#include "audioeffectx.h"
#endif

class ScoreGeneratorVst;

/**
 * Python proxy for ScoreGeneratorVst,
 * to be wrapped with SWIG.
 */
class ScoreGenerator : public std::vector<VstMidiEvent>
{
protected:
  ScoreGeneratorVst *scoreGeneratorVst;
public:
  ScoreGenerator();
  virtual ~ScoreGenerator();
  virtual void setScoreGeneratorVst(PyObject *scoreGeneratorVst);
  virtual PyObject *getScoreGeneratorVst();
  virtual size_t event(double time, double duration, double status, double channel, double key, double velocity);
  virtual void write(char *message);
};

#endif
