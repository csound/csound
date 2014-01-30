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
#include "Score.hpp"
#include "Voicelead.hpp"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <boost/format.hpp>
#include <sstream>
#if defined(HAVE_IO_H)
#ifdef LINUX
#include <sys/io.h>
#else
#include <io.h>
#endif
#endif
#include <stdio.h>
using boost::format;
using boost::io::group;

namespace csound
{
  ChordLindenmayer::ChordLindenmayer() :
    iterationCount(0),
    angle(1.0),
    beganAt(uintmax_t(0)),
    endedAt(uintmax_t(0)),
    elapsed(uintmax_t(0))
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
    System::inform("BEGAN ChordLindenmayer::generate()...\n");
    System::inform("      ChordLindenmayer::initialize()...\n");
    initialize();
    System::inform("      ChordLindenmayer::generateLindenmayerSystem()...\n");
    generateLindenmayerSystem();
    System::inform("      ChordLindenmayer::writeScore()...\n");
    writeScore();
    System::inform("      ChordLindenmayer::writeScore(): %d events.\n", score.size());
    System::inform("      ChordLindenmayer::tieOverlappingNotes()...\n");
    tieOverlappingNotes();
    System::inform("      ChordLindenmayer::applyVoiceleadingOperationse()...\n");
    applyVoiceleadingOperations();
    System::inform("      ChordLindenmayer::tieOverlappingNotes()...\n");
    tieOverlappingNotes();
    System::inform("      ChordLindenmayer::fixStatus()...\n");
    fixStatus();
    System::inform("ENDED ChordLindenmayer::generate(): %d events.\n", score.size());
  }

  void ChordLindenmayer::initialize()
  {
    turtle.initialize();
    while(!turtleStack.empty()) {
      turtleStack.pop();
    }
    score.clear();
  }

  void ChordLindenmayer::generateLindenmayerSystem()
  {
    std::stringstream source;
    std::stringstream target;
    std::string word;
    std::string rewrittenWord;
    target.str(axiom);
    for (int i = 0; i < iterationCount; i++)
      {
        source.str(target.str());
        source.clear();
        source.seekg(0);
        target.str("");
        target.clear();
        target.seekp(0);
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
            target << rewrittenWord << " ";
          }
      }
    production = target.str();
  }

  void ChordLindenmayer::writeScore()
  {
    std::string command;
    std::stringstream stream(production);
    while (!stream.eof()) {
      stream >> command;
      //std::cerr << command << std::endl;
      interpret(command);
    }
  }

  void ChordLindenmayer::fixStatus()
  {
    for(std::vector<Event>::iterator it = score.begin(); it != score.end(); ++it) {
      if (it->getStatusNumber() == 0.0) {
        it->setStatus(144.0);
      }
    }
  }

  void ChordLindenmayer::tieOverlappingNotes()
  {
    score.tieOverlappingNotes();
  }

  void ChordLindenmayer::applyVoiceleadingOperations()
  {
    transform(score, false);
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
          // the rangeBass will be applied only at the final stage.
          value = Conversions::modulus(value, turtle.rangeSize);
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
                                      size_t &dimension,
                                      size_t &dimension1,
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
    } else if (o == ']') {
      operation = o;
    } else if (std::strpbrk(command_, "FM") == command_) {
      operation = o;
      scalar = Conversions::stringToDouble(command.substr(1));
    } else if (o == 'R') {
      operation = o;
      target = command[1];
      dimension = getDimension(command[2]);
      dimension1 = getDimension(command[3]);
      if (command.length() > 4) {
        scalar =  Conversions::stringToDouble(command.substr(4));
      }
    } else if (std::strpbrk(command_, "=+-*/") == command_) {
      operation = o;
      target = command[1];
      if (target == 'V') {
        scalar = Conversions::stringToDouble(command.substr(2));
      } else if ((target == 'C') || (target == 'M')) {
        // Operations on chords can operate on vectors of pitches;
        // on Jazz-style chord names;
        // or on any individual voice of the chord.
        equivalenceClass = command[2];
        if (command[3] == '(') {
          Conversions::stringToVector(command.substr(4), vector);
        } else if (command[3] == '"') {
          std::string temp = command.substr(3);
          vector = Conversions::nameToPitches(Conversions::trimQuotes(temp));
        } else {
          dimension = getDimension(command[3]);
          if (command.length() > 4) {
            scalar =  Conversions::stringToDouble(command.substr(4));
          }
        }
      } else if ((target == 'N') or (target == 'S')) {
        equivalenceClass = command[2];
        dimension = getDimension(command[3]);
        if (command.length() > 4) {
          scalar =  Conversions::stringToDouble(command.substr(4));
        }
      }
    } else if (o == 'I') {
      operation = o;
      target = command[1];
      scalar = Conversions::stringToDouble(command.substr(2));
    } else if (o == 'T') {
      operation = o;
      target = command[1];
      scalar = Conversions::stringToDouble(command.substr(2));
    } else if (o == 'K') {
      operation = o;
      target = command[1];
    } else if (o == 'Q') {
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
          Event orientedStep;
          for (size_t i = 0, n = turtle.note.size(); i < n; ++i) {
            turtle.note[i] = turtle.note[i] + ((turtle.step[i] * turtle.orientation[i]) * scalar);
          }
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
        }
      case '-':
        {
          switch(target)
            {
            case 'N':
              {
                turtle.note[dimension] = turtle.note[dimension] - (turtle.step[dimension] * scalar);
                equivalence(turtle.note[dimension], equivalenceClass);
              }
              break;
            case 'S':
              {
                turtle.step[dimension] = turtle.step[dimension] - scalar;
                equivalence(turtle.step[dimension], equivalenceClass);
              }
              break;
            case 'O':
              {
                turtle.orientation[dimension] = turtle.orientation[dimension] - scalar;
                equivalence(turtle.orientation[dimension], equivalenceClass);
              }
              break;
            case 'C':
              {
                size_t vectorN = std::min(vector.size(), turtle.chord.size());
                for (size_t vectorI = 0; vectorI < vectorN; ++vectorI) {
                  turtle.chord[vectorI] = turtle.chord[vectorI] - vector[vectorI];
                }
              }
              break;
            case 'M':
              {
                size_t vectorN = std::min(vector.size(), turtle.chord.size());
                for (size_t vectorI = 0; vectorI < vectorN; ++vectorI) {
                  turtle.modality[vectorI] = turtle.modality[vectorI] - vector[vectorI];
                }
              }
              break;
            case 'V':
              {
                turtle.voicing = turtle.voicing - scalar;
                equivalence(turtle.voicing, equivalenceClass);
              }
              break;
            case 'B':
              {
                turtle.rangeBass = turtle.rangeBass - scalar;
                equivalence(turtle.rangeBass, equivalenceClass);
              }
              break;
            case 'R':
              {
                turtle.rangeSize = turtle.rangeSize - scalar;
                equivalence(turtle.rangeSize, equivalenceClass);
              }
              break;
            }
          break;
        }
        break;
      case '*':
        {
          switch(target)
            {
            case 'N':
              {
                turtle.note[dimension] = turtle.note[dimension] * (turtle.step[dimension] * scalar);
                equivalence(turtle.note[dimension], equivalenceClass);
              }
              break;
            case 'S':
              {
                turtle.step[dimension] = turtle.step[dimension] * scalar;
                equivalence(turtle.step[dimension], equivalenceClass);
              }
              break;
            case 'O':
              {
                turtle.orientation[dimension] = turtle.orientation[dimension] * scalar;
                equivalence(turtle.orientation[dimension], equivalenceClass);
              }
              break;
            case 'C':
              {
                size_t vectorN = std::min(vector.size(), turtle.chord.size());
                for (size_t vectorI = 0; vectorI < vectorN; ++vectorI) {
                  turtle.chord[vectorI] = turtle.chord[vectorI] * vector[vectorI];
                }
              }
              break;
            case 'M':
              {
                size_t vectorN = std::min(vector.size(), turtle.chord.size());
                for (size_t vectorI = 0; vectorI < vectorN; ++vectorI) {
                  turtle.modality[vectorI] = turtle.modality[vectorI] * vector[vectorI];
                }
              }
              break;
            case 'V':
              {
                turtle.voicing = turtle.voicing * scalar;
                equivalence(turtle.voicing, equivalenceClass);
              }
              break;
            case 'B':
              {
                turtle.rangeBass = turtle.rangeBass * scalar;
                equivalence(turtle.rangeBass, equivalenceClass);
              }
              break;
            case 'R':
              {
                turtle.rangeSize = turtle.rangeSize * scalar;
                equivalence(turtle.rangeSize, equivalenceClass);
              }
              break;
            }
          break;
        }
        break;
      case '/':
        {
          switch(target)
            {
            case 'N':
              {
                turtle.note[dimension] = turtle.note[dimension] / (turtle.step[dimension] * scalar);
                equivalence(turtle.note[dimension], equivalenceClass);
              }
              break;
            case 'S':
              {
                turtle.step[dimension] = turtle.step[dimension] / scalar;
                equivalence(turtle.step[dimension], equivalenceClass);
              }
              break;
            case 'O':
              {
                turtle.orientation[dimension] = turtle.orientation[dimension] / scalar;
                equivalence(turtle.orientation[dimension], equivalenceClass);
              }
              break;
            case 'C':
              {
                size_t vectorN = std::min(vector.size(), turtle.chord.size());
                for (size_t vectorI = 0; vectorI < vectorN; ++vectorI) {
                  turtle.chord[vectorI] = turtle.chord[vectorI] / vector[vectorI];
                }
              }
              break;
            case 'M':
              {
                size_t vectorN = std::min(vector.size(), turtle.chord.size());
                for (size_t vectorI = 0; vectorI < vectorN; ++vectorI) {
                  turtle.modality[vectorI] = turtle.modality[vectorI] / vector[vectorI];
                }
              }
              break;
            case 'V':
              {
                turtle.voicing = turtle.voicing / scalar;
                equivalence(turtle.voicing, equivalenceClass);
              }
              break;
            case 'B':
              {
                turtle.rangeBass = turtle.rangeBass / scalar;
                equivalence(turtle.rangeBass, equivalenceClass);
              }
              break;
            case 'R':
              {
                turtle.rangeSize = turtle.rangeSize / scalar;
                equivalence(turtle.rangeSize, equivalenceClass);
              }
              break;
            }
          break;
        }
        break;
      case 'R':
        {
          Eigen::MatrixXd rotation = createRotation(dimension, dimension1, scalar);
          turtle.orientation = rotation * turtle.orientation;
        }
        break;
      case 'T':
        {
          turtle.chord = Voicelead::T(turtle.chord, scalar);
        }
        break;
      case 'I':
        {
          turtle.chord = Voicelead::I(turtle.chord, scalar);
        }
        break;
      case 'K':
        {
          turtle.chord = Voicelead::K(turtle.chord);
        }
        break;
      case 'Q':
        {
          turtle.chord = Voicelead::Q(turtle.chord, scalar, turtle.modality);
        }
        break;
      default:
        {
          if        (operation == "VC+") {
            std::vector<double> temp = turtle.chord;
            std::sort(temp.begin(), temp.end());
            if (turtle.chord.size()){
              turtle.chord.push_back(temp.front());
            } else {
              turtle.chord.push_back(0.0);
            }
          } else if (operation == "VC-") {
            if (turtle.chord.size() > 0) {
              turtle.chord.resize(turtle.chord.size() - 1);
            }
          } else if (operation == "WN") {
            score.append(turtle.note);
          } else if (operation == "WCV") {
            std::vector<double> ptv = Voicelead::chordToPTV(turtle.chord,
                                                            0,
                                                            turtle.rangeSize);
            turtle.chord = Voicelead::ptvToChord(ptv[0],
                                                 ptv[1],
                                                 turtle.voicing,
                                                 0,
                                                 turtle.rangeSize);
            for (size_t i = 0, n = turtle.chord.size(); i < n; ++i) {
              Event event = turtle.note;
              event.setKey(turtle.chord[i]);
              score.append(event);
            }
          } else if (operation == "WCNV") {
            std::vector<double> ptv = Voicelead::chordToPTV(turtle.chord,
                                                            0,
                                                            turtle.rangeSize);
            ptv[1] = Voicelead::T(ptv[1], turtle.note.getKey());
            turtle.chord = Voicelead::ptvToChord(ptv[0],
                                                 ptv[1],
                                                 turtle.voicing,
                                                 0,
                                                 turtle.rangeSize);
            for (size_t i = 0, n = turtle.chord.size(); i < n; ++i) {
              Event event = turtle.note;
              event.setKey(turtle.chord[i]);
              score.append(event);
            }
            //    } else if (operation == "WCL") {
            //    } else if (operation == "WCNL") {
          } else if (operation == "AC") {
            std::vector<double> ptv = Voicelead::chordToPTV(turtle.chord,
                                                            0,
                                                            turtle.rangeSize);
            PT(turtle.note.getTime(), ptv[0], ptv[1]);
            //    } else if (operation == "ACV") {
          } else if (operation == "ACN") {
            std::vector<double> ptv = Voicelead::chordToPTV(turtle.chord,
                                                            0,
                                                            turtle.rangeSize);
            ptv[1] = Voicelead::T(ptv[1], turtle.note.getKey());
            PT(turtle.note.getTime(), ptv[0], ptv[1]);
            //    } else if (operation == "ACNV") {
          } else if (operation == "ACL") {
            std::vector<double> ptv = Voicelead::chordToPTV(turtle.chord,
                                                            0,
                                                            turtle.rangeSize);
            PTL(turtle.note.getTime(), ptv[0], ptv[1]);
          } else if (operation == "ACNL") {
            std::vector<double> ptv = Voicelead::chordToPTV(turtle.chord,
                                                            0,
                                                            turtle.rangeSize);
            ptv[1] = Voicelead::T(ptv[1], turtle.note.getKey());
            PTL(turtle.note.getTime(), ptv[0], ptv[1]);
          } else if (operation == "A0") {
            // Creates an uninitialized operation, which does nothing,
            // thus ending the prior operation.
            operations[turtle.note.getTime()].beginTime = turtle.note.getTime();
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

  Eigen::MatrixXd ChordLindenmayer::createRotation (int dimension1, int dimension2, double angle) const
  {
    Eigen::MatrixXd rotation_ = Eigen::MatrixXd::Identity(Event::ELEMENT_COUNT, Event::ELEMENT_COUNT);
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
    score.clear();
  }

  void ChordLindenmayer::produceOrTransform(Score &collectingScore,
                                            size_t beginAt,
                                            size_t endAt,
                                            const Eigen::MatrixXd &compositeCoordinates)
  {
    // Begin at the end of the score generated so far.
    size_t collectingScoreI = collectingScore.size();
    // Allocate all new notes at once.
    collectingScore.resize(collectingScore.size() + score.size());
    for (size_t scoreI = 0, collectingScoreN = collectingScore.size();
         collectingScoreI < collectingScoreN;
         ++scoreI, ++collectingScoreI) {
      collectingScore[collectingScoreI] = compositeCoordinates * score[scoreI];
    }
  }
}

