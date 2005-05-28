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
#ifndef LINDENMAYER_TO_SOUND_H
#define LINDENMAYER_TO_SOUND_H

#ifdef SWIG
%module CsoundVST
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
        * This class implements a Lindenmayer system in music space
        * for a turtle that writes either notes into a score, or Jones-Parks grains into a memory soundfile.
        * The Z dimension of note space is used for chirp rate.
        * The actions of the turtle are rescaled to fit the specified bounding hypercube.
        * The turtle commands are represented by letters (all n default to 1):
        * <ul>
        * <li>G     = Write the current state of the turtle into the soundfile as a grain.</li>
        * <li>Mn    = Translate the turtle by adding to its state its step times its orientation times n.</li>
        * <li>Rabn  = Rotate the turtle from dimension a to dimension b by angle 2 pi / (angleCount * n)</li>
        * <li>Uan   = Vary the turtle state on dimension a by a normalized (-1 through +1) uniformly distributed random variable times n.</li>
        * <li>Gan   = Vary the turtle state on dimension a by a normalized (-1 through +1) Gaussian random variable times n.</li>
        * <li>T=an  = Assign to dimension a of the turtle state the value n.</li>
        * <li>T*an  = Multiply dimension a of the turtle state by n.</li>
        * <li>T/an  = Divide dimension a of the turtle state by n.</li>
        * <li>T+an  = Add to dimension a of the turtle state the value n.</li>
        * <li>T-an  = Subtract from dimension a of the turtle state the value n.</li>
        * <li>S=an  = Assign to dimension a of the turtle step the value n.</li>
        * <li>S*an  = Multiply dimension a of the turtle step by n.</li>
        * <li>S/an  = Divide dimension a of the turtle step by n.</li>
        * <li>S+an  = Add to dimension a of the turtle step the value n.</li>
        * <li>S-an  = Subtract from dimension a of the turtle step the value n.</li>
        * <li>[     = Push the current state of the turtle state onto a stack.</li>
        * <li>]     = Pop the current state of the turtle from the stack.</li>
        * </ul>
        */
        class Lindenmayer :
                public ScoreNode
        {
        protected:
                int iterationCount;
                double angle;
                std::string axiom;
                Event turtle;
                Event turtleStep;
                Event turtleOrientation;
                std::map<std::string, std::string> rules;
                std::stack<Event> turtleStack;
                std::stack<Event> turtleStepStack;
                std::stack<Event> turtleOrientationStack;
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
                Lindenmayer();
                virtual ~Lindenmayer();
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
