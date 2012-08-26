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
#ifndef CHORDLINDENMAYER_TO_SOUND_H
#define CHORDLINDENMAYER_TO_SOUND_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%include "std_string.i"
%include "std_vector.i"
%include "std_map.i"
%{
#include "Conversions.hpp"
#include "Event.hpp"
#include "Score.hpp"
#include "Node.hpp"
#include "Voicelead.hpp"
#include "VoiceleadingNode.hpp"
#include "System.hpp"
#include <sstream>
#include <stack>
#include <string>
#include <map>
#include <vector>
#include <eigen3/Eigen/Dense>
%}
%template(StringMap) std::map<std::string, std::string>;
#else
#include "Conversions.hpp"
#include "Event.hpp"
#include "Score.hpp"
#include "Node.hpp"
#include "Voicelead.hpp"
#include "VoiceleadingNode.hpp"
#include "System.hpp"
#include <sstream>
#include <stack>
#include <string>
#include <map>
#include <vector>
#include <eigen3/Eigen/Dense>
#endif

namespace csound
{
  extern void SILENCE_PUBLIC printChord(std::ostream &stream, std::string label, const std::vector<double> &chord);

  struct SILENCE_PUBLIC Turtle
  {
    Event note;
    Event step;
    Event orientation;
    std::vector<double> chord;
    double rangeBass;
    double rangeSize;
    double voicing;
    std::vector<double> modality;
    Turtle()
    {
      initialize();
    }
    Turtle(const Turtle &other)
    {
      *this = other;
    }
    void initialize()
    {
      note = csound::Event();
      step = csound::Event();
      for(size_t i = 0; i < Event::HOMOGENEITY; i++)
        {
          step[i] = 1.0;
        }
      orientation = csound::Event();
      orientation[Event::TIME] = 1.0;
      chord.clear();
      modality.clear();
      rangeBass = 36;
      rangeSize = 60;
      voicing = 0;
      modality = Conversions::nameToPitches("C Major");
    }
    Turtle &operator = (const Turtle &other)
    {
      note = other.note;
      step = other.step;
      orientation = other.orientation;
      chord = other.chord;
      rangeBass = other.rangeBass;
      rangeSize = other.rangeSize;
      voicing = other.voicing;
      modality = other.modality;
      return *this;
    }
    bool operator < (const Turtle &other) const
    {
      if (note < other.note) {
        return true;
      } else if (other.note < note) {
        return false;
      }
      if (step < other.step) {
        return true;
      } else if (other.step < step) {
        return false;
      }
      if (orientation < other.orientation) {
        return true;
      } else if (other.orientation < orientation) {
        return false;
      }
      if (chord < other.chord) {
        return true;
      } else if (other.chord < chord) {
        return false;
      }
      if (rangeBass < other.rangeBass) {
        return false;
      } else if (other.rangeBass < rangeBass) {
        return true;
      }
      if (rangeSize < other.rangeSize) {
        return true;
      } else if (other.rangeSize < rangeSize) {
        return false;
      }
      if (voicing < other.voicing) {
        return true;
      } else if (other.voicing < voicing) {
        return false;
      }
      if (modality < other.modality) {
        return true;
      }
      return false;
    }
    virtual std::string __str__() const
    {
      std::stringstream stream;
      stream << "Turtle:       " << std::endl;
      stream << " note:        " << note.toString() << std::endl;
      stream << " step:        " << step.toString() << std::endl;
      stream << " orientation: " << orientation.toString() << std::endl;
      printChord(stream, " chord:       ", chord);
      stream << " rangeBass:   " << rangeBass << std::endl;
      stream << " rangeSize:   " << rangeSize << std::endl;
      stream << " voicing:     " << voicing << std::endl;
      printChord(stream, " modality:    ", modality);
      return stream.str();
    }
  };

  struct SILENCE_PUBLIC Command
  {
    char operation;
    char target;
    char equivalence;
    int dimension;
    double x;
    std::vector<double> v;
  };

  /**
   * This class implements a Lindenmayer system that generates a score 
   * by moving a turtle around in various implicit music spaces.
   * 
   * The turtle consists of:
   * <ul>
   * <li> N, a note, i.e. a vector of real numbers in score space.</li>
   * <li> S, a step, i.e. an increment by which to move N 
   *      (also a vector in score space).</li>
   * <li> O, an orientation, i.e. a direction to move N 
   *      (also a  vector).</li>
   * <li> C, a chord, i.e. a vector of voices in chord space.</li>
   * <li> M, a modality used as a reference for neo-Riemannian 
   *      operations upon chords (also a vector).</li>
   * <li> V, a chord voicing, i.e. the index of the octavewise
   *      permutation of C within a range.</li>
   * <li> B, the bass of the range.</li>
   * <li> R, the size of the range.</li>
   * </ul>
   *
   * In accordance with both mathematical music theory and the practice 
   * of composers, operations on elements of music take place in spaces
   * whose geometry changes fluidly depending upon the musical context.
   * A paradigmatic example is transposition, which may apply
   * to individual notes, or to chords, or to larger parts of scores; 
   * as an even more indicative example, transposition may apply 
   * to pitch under octave equivalence (pitch-classes), to pitch under 
   * range equivalence (transposition on a staff), or simply to pitch 
   * as a real number.
   *
   * Consequently, the independent parts of an operation in this 
   * Lindenmayer system are specified by commands in the format 
   * OTEDX, where:
   * <ul>
   * <li>O = the operation proper (e.g. sum or product).</li> 
   * <li>T = the target, or part of the turtle to which the
   *         operation applies, and which has an implicit rank 
   *         (e.g. scalar, vector, tensor).</li>
   * <li>E = its equivalence class (e.g. octave or range).</li> 
   * <li>D = the individual dimension of the operation 
   *         (e.g. pitch or time).</li>
   * <li>X = the operand.</li>
   * </ul>
   *
   * Of course, some operations apply in all ranks, dimensions, and 
   * equivalence classes; other operations, only to one dimension 
   * or one class.
   *
   * Commands are as follows (x is a real scalar; 
   * for chords, v is a real vector "(x1,..,xn)" or a jazz-style chord name ("F#7b9")):
   * <ul> 
   * <li> [     = Push the active turtle onto a stack (start a branch).</li>
   * <li> ]     = Pop the active turtle from the stack (return to the branching point).</li>
   * <li> Fx    = Move the turtle "forward" x steps along its current orientation:
   *              N := N + (S * O) * x.</li>
   * <li> ROdex = Rotate the turtle orientation from dimension d to dimension e by angle x:</li>
   *              R = makeRotation(d, e, x); O := R * O.</li>
   * <li> oNEdx = Apply algebraic operation o to turtle note dimension d with operand x:
   *              N[d] := N[d] + S[d] o x.</li>
   * <li> oSEdx = Apply algebraic operation o to turtle step dimension d with operand x:
   *              S[d] := S[d] o x.</li>
   * <li> oCEix = Apply algebraic operation o to voice i of the turtle chord with operand x:
   *              C[i] := C[i] o x.</li>
   * <li> oCEv  = Apply algebraic operation o to the turtle chord with operand x:
   *              C := C o x (x may be a vector or chord name).</li>
   * <li> oMEv  = Apply algebraic operation o to the turtle modality with operand x:
   *              M := M o x (x may be a vector or chord name).</li>
   * <li> oVx =   Apply algebraic operation o to the voicing index of the turtle chord with operand x:
   *              V := V o x. Of necessity the equivalence class is the range of the score.</li>
   * <li> ICOx  = Invert the turtle chord by reflecting it around pitch-class x.</li>
   * <li> KCO   = Apply Neo-Riemannian inversion by exchange to the turtle chord.</li>
   * <li> QCOx  = Apply Neo-Riemannian contextual transposition by x pitch-classes 
   *              (with reference to the turtle's modality) to the turtle chord.</li>
   * <li> VC+   = Add a voice (doubling the root) to the turtle chord.</li>
   * <li> VC-   = Remove a voice from the turtle chord.</li>
   * <li> WN    = Write the current turtle note to the score.</li>
   * <li> WCV   = Write the current turtle chord and voicing to the score.</li>
   * <li> WCNV  = Write the current turtle chord and voicing to the score, 
   *              after first applying the turtle note to each voice in the chord.</li>
   * <li> AC    = Apply the current turtle chord to the score, starting 
   *              at the current time and continuing to the next A command.</li>
   * <li> ACN   = Apply the current turtle chord to the score, 
   *              after first applying the turtle note to each voice in the chord, starting 
   *              at the current time and continuing to the next A command.</li>
   * <li> ACL   = Apply the current turtle chord to the score, using the closest voice-leading
   *              from the previous chord (if any), starting 
   *              at the current time and continuing to the next A command.</li>
   * <li> ACNL  = Apply the current turtle chord to the score, after first applying the turtle
   *              note to each voice in the chord, using the closest voice-leading from the
   *              previous chord (if any), starting 
   *              at the current time and continuing to the next A command.</li>
   * <li> A0    = End application of the previous A command.</li>
   * </ul>
   * Dimensions of notes:
   * <ol>
   * <li>i = instrument.</li>
   * <li>t = time.</li>
   * <li>d = duration.</li>
   * <li>k = MIDI key number.</li>
   * <li>v = MIDI velocity number.</li>
   * <li>p = phase.</li>
   * <li>x = pan.</li>
   * <li>y = height.</li>
   * <li>z = depth.</li>
   * <li>s = pitch-class set as Mason number (deprecated here).</li>
   * </ol>
   * Algebraic operations:
   * <ul>
   * <li>= = Assign.</li>
   * <li>+ = Add.</li>
   * <li>- = Subtract.</li>
   * <li>* = Multiply.</li>
   * <li>/ = Divide.</li>
   * </ul>
   * Equivalence classes:
   * <ul>
   * <li>0 = None.</li>
   * <li>O = The octave (12).</li>
   * <li>R = The range of the turtle.</li>
   * </ul>
   */

  class SILENCE_PUBLIC ChordLindenmayer :
    public VoiceleadingNode
  {
  public:
    ChordLindenmayer();
    virtual ~ChordLindenmayer();
    virtual int getIterationCount() const;
    virtual void setIterationCount(int count);
    virtual double getAngle() const;
    virtual void setAngle(double angle);
    virtual std::string getAxiom() const;
    virtual void setAxiom(std::string axiom);
    virtual void addRule(std::string command, std::string replacement);
    virtual std::string getReplacement(std::string command);
    /**
     * Scores are generated as follows:
     * <ol>
     * <li> The initial value of the turtle is set by the Lindenmayer system.<\li>
     * <li> The Lindenmayer system is rewritten by taking the axiom, parsing it into words,
     *      and replacing each word with the product of a rewriting rule, if one exists, or itself,
     *      if there is no rule. This procedure is iterated for a specified number of times.</li>
     * <li> The finished, rewritten Lindenmayer system is interpreted as a series of commands for 
     *      moving a turtle around in various music spaces to write a score.</li><ol>
     * <li> Notes (N operations) are written directly into the score.</li>
     * <li> Chords (C operations) are written into the score as notes.</li>
     * <li> L and A operations are written into the score as voice-leading operations, 
     *      to be applied after all notes have been generated.</li></ol>
     * <li> Overlapping and directly abutting notes in the score are joined.</li>
     * <li> The L and A operations are actually applied to the score.
     * <li> Overlapping and abutting notes in the score are again joined.</li>
     * </ol>
     */
    virtual void generate();
    virtual void clear();
    Score score;
    int iterationCount;
    double angle;
    std::string axiom;
    std::string production;
    Turtle turtle;
    std::map<std::string, std::string> rules;
    std::stack<Turtle> turtleStack;
    clock_t beganAt;
    clock_t endedAt;
    clock_t elapsed;
    virtual void initialize();
    virtual void generateLindenmayerSystem();
    virtual void writeScore();
    virtual void fixStatus();
    virtual void tieOverlappingNotes();
    virtual void applyVoiceleadingOperations();
    virtual void interpret(std::string command);
    virtual int getDimension (char dimension) const;
    virtual char parseCommand(const std::string &command, 
                              std::string &operation, 
                              char &target, 
                              char &equivalenceClass, 
                              size_t &dimension,
                              size_t &dimension1,
                              double &scalar,
                              std::vector<double> &vector);
    virtual Eigen::MatrixXd createRotation (int dimension1, int dimension2, double angle) const;
    /**
     * Returns the result of applying the equivalence class to the value,
     * both in the argument and as the return value; there may be no effect.
     */
    virtual double equivalence(double &value, char equivalenceClass) const;
    virtual void produceOrTransform(Score &score,
                                    size_t beginAt,
                                    size_t endAt,
                                    const Eigen::MatrixXd &compositeCordinates);
    
  };
}
#endif
