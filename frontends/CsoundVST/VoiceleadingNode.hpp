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
  class SILENCE_PUBLIC VoiceleadingOperation 
  {
  public:
    VoiceleadingOperation();
    virtual ~VoiceleadingOperation();
    /**
     * The operation begins at this time,
     * and continues until just before the beginning 
     * of the next operation, or the end of the score,
     * whichever comes first.
     */
    double beginTime;
    /**
     * Times may need to be rescaled to
     * match the duration of the score.
     */
    double rescaledBeginTime;
    /**
     * The operation ends before this time.
     */
    double endTime;
    /**
     * Times may need to be rescaled
     * to match the duration of the score.
     */
    double rescaledEndTime;
    double P;
    /**
     * Prime chord, or NAN if no operation.
     */
    double T;
    /**
     * Transposition, or NAN if no operation.
     */
    double C;
    /**
     * Voicing, or NAN if no operation.
     */
    double V;
    /**
     * If true, perform the closest voice-leading from the prior operation.
     */
    bool L;
    /**
     * The index of the first event to which the operation is applied.
     */
    size_t begin;
    /**
     * One past the index of the last event to which the operation is applied.
     */
    size_t end;
    bool avoidParallels;
  };

  std::ostream SILENCE_PUBLIC &operator << (std::ostream &stream, const VoiceleadingOperation &operation);
  
  /**
   * This node class imposes 
   * a sequence of one or more 
   * "voice-leading" operations upon
   * the pitches of notes produced by children of this node.
   * These operations comprise:
   * prime chord (P),
   * transpose (T),
   * unordered pitch-class set (C, equivalent to PT),
   * voicing (V),
   * and voice-lead (L).
   * The values of P, T, C, and V 
   * each form an additive cyclic group
   * whose elements are defined 
   * by counting through all possible values in order.
   * Note that C is not the same as "pitch-class set number"
   * in the sense of M = sum over pitch-classes of (2 ^ pitch-class); 
   * it is rather one less than M.
   * Not all combinations of operations are consistent.
   * P requires T.
   * PT cannot be used with C.
   * V cannot be used with L.
   * If neither PT nor C is specified, the existing C of the notes is used.
   * The consistent combinations of operations are thus:
   * PT, PTV, PTL, C, CV, CL, V, and L.
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
    virtual void PT(double time, double P_, double T);
    virtual void PTV(double time, double P_, double T, double V_);
    virtual void PTL(double time, double P_, double T, bool avoidParallels = true);
    virtual void C(double time, double C_);
    virtual void C(double time, std::string C_);
    virtual void CV(double time, double C_, double V_);
    virtual void CV(double time, std::string C_, double V_);
    virtual void CL(double time, double C_, bool avoidParallels = true);
    virtual void CL(double time, std::string C_, bool avoidParallels = true);
    virtual void V(double time, double V_);
    virtual void L(double time, bool avoidParallels = true);
    /**
     * Simplifies use out of the context of a music graph.
     */
    virtual void transform(Score &score, bool rescaleTime = false);
  };
}
#endif

