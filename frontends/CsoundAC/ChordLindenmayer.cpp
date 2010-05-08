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
#include "CppSound.hpp"
#include "ChordLindenmayer.hpp"
#include "Event.hpp"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <boost/format.hpp>
#include <sstream>
#if defined(HAVE_IO_H)
#include <io.h>
#endif
#include <stdio.h>
using boost::format;
using boost::io::group;

namespace csound
{
  ChordLindenmayer::ChordLindenmayer() : angle(1.0)
  {
  }

  ChordLindenmayer::~ChordLindenmayer()
  {
  }

  std::string ChordLindenmayer::getReplacement(std::string word)
  {
    if(rules.find(word) == rules.end())
      {
        return word;
      }
    else
      {
        return rules[word];
      }
  }

  void ChordLindenmayer::generate()
  {
    initialize();
    generateLindenmayerSystem();
    writeScore();
    tieOverlappingNotes();
    applyVoiceleadingOperations();
    tieOverlappingNotes();
  }
  
  void ChordLindenmayer::initialize()
  {
    turtle.initialize();
    while(!turtleStack.empty()) {
      turtleStack.pop();
    }
  }
  
  void ChordLindenmayer::generateLindenmayerSystem()
  {
    System::inform("BEGAN ChordLindenmayer::generateLindenmayerSystem()...");
    std::stringstream source;
    std::stringstream target(axiom);
    std::string word;
    std::string rewrittenWord;
    for (int i = 0; i < iterationCount; i++)
      {
        source.str(target.str());
        target.str("");
        while (!source.eof())
          {
            source >> word;
            if(rules.find(word) == rules.end())
              {
                rewrittenWord = word;
              }
            else
              {
                rewrittenWord = rules[word];
              }
            target << rewrittenWord;
          }
      }
    production = target.str();
    System::inform("ENDED ChordLindenmayer::generateLindenmayerSystem().");
  }
  
  void ChordLindenmayer::writeScore()
  {
    std::string command;
    std::stringstream stream(production);
    while (!stream.eof()) {
      stream >> command;
      interpret(command);
    }
  }
  
  void ChordLindenmayer::tieOverlappingNotes()
  {
  }
  
  void ChordLindenmayer::applyVoiceleadingOperations()
  {
  }

  double ChordLindenmayer::equivalence(double &value, char equivalenceClass) const
  {
    switch(equivalenceClass)
      {
      case 'O':
	{
	  value = Conversions::modulus(value, 12.0);
	}
	break;
      case 'R':
	{
	  value -= turtle.rangeBass;
	  value = Conversions::modulus(value, turtle.rangeSize);
	  value += turtle.rangeBass;
	}
	break;
      }
    return value;
  }

  /*
   * [
   * ]     
   * Fx    
   * Mv    
   * oNEdx 
   * oSEdx 
   * oCEdx 
   * oCEv  
   * oVx 
   * ROdex 
   * ICOx  
   * KCO   
   * QCOx   
   * VC+
   * VC-
   * WN  
   * WCV 
   * WCNV 
   * WCL 
   * WCNL
   * AC   
   * ACV  
   * ACN  
   * ACNV 
   * ACL 
   * ACNL
   * A0  
   */
  char ChordLindenmayer::parseCommand(const std::string &command, 
				      std::string &operation, 
				      char &target, 
				      char &equivalenceClass, 
				      size_t dimension,
				      size_t dimension1,
				      double &scalar,
				      std::vector<double> &vector)
  {
    const char *command_ = command.c_str();
    char o = command[0];
    operation = "";
    target = 0;
    equivalenceClass = 0;
    dimension = 0;
    dimension1 = 0;
    scalar = 0;
    vector.clear();
    if (o == '[') {
      operation = o;
    } else
    if (o == ']') {
      operation = o;
    } else 
    if (std::strpbrk(command_, "FM") == command) {
      operation = o;
      scalar = Conversions::stringToDouble(command.substr(1));
    } else 
    if (o == 'R') {
      operation = o;
      target = command[1];
      dimension = getDimension(command[2]);
      dimension1 = getDimension(command[3]);
      scalar = Conversions::stringToDouble(command.substr(4));
    } else
    if (std::strpbrk(command_, "=+*/") == command) {
      operation = o;
      target = command[1];
      if (target == 'V') {
	scalar = Conversions::stringToDouble(command.substr(2));
      } else 
      if (target == 'C') {
	// Operations on chords can operate on vectors of pitches; 
	// on Jazz-style chord names; 
	// or on any individual voice of the chord.
	equivalenceClass = command[2];
	if (command[3] == '(') {
	  Conversions::stringToVector(command.substr(4),vector);
	} else if (command[3] == '"') {
	  std::string temp = command.substr(3);
	  vector = Conversions::nameToPitches(Conversions::trimQuotes(temp));
	} else {
	  dimension = getDimension(command[3]);
	  scalar = Conversions::stringToDouble(command.substr(4));
	}
      } else {
	equivalenceClass = command[2];
	dimension = getDimension(command[3]);
	scalar =  Conversions::stringToDouble(command.substr(4));		     
      }
    } else
    if (o == 'I') {
      operation = o;
      target = command[1];
      scalar = Conversions::stringToDouble(command.substr(2));
    } else
    if (o == 'K') {
      operation = o;
      target = command[1];
    } else
    if (o == 'Q') {
      operation = o;
      target = command[1];
      scalar = Conversions::stringToDouble(command.substr(2));
    } else {
      // All other commands take no parameters.
      operation = command;
    }
    return o;
  }

  void ChordLindenmayer::interpret(std::string command)
  {
    /* <ul>
     * <li>O = the operation proper (e.g. sum or product).</li> 
     * <li>T = the target, or part of the turtle to which the
     *         operation applies, and which has an implicit rank 
     *         (e.g. scalar, vector, tensor).</li>
     * <li>E = its equivalence class (e.g. octave or range).</li> 
     * <li>D = the individual dimension of the operation 
     *         (e.g. pitch or time).</li>
     * <li>X = the operand (which defaults to 1).</li>
     * </ul>
     */
    std::string operation; 
    char target;
    char equivalenceClass; 
    size_t dimension;
    size_t dimension1;
    double scalar;
    std::vector<double> vector;
    char o = parseCommand(command, 
			  operation, 
			  target, 
			  equivalenceClass, 
			  dimension, 
			  dimension1, 
			  scalar, 
			  vector);
    switch (o)
      { 
      case '[':
	{
	  turtleStack.push(turtle);
	}
	break;
      case ']':
	{
	  turtle = turtleStack.top();
	  turtleStack.pop();
	}
	break;
      case 'F':
	{
	  boost::numeric::ublas::vector<double> orientedStep = boost::numeric::ublas::element_prod(turtle.step, 
												   turtle.orientation);
	  boost::numeric::ublas::vector<double> scaledOrientedStep = orientedStep * scalar;
	  turtle.note = turtle.note + scaledOrientedStep;
	}
	break;
      case '=':
	{
	  switch(target)
	    {
	    case 'N':
	      {
		turtle.note[dimension] = scalar;
		equivalence(turtle.note[dimension], equivalenceClass);
	      }
	      break;
	    case 'S':
	      {
		turtle.step[dimension] = scalar;
		equivalence(turtle.step[dimension], equivalenceClass);
	      }
	      break;
	    case 'O':
	      {
		turtle.orientation[dimension] = scalar;
		equivalence(turtle.orientation[dimension], equivalenceClass);
	      }
	      break;
	    case 'C':
	      {
		turtle.chord = vector;
	      }
	      break;
	    case 'M':
	      {
		turtle.modality = vector;
	      }
	      break;
	    case 'V':
	      {
		turtle.voicing = scalar;
		equivalence(turtle.voicing, equivalenceClass);
	      }
	      break;
	    case 'B':
	      {
		turtle.rangeBass = scalar;
		equivalence(turtle.rangeBass, equivalenceClass);
	      }
	      break;
	    case 'R':
	      {
		turtle.rangeSize = scalar;
		equivalence(turtle.rangeSize, equivalenceClass);
	      }
	      break;
	    }
	}
	break;
      case '+':
	{
	  switch(target)
	    {
	    case 'N':
	      {
		turtle.note[dimension] = turtle.note[dimension] + (turtle.step[dimension] * scalar);
		equivalence(turtle.note[dimension], equivalenceClass);
	      }
	      break;
	    case 'S':
	      {
		turtle.step[dimension] = turtle.step[dimension] + scalar;
		equivalence(turtle.step[dimension], equivalenceClass);
	      }
	      break;
	    case 'O':
	      {
		turtle.orientation[dimension] = turtle.orientation[dimension] + scalar;
		equivalence(turtle.orientation[dimension], equivalenceClass);
	      }
	      break;
	    case 'C':
	      {
		size_t vectorN = std::min(vector.size(), turtle.chord.size());
		for (size_t vectorI = 0; vectorI < vectorN; ++vectorI) {
		  turtle.chord[vectorI] = turtle.chord[vectorI] + vector[vectorI];
		}
	      }
	      break;
	    case 'M':
	      {
		size_t vectorN = std::min(vector.size(), turtle.chord.size());
		for (size_t vectorI = 0; vectorI < vectorN; ++vectorI) {
		  turtle.modality[vectorI] = turtle.modality[vectorI] + vector[vectorI];
		}
	      }
	      break;
	    case 'V':
	      {
		turtle.voicing = turtle.voicing + scalar;
		equivalence(turtle.voicing, equivalenceClass);
	      }
	      break;
	    case 'B':
	      {
		turtle.rangeBass = turtle.rangeBass + scalar;
		equivalence(turtle.rangeBass, equivalenceClass);
	      }
	      break;
	    case 'R':
	      {
		turtle.rangeSize = turtle.rangeSize + scalar;
		equivalence(turtle.rangeSize, equivalenceClass);
	      }
	      break;
	}
	break;
      case '-':
	{
	}
	break;
      case '*':
	{
	}
	break;
      case '/':
	{
	}
	break;
      case 'R':
	{
	  boost::numeric::ublas::matrix<double> rotation = createRotation(dimension, dimension1, scalar);
	  turtle.orientation = rotation * turtle.orientation;
	}
	break;
      case 'I':
	{
	}
	break;
      case 'K':
	{
	}
	break;
      case 'Q':
	{
	}
	break;
      default:
	{
	  if        (operation == "VC+") {
	  } else if (operation == "VC-") {
	  } else if (operation == "WN") {
	  } else if (operation == "WCV") {
	  } else if (operation == "WCNV") {
	  } else if (operation == "WCL") {
	  } else if (operation == "WCNL") {
	  } else if (operation == "AC") {
	  } else if (operation == "ACV") {
	  } else if (operation == "ACN") {
	  } else if (operation == "ACNV") {
	  } else if (operation == "ACL") {
	  } else if (operation == "ACNL") {
	  } else if (operation == "A0") {
	  }
	}
      }
  }

  int ChordLindenmayer::getDimension (char dimension) const
  {
    switch(dimension)
      {
      case 'i': return Event::INSTRUMENT;
      case 't': return Event::TIME;
      case 'd': return Event::DURATION;
      case 'k': return Event::KEY;
      case 'v': return Event::VELOCITY;
      case 'p': return Event::PHASE;
      case 'x': return Event::PAN;
      case 'y': return Event::HEIGHT;
      case 'z': return Event::DEPTH;
      case 's': return Event::PITCHES;
      }
    return -1;
  }

  ublas::matrix<double> ChordLindenmayer::createRotation (int dimension1, int dimension2, double angle) const
  {
    ublas::matrix<double> rotation_ = ublas::identity_matrix<double>(Event::ELEMENT_COUNT);
    rotation_(dimension1,dimension1) =  std::cos(angle);
    rotation_(dimension1,dimension2) = -std::sin(angle);
    rotation_(dimension2,dimension1) =  std::sin(angle);
    rotation_(dimension2,dimension2) =  std::cos(angle);
    return rotation_;
  }

  double ChordLindenmayer::getAngle() const
  {
    return angle;
  }

  void ChordLindenmayer::setAngle(double angle)
  {
    this->angle = angle;
  }

  std::string ChordLindenmayer::getAxiom() const
  {
    return axiom;
  }

  void ChordLindenmayer::setAxiom(std::string axiom)
  {
    this->axiom = axiom;
  }

  void ChordLindenmayer::addRule(std::string command, std::string replacement)
  {
    rules[command] = replacement;
  }

  int ChordLindenmayer::getIterationCount() const
  {
    return iterationCount;
  }

  void ChordLindenmayer::setIterationCount(int count)
  {
    iterationCount = count;
  }

  void ChordLindenmayer::clear()
  {
    rules.clear();
    while(!turtleStack.empty()) {
      turtleStack.pop();
    }
  }
}

