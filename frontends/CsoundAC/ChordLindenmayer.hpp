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
%{
#include "Silence.hpp"
#include <stack>
#include <string>
#include <map>
#include <vector>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
  %}
#else
#include "Silence.hpp"
#include <stack>
#include <string>
#include <map>
#include <vector>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
using namespace boost::numeric;
#endif

namespace csound
{
  /**
   * This class implements a Lindenmayer system that generates a score 
   * by moving a turtle around in various implicit music spaces. 
   *
   * The turtle consists of a note (a vector representing a point in score space); 
   * a step (a vector representing an increment in score space); 
   * an orientation (a vector representing a direction in score space); 
   * a modality (a vector of pitch-class sets in chord space);
   * a chord (a vector of pitch-class sets in chord space); a voicing number;
   * and a range (bass and size).
   *
   * The turtle may write either notes, chords, or voicings into the score.
   * When a chord is written, it is voiced by first transposing it to the pitch
   * of the turtle state, and then permuting it octavewise by the voicing number
   * modulo the range; or, it is voice-led from its predecessor.
   * When a voicing is written, that means that all notes in the score beginning
   * at that time, until the next voicing, are conformed to its pitch-class set,
   * or to its pitch-class set and voicing.
   *
   * Domains of operations: reals, octave equivalence, range equivalence.
   * 
   * Commands (if ommitted, x defaults to 1; x is a real scalar; 
   * for chords, x may also be a real vector x1,..,xn or a jazz-style chord name):
   * <ul> 
   * <li> [    = Push the turtle state onto a stack (start a branch).</li>
   * <li> ]    = Pop the turtle state from the stack (return to the branching point).</li>
   * <li> Fx   = Move the turtle "forward" x steps along its current orientation.</li>
   * <li> Tdox = Apply operation o to turtle note dimension d with operand x.</li>
   * <li> Sdox = Apply operation o to turtle step dimension d with operand x.</li>
   * <li> Vvox = Apply operation o to turtle chord voice v with operand x.</li>
   * <li> Rdex = Rotate the turtle orientation from dimension d to dimension e by angle x.</li>
   * <li> MCx  = Set the modality of the turtle to x (e.g. "C Major").
   * <li> CCx  = Set the turtle chord to x (e.g. "0,4,7").
   * <li> ICx  = Invert the turtle chord by reflecting it around pitch-class x.</li>
   * <li> KC   = Apply Neo-Riemannian inversion by exchange to the turtle chord.</li>
   * <li> QCx  = Apply Neo-Riemannian contextual transposition by x pitch-classes 
   *             (with reference to the turtle's modality) to the turtle chord.</li>
   * <li> VC+  = Add a voice (doubling the root) to the turtle chord.</li>
   * <li> VC-  = Remove a voice from the turtle chord.</li>
   * <li> WN   = Write the turtle note to the score.</li>
   * <li> WC   = Write the turtle chord to the score, voiced from there within the range.</li>
   * <li> WCN  = Write the turtle chord to the score, transposed to the turtle note and voiced from there
   *             within the range.</li>
   * </ul>
   * Dimensions of score space (rotatable):
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
   * Additional "dimensions" for operations (not rotatable):
   * <ol>
   * <li>V = voicing (octavewise permutation of a chord modulo its range).</li>
   * <li>B = base of range.</li>
   * <li>R = size of range.</li>
   * </ol>
   * Operations:
   * <ul>
   * <li> = Assign x to the dimension.</li>
   * <li> + Add x to the value of the dimension.</li>
   * <li> - Subtract x from the value of the dimension.</li>
   * <li> * Multiply the value of the dimension by x.</li>
   * <li> / Divide the value of the dimension by x.</li>
   * </ul>
   */
  class ChordLindenmayer :
    public ScoreNode
  {
  protected:
    struct Turtle
    {
      Event note;
      Event step;
      Event orientation;
      std::vector<double> modality;
      std::vector<double> chord;
      int voicing;
      double rangeBase;
      double rangeSize;
    };
    int iterationCount;
    double angle;
    std::string axiom;
    Turtle turtle;
    std::map<std::string, std::string> rules;
    std::stack<Turtle> turtleStack;
    clock_t beganAt;
    clock_t endedAt;
    clock_t elapsed;
    virtual void interpret(std::string command, bool render);
    virtual int getDimension (char dimension) const;
    virtual void rewrite();
    virtual ublas::matrix<double> createRotation (int dimension1, int dimension2, double angle) const;
    virtual void updateActual(Event &event);
    virtual void initialize();
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
    virtual void generate();
    virtual void clear();
  };
}
#endif
