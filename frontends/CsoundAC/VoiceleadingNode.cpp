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
#include "VoiceleadingNode.hpp"
#include "Conversions.hpp"
#include "Voicelead.hpp"
#include "System.hpp"

#include <cmath>
#include <cfloat>
#include <sstream>

namespace csound
{
  extern void printChord(std::ostream &stream, std::string label, const std::vector<double> &chord);

  extern void printChord(std::string label, const std::vector<double> &chord);

  /**
   * Utility class for storing voice-leading operations.
   */
  VoiceleadingOperation::VoiceleadingOperation() :
    beginTime(0.0),
    rescaledBeginTime(0.0),
    endTime(0.0),
    rescaledEndTime(0.0),
    P(DBL_MAX),
    T(DBL_MAX),
    C(DBL_MAX),
    V(DBL_MAX),
    L(false),
    begin(0),
    end(0),
    avoidParallels(false)
  {
  }

  VoiceleadingOperation::~VoiceleadingOperation()
  {
  }

  VoiceleadingNode::VoiceleadingNode() :
    base(36.0),
    range(60.0),
    rescaleTimes(true),
    avoidParallels(true),
    divisionsPerOctave(12)
  {
  }

  VoiceleadingNode::~VoiceleadingNode()
  {
  }

  std::ostream &operator << (std::ostream &stream, const VoiceleadingOperation &operation)
  {
    stream << "  beginTime:         " << operation.beginTime << std::endl;
    stream << "  endTime:           " << operation.endTime << std::endl;
    stream << "  rescaledBeginTime: " << operation.rescaledBeginTime << std::endl;
    stream << "  rescaledEndTime:   " << operation.rescaledEndTime << std::endl;
    stream << "  begin:             " << operation.begin << std::endl;
    stream << "  end:               " << operation.end << std::endl;
    if (!(operation.P == DBL_MAX)) {
      stream << "  P:                 " << operation.P << std::endl;
    }
    if (!(operation.T == DBL_MAX)) {
      stream << "  T:                 " << operation.T << std::endl;
    }
    if (!(operation.C == DBL_MAX)) {
      stream << "  C:                 " << operation.C << std::endl;
    }
    if (!(operation.V == DBL_MAX)) {
      stream << "  V:                 " << operation.V << std::endl;
    }
    if (operation.L) {
      stream << "  L:                 " << int(operation.L) << std::endl;
    }
    return stream;
  }

  void VoiceleadingNode::apply(Score &score, const VoiceleadingOperation &priorOperation, const VoiceleadingOperation &operation)
  {
    if ( (System::getMessageLevel() & System::INFORMATION_LEVEL) == System::INFORMATION_LEVEL) {
      std::stringstream stream;
      stream << "BEGAN VoiceleadingNode::apply:..." << std::endl;
      stream << "Events in score:     " << score.size() << std::endl;
      stream << "Score duration:      " << score.getDuration() << std::endl;
      stream << "Events in operation: " << (operation.end - operation.begin) << std::endl;
      stream << "priorOperation:      " << std::endl << priorOperation;
      stream << "currrentOperation:   " << std::endl << operation;
      stream << std::endl;
      System::inform(stream.str().c_str());
    }
    if (operation.begin == operation.end) {
      return;
    }
    if (!(operation.P == DBL_MAX) && !(operation.T == DBL_MAX)) {
      if (!(operation.V == DBL_MAX)) {
        score.setPTV(operation.begin,
                     operation.end,
                     operation.P,
                     operation.T,
                     operation.V,
                     base,
                     range);
      } else if (operation.L) {
        score.setPT(operation.begin,
                    operation.end,
                    operation.P,
                    operation.T,
                    base,
                    range,
                    divisionsPerOctave);
        score.voicelead(priorOperation.begin,
                        priorOperation.end,
                        operation.begin,
                        operation.end,
                        base,
                        range,
                        avoidParallels,
                        divisionsPerOctave);
      } else {
        score.setPT(operation.begin,
                    operation.end,
                    operation.P,
                    operation.T,
                    base,
                    range,
                    divisionsPerOctave);
      }
    } else if (!(operation.C == DBL_MAX)) {
      if (!(operation.V == DBL_MAX)) {
        std::vector<double> pcs = Voicelead::mToPitchClassSet(Voicelead::cToM(operation.C, divisionsPerOctave), divisionsPerOctave);
        printChord("CV", pcs);
        std::vector<double> pt = Voicelead::pitchClassSetToPandT(pcs, divisionsPerOctave);
        double prime = pt[0];
        double transposition = pt[1];
        System::inform("prime: %f transposition %f: divisionsPerOctave %d\n", prime, transposition, divisionsPerOctave);
        score.setPTV(operation.begin,
                     operation.end,
                     prime,
                     transposition,
                     operation.V,
                     base,
                     range);
      } else if (operation.L) {
        std::vector<double> pcs = Voicelead::mToPitchClassSet(Voicelead::cToM(operation.C, divisionsPerOctave), divisionsPerOctave);
        printChord("CL", pcs);
        score.voicelead(priorOperation.begin,
                        priorOperation.end,
                        operation.begin,
                        operation.end,
                        pcs,
                        base,
                        range,
                        avoidParallels,
                        divisionsPerOctave);
      } else {
        std::vector<double> pcs = Voicelead::mToPitchClassSet(Voicelead::cToM(operation.C, divisionsPerOctave), divisionsPerOctave);
        score.setPitchClassSet(operation.begin,
                               operation.end,
                               pcs,
                               divisionsPerOctave);
      }
    } else {
        if (!(operation.V == DBL_MAX)) {
        std::vector<double> ptv = score.getPTV(operation.begin,
                                               operation.end,
                                               base,
                                               range,
                                               divisionsPerOctave);
        score.setPTV(operation.begin,
                     operation.end,
                     ptv[0],
                     ptv[1],
                     operation.V,
                     base,
                     range,
                     divisionsPerOctave);
      } else if (operation.L) {
        score.voicelead(priorOperation.begin,
                        priorOperation.end,
                        operation.begin,
                        operation.end,
                        base,
                        range,
                        avoidParallels,
                        divisionsPerOctave);
      }
    }
    System::message("ENDED VoiceleadingNode::apply.\n");
  }

  void VoiceleadingNode::PT(double time, double P, double T)
  {
    operations[time].beginTime = time;
    operations[time].P = P;
    operations[time].T = T;
  }

  void VoiceleadingNode::PTV(double time, double P, double T, double V)
  {
    operations[time].beginTime = time;
    operations[time].P = P;
    operations[time].T = T;
    operations[time].V = V;
  }

  void VoiceleadingNode::PTL(double time, double P, double T, bool avoidParallels)
  {
    operations[time].beginTime = time;
    operations[time].P = P;
    operations[time].T = T;
    operations[time].L = true;
    operations[time].avoidParallels = avoidParallels;
  }

  void VoiceleadingNode::C(double time, double C_)
  {
    operations[time].beginTime = time;
    operations[time].C = C_;
  }

  void VoiceleadingNode::C(double time, std::string C_)
  {
    C(time, Voicelead::nameToC(C_, divisionsPerOctave));
  }

  void VoiceleadingNode::CV(double time, double C, double V)
  {
    operations[time].beginTime = time;
    operations[time].C = C;
    operations[time].V = V;
  }

  void VoiceleadingNode::CV(double time, std::string C, double V)
  {
    CV(time, Voicelead::nameToC(C, divisionsPerOctave), V);
  }

  void VoiceleadingNode::CL(double time, double C, bool avoidParallels)
  {
    operations[time].beginTime = time;
    operations[time].C = C;
    operations[time].L = true;
    operations[time].avoidParallels = avoidParallels;
  }

  void VoiceleadingNode::CL(double time, std::string C, bool avoidParallels)
  {
    CL(time, Voicelead::nameToC(C, divisionsPerOctave), avoidParallels);
  }

  void VoiceleadingNode::V(double time, double V_)
  {
    operations[time].beginTime = time;
    operations[time].V = V_;
  }

  void VoiceleadingNode::L(double time, bool avoidParallels)
  {
    operations[time].beginTime = time;
    operations[time].L = true;
    operations[time].avoidParallels = avoidParallels;
  }

  void VoiceleadingNode::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &coordinates)
  {
    transform(score, rescaleTimes);
  }

  void VoiceleadingNode::transform(Score &score, bool rescaleTimes_)
  {
    if (operations.empty()) {
      return;
    }
    // Find the time rescale factor.
    score.sort();
    score.findScale();
    double origin = score.scaleActualMinima.getTime();
    double duration = score.getDuration();
    double scoreMaxTime = origin + duration;
    double operationMaxTime = 0.0;
    double timeScale = 1.0;
    std::vector<VoiceleadingOperation *> ops;
    for (std::map<double, VoiceleadingOperation>::iterator it = operations.begin(); it != operations.end(); ++it) {
      if (it->second.beginTime > operationMaxTime) {
        operationMaxTime = it->second.beginTime;
      }
      ops.push_back(&it->second);
    }
    if (rescaleTimes_) {
      if (operationMaxTime > 0.0) {
        timeScale = duration / operationMaxTime;
      }
    }
    System::inform("BEGAN VoiceleadingNode::produceOrTransform  operationMaxTime: %f  origin: %f  duration: %f  scoreMaxTime: %f  timeScale: %f...\n",
                   operationMaxTime,
                   origin,
                   duration,
                   scoreMaxTime,
                   timeScale);
    VoiceleadingOperation *priorOperation = 0;
    VoiceleadingOperation *currentOperation = 0;
    VoiceleadingOperation *nextOperation = 0;
    for (int priorIndex = -1, currentIndex = 0, nextIndex = 1, firstIndex = 0, endIndex = ops.size(), backIndex = ops.size() - 1;
         currentIndex < endIndex;
         priorIndex++, currentIndex++, nextIndex++) {
      if (currentIndex <= firstIndex) {
        priorOperation = ops[currentIndex];
        currentOperation = ops[currentIndex];
      } else {
        priorOperation = ops[priorIndex];
        currentOperation = ops[currentIndex];
      }
      currentOperation->rescaledBeginTime = ((currentOperation->beginTime - origin) * timeScale) + origin;
      currentOperation->begin = score.indexAtTime(currentOperation->rescaledBeginTime);
      if (currentIndex >= backIndex) {
        currentOperation->endTime = std::max(currentOperation->rescaledBeginTime, scoreMaxTime);
        currentOperation->rescaledEndTime = currentOperation->endTime;
        currentOperation->end = score.size();
      } else {
        nextOperation = ops[nextIndex];
        currentOperation->endTime = nextOperation->beginTime;
        currentOperation->rescaledEndTime = currentOperation->endTime * timeScale;
        currentOperation->end = score.indexAfterTime(currentOperation->rescaledEndTime);
      }
      apply(score, *priorOperation, *currentOperation);
    }
    System::inform("ENDED VoiceleadingNode::produceOrTransform.\n");
  }

}
