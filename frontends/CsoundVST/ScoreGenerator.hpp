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
#ifndef SCOREGENERATOR_H
#define SCOREGENERATOR_H

#include "Platform.hpp"
#ifdef SWIG
%module scoregen
%include "typemaps.i"
%include "std_vector.i"
%{
#include <vector>
#include "Score.hpp"
#include "public.sdk/source/vst2.x/audioeffectx.h"
%}
%template(VstMidiEventVector) std::vector<VstMidiEvent>;
#else
#include <Python.h>
#include <vector>
#include "Score.hpp"
#include "public.sdk/source/vst2.x/audioeffectx.h"
#endif

class ScoreGeneratorVst;

/**
 * Python proxy for ScoreGeneratorVst,
 * to be wrapped with SWIG.
 */
class SILENCE_PUBLIC ScoreGenerator : public std::vector<VstMidiEvent>
{
protected:
  ScoreGeneratorVst *scoreGeneratorVst;
public:
  ScoreGenerator();
  virtual ~ScoreGenerator();
  virtual void setScoreGeneratorVst(PyObject *scoreGeneratorVst);
  virtual PyObject *getScoreGeneratorVst();
  virtual size_t event(double time, double duration, double status, double channel, double key, double velocity);
  virtual void score(csound::Score *score);
  virtual void write(char *message);
};

#endif
