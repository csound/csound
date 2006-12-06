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

namespace csound
{

  double nameToS(std::string name)
  {
    double pcs = Conversions::nameToPitchClassSet(name);
    return pcs - 1.0;
  }

  VoiceleadingOperation::VoiceleadingOperation() : 
    time(0.0),
    rescaledTime(0.0),
    P(double(0.0) / double(0.0)),
    T(double(0.0) / double(0.0)),
    N(double(0.0) / double(0.0)),
    V(double(0.0) / double(0.0)),
    L(false),
    parallels(false),
    begin(0),
    end(0)
  {
  }
  
  VoiceleadingOperation::~VoiceleadingOperation()
  {
  }
  
  VoiceleadingNode::VoiceleadingNode() : 
    base(36.0),
    range(60.0),
    rescaleTimes(true)
  {
  }

  VoiceleadingNode::~VoiceleadingNode()
  {
  }

  void VoiceleadingNode::apply(Score &score, const VoiceleadingOperation &priorOperation, const VoiceleadingOperation &currentOperation, double nextTime)
  {
    size_t begin = score.indexForTime(currentOperation.time);
    if (currentOperation.P != VoiceleadingOperation::INAPPLICABLE) {
    } else if (currentOperation.S != VoiceleadingOperation::INAPPLICABLE) {
    } else {
    }
  }

  void VoiceleadingNode::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &coordinates)
  {
    // First, rescale the times for the operations, if that is required.
    double timeMinimum;
    double timeRange;
    double timeScale = 1.0;
    double maxTime = operations.rbegin()->first;
    if (rescaleTimes) {
      score::getScale(score, Event::TIME, 0, score.size(), timeMinimum, timeRange);
      timeScale = maxTime / timeRange;
    }
    for (operations.iterator it = operations.begin(); it != operations.end(); ++it) {
      it->rescaledTime = it->time * timeScale;
    }
    // Iterate over the operations, keeping the prior one around.
    VoiceleadingOperation priorOperation = *operations.begin();
    for (operations.const_iterator it = operations.begin(); it != operations.end(); ++it) {
      apply(score, priorOperation, *it);
      priorOperation = *it;
    }
  }

  void VoiceleadingNode::PT(double time, double P, double T)
  {
    operations[time].P = P;
    operations[time].T = T;
  }

  void VoiceleadingNode::PTV(double time, double P, double T, double V)
  {
    operations[time].P = P;
    operations[time].T = T;
    operations[time].V = V;
  }

  void VoiceleadingNode::PTL(double time, double P, double T, bool avoidParallels)
  {
    operations[time].P = P;
    operations[time].T = T;
    operations[time].L = true;
    operations[time].avoidParallels = avoidParallels;
  }

  void VoiceleadingNode::S(double time, double S_)
  {
    operations[time].S = S_;
  }

  void VoiceleadingNode::S(double time, std::string S_)
  {
    S(time, nameToS(S_));
  }

  void VoiceleadingNode::SV(double time, double S, double V)
  {
    operations[time].S = S;
    operations[time].V = V;
  }

  void VoiceleadingNode::SV(double time, std::string S, double V)
  {
    double s = Conversions::chordToNumber(S) - 1.0;
    SV(time, s, V);
  }

  void VoiceleadingNode::SL(double time, double S, bool avoidParallels)
  {
    operations[time].S = S;
    operations[time].L = true;
    operations[time].avoidParallels = avoidParallels;
  }

  void VoiceleadingNode::SL(double time, std::string S, bool avoidParallels)
  {
    SL(time, nameToS(S), avoidParallels);
  }

  void VoiceleadingNode::V(double time, double V_)
  {
    operations[time].V = V_;
  }

  void VoiceleadingNode::L(double time, bool avoidParallels)
  {
    operations[time].L = true;
    operations[time].avoidParallels = avoidParallels;
  }

}
