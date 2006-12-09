/*
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
#ifndef VOICELEADNODE_H
#define VOICELEADNODE_H

#include "Platform.hpp"
#include "Voicelead.hpp"
#ifdef SWIG
%module CsoundVST
%{
#include "Node.hpp"
#include "Score.hpp"
  %}
#else
#include "Node.hpp"
#include "Score.hpp"
#include "Voicelead.hpp"
using namespace boost::numeric;
#endif

namespace csound
{
  class VoiceleadingOperation 
  {
  public:
    VoiceleadingOperation();
    virtual ~VoiceleadingOperation();
    double time;
    double rescaledTime;
    double P;
    double T;
    double S;
    double V;
    bool L;
    size_t begin;
    size_t end;
    bool avoidParallels;
  };
  
  /**
   * This node class imposes 
   * a sequence of one or more 
   * "voice-leading" operations upon
   * the pitches of notes produced by children of this node.
   * These operations comprise:
   * prime chord (P),
   * transpose (T),
   * unordered pitch-class set (S, equivalent to PT),
   * voicing (V),
   * and voice-lead (L).
   * The values of P, T, S, and V 
   * each form an additive group
   * whose elements are defined 
   * by counting through all possible values in order.
   * Not all combinations of operations are consistent.
   * P requires T.
   * PT cannot be used with S.
   * V cannot be used with L.
   * If neither PT nor S is specified, the existing S of the notes is used.
   * The consistent combinations of operations are thus:
   * PT, PTV, PTL, S, SV, SL, V, and L.
   */
  class SILENCE_PUBLIC VoiceleadingNode :
    public Node
  {
  public:
    std::map<double, VoiceleadingOperation> operations;
    double base;
    double range;
    bool rescaleTimes;
    bool avoidParallels;
    size_t divisionsPerOctave;
    VoiceleadingNode();
    virtual ~VoiceleadingNode();
    /**
     * Applies stored operations to specified ranges of notes produced by children of this node.
     */
    virtual void apply(Score &score, const VoiceleadingOperation &priorOperation, const VoiceleadingOperation &currentOperation);
    virtual void produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &coordinates);
    virtual void PT(double time, double P, double T);
    virtual void PTV(double time, double P, double T, double V);
    virtual void PTL(double time, double P, double T, bool avoidParallels = true);
    virtual void S(double time, double S_);
    virtual void S(double time, std::string S_);
    virtual void SV(double time, double S, double V);
    virtual void SV(double time, std::string S, double V);
    virtual void SL(double time, double S, bool avoidParallels = true);
    virtual void SL(double time, std::string S, bool avoidParallels = true);
    virtual void V(double time, double V_);
    virtual void L(double time, bool avoidParallels = true);
  };
}
#endif

