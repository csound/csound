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
#ifndef VOICELEADNODE_H
#define VOICELEADNODE_H

#include "Platform.hpp"
#include "Voicelead.hpp"
#ifdef SWIG
%module CsoundAC
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
  /**
   * Utility class for storing voice-leading operations
   * within a VoiceleadNode for future application.
   */
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
    /**
     * Prime chord, or DBL_MAX if no operation.
     */
    double P;
    /**
     * Transposition, or DBL_MAX if no operation.
     */
    double T;
    /**
     * Pitch-set class, or DBL_MAX if no operation.
     */
    double C;
    /**
     * Inversion by interchange.
     */
    double K;
    /**
     * Contextual transposition.
     */
    double Q;
    /**
     * Voicing, or DBL_MAX if no operation.
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
   * the pitches of notes produced by children of this node,
   * within a segment of the notes.
   * These operations comprise:
   * prime chord (P),
   * transpose (T),
   * unordered pitch-class set (C, equivalent to PT),
   * contextual inversion (K),
   * contextual transposition (Q),
   * voicing (V) within a specified range of pitches,
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
   * K and Q require a previous section, and cannot be used with P, T, or C.
   * The consistent combinations of operations are thus:
   * PT, PTV, PTL, C, CV, CL, K, KV, KL, Q, QV, QL, V, and L.
   */
  class SILENCE_PUBLIC VoiceleadingNode :
    public Node
  {
  public:
    /**
     * Voice-leading operations stored in order
     * of starting time.
     */
    std::map<double, VoiceleadingOperation> operations;
    /**
     * The lowest pitch of the range of voicings,
     * as a MIDI key number (default = 36).
     */
    double base;
    /**
     * The range of voicings, from the lowest to the highest
     * pitch, as a MIDI key number (default = 60).
     */
    double range;
    /**
     * Context for the K and Q operations; must have the
     * same cardinality as the pitch-classes in use.
     */
    std::vector<double> modality;
    /**
     * If true (the default), rescale times of voice-leading operations
     * in proportion to the duration of the notes produced
     * by this and its child nodes.
     */
    bool rescaleTimes;
    /**
     * If true (the default), voice-leadings will avoid
     * parallel fifths.
     */
    bool avoidParallels;
    /**
     * The number of equally tempered divisions of the octave (default = 12).
     * Note that the octave is always size 12. The size of a division of the
     * octave is then 1 in 12-tone equal temperament, 0.5 in 24-tone
     * equal temperament, 1.33333 in 9-tone equal temperament, and so on.
     */
    size_t divisionsPerOctave;
    VoiceleadingNode();
    virtual ~VoiceleadingNode();
    /**
     * Apply the current voice-leading operation to the score, within the specified range of notes.
     * If voice-leading proper is to be performed, the prior voice-leading operation is used to determine
     * how to lead the voices.
     */
    virtual void apply(Score &score, const VoiceleadingOperation &priorOperation, const VoiceleadingOperation &currentOperation);
    /**
     * Applies all of the stored voice-leading operations
     * to the specified range of notes in the score.
     * if rescaleTimes is true, the times of the operations
     * will be rescaled to fit the times in the range of notes.
     */
    virtual void produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &coordinates);
    /**
     * Beginning at the specified time and continuing
     * to the beginning of the next operation
     * or the end of the score, whichever comes first,
     * conform notes produced by this node or its children
     * to the specified prime chord and transposition.
     * Note that PT specifies what musicians normally call a chord,
     * e.g. "E flat major ninth." However, chords do not have to be
     * in twelve tone equal temperament.
     */
    virtual void PT(double time, double P_, double T);
    /**
     * Beginning at the specified time and continuing
     * to the beginning of the next operation
     * or the end of the score, whichever comes first,
     * conform notes produced by this node or its children
     * to the specified prime chord, transposition, and voicing.
     * Note that PTV specifies what musicians normally call
     * the voicing, or inversion, of a chord.
     */
    virtual void PTV(double time, double P_, double T, double V_);
    /**
     * Beginning at the specified time and continuing
     * to the beginning of the next operation
     * or the end of the score, whichever comes first,
     * conform notes produced by this node or its children
     * to the specified chord; the voicing of the chord will be
     * the smoothest voice-leading from the pitches of the previous chord.
     * Optionally, parallel fifths can be avoided.
     * Note that PTL specifies what musicians normally call
     * the voice-leading of a chord.
     */
    virtual void PTL(double time, double P_, double T, bool avoidParallels = true);
    /**
     * Beginning at the specified time and continuing
     * to the beginning of the next operation
     * or the end of the score, whichever comes first,
     * conform notes produced by this node or its children
     * to the specified prime chord and transposition.
     * Note that C (equivalent to PT) specifies what musicians normally
     * call a chord.
     */
    virtual void C(double time, double C_);
    /**
     * Same as PT, except that a single number
     * is used in place of the P and T numbers.
     */
    virtual void C(double time, std::string C_);
    /**
     * Same as C, except the chord can be specified by
     * jazz-type name (e.g. EbM7) instead of C number.
     */
    virtual void CV(double time, double C_, double V_);
    /**
     * Same as PTV, except the chord is specified by
     * a single number instead of the P and T numbers.
     */
    virtual void CV(double time, std::string C_, double V_);
    /**
     * Same as CV, except the chord is specified by
     * jazz-type name (e.g. EbM7) instead of C number.
     */
    virtual void CL(double time, double C_, bool avoidParallels = true);
    /**
     * Same as PTL, except the chord is specified by
     * a single number instead of P and T numbers.
     */
    virtual void CL(double time, std::string C_, bool avoidParallels = true);
    /**
     * Find the C of the previous segment, and contextually invert it; apply
     * the resulting C to the current segment. Contextual inversion is
     * that inversion of C in which the first two pitch-classes are exchanged.
     */
    virtual void K(double time);
    /**
     * Find the C of the previous segment, and contextually invert it; apply
     * the resulting C to the current segment with voicing V. Contextual
     * inversion is that inversion of C in which the first two pitch-classes
     * are exchanged.
     */
    virtual void KV(double time, double V_);
    /**
     * Find the C of the previous segment, and contextually invert it; apply
     * the resulting C to the current segment, using the closest voiceleading
     * from the pitches of the previous segment.
     * Contextual inversion is that inversion of C in which the first two
     * pitch-classes are exchanged.
     */
    virtual void KL(double time, bool avoidParallels = true);
    /**
     * Find the C of the previous segment, and contextually transpose it;
     * apply the resulting C to the current segment. Contextual transposition
     * transposes C up by Q if C is an I-form, and down by Q if C is a T-form.
     */
    virtual void Q(double time, double Q_);
    /**
     * Find the C of the previous segment, and contextually transpose it;
     * apply the resulting C to the current segment with voicing V.
     * Contextual transposition transposes C up by Q if C is an I-form,
     * and down by Q if C is a T-form.
     */
    virtual void QV(double time, double Q_, double V_);
    /**
     * Find the C of the previous segment, and contextually transpose it;
     * apply the resulting C to the current segment, using the closest
     * voiceleading from the pitches of the previous segment.
     * Contextual transposition transposes C up by Q if C is an I-form,
     * and down by Q if C is a T-form.
     */
    virtual void QL(double time, double Q_, bool avoidParallels = true);
    /**
     * Beginning at the specified time and continuing
     * to the beginning of the next operation
     * or the end of the score, whichever comes first,
     * conform notes produced by this node or its children
     * to the specified voicing of the chord.
     * Note that V specifies what musicians normally call
     * the voicing or inversion of the chord.
     */
    virtual void V(double time, double V_);
    /**
     * Beginning at the specified time and continuing
     * to the beginning of the next operation
     * or the end of the score, whichever comes first,
     * conform notes produced by this node or its children
     * to the smoothest voice-leading from the pitches
     * of the previous segment.
     * Optionally, parallel fifths can be avoided.
     * Note that L specifies what musicians normally call
     * voice-leading.
     */
    virtual void L(double time, bool avoidParallels = true);
    /**
     * Apply all of the voice-leading operations stored within this
     * node to the score. Enables voice-leading operations to be used
     * outside the context of a music graph.
     */
    virtual void transform(Score &score, bool rescaleTime = false);
    virtual void setModality(const std::vector<double> &pcs);
    virtual std::vector<double> getModality() const;
  };
}
#endif
