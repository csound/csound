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
#include "VoiceleadingNode.hpp"
#include "Conversions.hpp"
#include "Voicelead.hpp"
#include "System.hpp"

#include <cmath>
#include <sstream>

namespace csound
{

  extern void printChord(std::ostream &stream, std::string label, const std::vector<double> &chord);

  extern void printChord(std::string label, const std::vector<double> &chord); 

  VoiceleadingOperation::VoiceleadingOperation() : 
    time(0.0),
    rescaledTime(0.0),
    P(double(0.0) / double(0.0)),
    T(double(0.0) / double(0.0)),
    C(double(0.0) / double(0.0)),
    V(double(0.0) / double(0.0)),
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
    stream << "time (rescaled): " << operation.time << " (" << operation.rescaledTime << ")" << std::endl;
    stream << "  begin:         " << operation.begin << std::endl;
    stream << "  end:           " << operation.end << std::endl;
    if (!std::isnan(operation.P)) {
      stream << "  P:             " << operation.P << std::endl;
    }
    if (!std::isnan(operation.T)) {
      stream << "  T:             " << operation.T << std::endl;
    }
    if (!std::isnan(operation.C)) {
      stream << "  C:             " << operation.C << std::endl;
    }
    if (!std::isnan(operation.V)) {
      stream << "  V:             " << operation.V << std::endl;
    }
    if (operation.L) {
      stream << "  L:             " << int(operation.L) << std::endl;
    }
    return stream;
  }

  void VoiceleadingNode::apply(Score &score, const VoiceleadingOperation &priorOperation, const VoiceleadingOperation &operation)
  {
    if ( (System::getMessageLevel() & System::INFORMATION_LEVEL) == System::INFORMATION_LEVEL) {
      std::stringstream stream;
      stream << "BEGAN VoiceleadingNode::apply:..." << std::endl;
      stream << "priorOperation:    " << priorOperation;
      stream << "currrentOperation: " << operation;
      stream << std::endl;
      System::inform(stream.str().c_str());
    }
    if (operation.begin == operation.end) {
      return;
    }
    if (!std::isnan(operation.P) && !std::isnan(operation.T)) {
      if (!std::isnan(operation.V)) {
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
    } else if (!std::isnan(operation.C)) {
      if (!std::isnan(operation.V)) {
	double prime = 0.0;
	double transposition = 0.0;
	std::vector<double> pcs = Voicelead::pitchClassSetFromM(Voicelead::cToM(operation.C, divisionsPerOctave), divisionsPerOctave);
	printChord("CV", pcs);
	Voicelead::primeAndTranspositionFromPitchClassSet(pcs, prime, transposition, divisionsPerOctave);
	System::inform("prime: %f transposition %f: divisionsPerOctave %d\n", prime, transposition, divisionsPerOctave);
	score.setPTV(operation.begin, 
		     operation.end, 
		     prime, 
		     transposition, 
		     operation.V, 
		     base, 
		     range);
      } else if (operation.L) {
	std::vector<double> pcs = Voicelead::pitchClassSetFromM(Voicelead::cToM(operation.C, divisionsPerOctave), divisionsPerOctave);
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
	std::vector<double> pcs = Voicelead::pitchClassSetFromM(Voicelead::cToM(operation.C, divisionsPerOctave), divisionsPerOctave);
	score.setPitchClassSet(operation.begin, 
			       operation.end, 
			       pcs,
			       divisionsPerOctave);
      }
    } else {
      if (!std::isnan(operation.V)) {
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
    System::inform("ENDED VoiceleadingNode::apply.\n");
  }

  void VoiceleadingNode::transform(Score &score)
  {
    if (operations.empty()) {
      return;
    }
    // First, rescale the times for the operations, if that is required.
    double scoreMaxTime = score.getDuration();
    double operationMaxTime = 0.0;
    double timeScale = 1.0;
    std::vector<double> keys;
    for (std::map<double, VoiceleadingOperation>::iterator it = operations.begin(); it != operations.end(); ++it) {
      if (it->second.time > operationMaxTime) {
	operationMaxTime = it->second.time;
      }
      keys.push_back(it->first);
    }
    if (rescaleTimes) {
      if (operationMaxTime > 0.0) {
	timeScale = scoreMaxTime / operationMaxTime;
      }
    }
    System::inform("BEGAN VoiceleadingNode::transform scoreMaxTime: %f  operationMaxTime: %f  timeScale: %f...\n", scoreMaxTime, operationMaxTime, timeScale);
    for (size_t i = 0, n = keys.size(); i < n; i++) {
      VoiceleadingOperation &operation = operations[keys[i]];
      operation.rescaledTime = operation.time * timeScale;
      operation.begin = score.indexAtTime(operation.rescaledTime);
      operation.end = score.size();
    }
    for (size_t i = 0, j = 1, n = keys.size(); j < n; i++, j++) {
      VoiceleadingOperation &operationI = operations[keys[i]];
      VoiceleadingOperation &operationJ = operations[keys[j]];
      operationI.end = operationJ.begin;
    }
    if (keys.size() == 1) {
      VoiceleadingOperation &operation = operations[keys[0]];
      apply(score, operation, operation);
    } else {
      for (size_t i = 0, n = keys.size() - 1; i < n; i++) {
	VoiceleadingOperation *operationI = 0;
	VoiceleadingOperation *operationJ = &operations[keys[i]];
	if (i == 0) {
	  operationI = &operations[keys[i]];
	} else {
	  operationI = &operations[keys[i - 1]];
	}
	System::inform("Operation: %d\n", (i + 1));
	apply(score, *operationI, *operationJ);
      }
    }
    System::inform("ENDED VoiceleadingNode::transform.\n");
  }
  
  void VoiceleadingNode::PT(double time, double P, double T)
  {
    operations[time].time = time;
    operations[time].P = P;
    operations[time].T = T;
  }

  void VoiceleadingNode::PTV(double time, double P, double T, double V)
  {
    operations[time].time = time;
    operations[time].P = P;
    operations[time].T = T;
    operations[time].V = V;
  }

  void VoiceleadingNode::PTL(double time, double P, double T, bool avoidParallels)
  {
    operations[time].time = time;
    operations[time].P = P;
    operations[time].T = T;
    operations[time].L = true;
    operations[time].avoidParallels = avoidParallels;
  }

  void VoiceleadingNode::C(double time, double C_)
  {
    operations[time].time = time;
    operations[time].C = C_;
  }

  void VoiceleadingNode::C(double time, std::string C_)
  {
    C(time, Voicelead::nameToC(C_, divisionsPerOctave));
  }

  void VoiceleadingNode::CV(double time, double C, double V)
  {
    operations[time].time = time;
    operations[time].C = C;
    operations[time].V = V;
  }

  void VoiceleadingNode::CV(double time, std::string C, double V)
  {
    CV(time, Voicelead::nameToC(C, divisionsPerOctave), V);
  }

  void VoiceleadingNode::CL(double time, double C, bool avoidParallels)
  {
    operations[time].time = time;
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
    operations[time].time = time;
    operations[time].V = V_;
  }

  void VoiceleadingNode::L(double time, bool avoidParallels)
  {
    operations[time].time = time;
    operations[time].L = true;
    operations[time].avoidParallels = avoidParallels;
  }

  void VoiceleadingNode::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &coordinates)
  {
    transform(score);
  }
}
