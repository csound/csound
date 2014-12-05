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
#include "Lindenmayer.hpp"
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
  Lindenmayer::Lindenmayer() : iterationCount(0), angle(1.0), beganAt(0), endedAt(0), elapsed(0)
  {
  }

  Lindenmayer::~Lindenmayer()
  {
  }

  std::string Lindenmayer::getReplacement(std::string word)
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

  void Lindenmayer::generate()
  {
    std::string word;
    std::string replacement;
    std::ifstream inputStream;
    std::ofstream outputStream;
    std::string inputFilename = "a.lindenmayer";
    std::string outputFilename = "b.lindenmayer";
    std::string tempFilename;
    outputStream.open(outputFilename.c_str());
    outputStream << axiom.c_str() << std::endl;
    outputStream.close();
    for(int i = 0; i < iterationCount; i++)
      {
        std::ifstream inputStream;
        std::ofstream outputStream;
        tempFilename = inputFilename;
        inputFilename = outputFilename;
        outputFilename = tempFilename;
        unlink(outputFilename.c_str());
        inputStream.open(inputFilename.c_str());
        inputStream.seekg(0, std::ios_base::beg);
        outputStream.open(outputFilename.c_str());
        while(!inputStream.eof())
          {
            inputStream >> word;
            inputStream >> std::ws;
            replacement = getReplacement(word);
            outputStream << replacement << std::endl;
          }
        inputStream.close();
        outputStream.close();
      }
    score.scaleActualMinima = turtle;
    score.scaleActualRanges = turtle;
    initialize();
    inputStream.open(inputFilename.c_str());
    while(!inputStream.eof())
      {
        inputStream >> word;
        interpret(word, false);
      }
    initialize();
    inputStream.close();
    std::ifstream finalInputStream(inputFilename.c_str());
    while(!finalInputStream.eof())
      {
        finalInputStream >> word;
        //std::cerr << word << std::endl;
        interpret(word, true);
      }
    finalInputStream.close();
    for(std::vector<Event>::iterator i = score.begin(); i != score.end(); ++i)
      {
        Event &event = *i;
        event.setStatus(MidiFile::CHANNEL_NOTE_ON);
      }
  }

  void Lindenmayer::initialize()
  {
    turtle = csound::Event();
    turtleStep = csound::Event();
    for(size_t i = 0; i < Event::HOMOGENEITY; i++)
      {
        turtleStep[i] = 1.0;
      }
    turtleOrientation = csound::Event();
    turtleOrientation[Event::TIME] = 1.0;
  }

  void Lindenmayer::interpret(std::string action, bool render)
  {
    try
      {
        action = Conversions::trim(action);
          char command = action[0];
          switch(command)
            {
            case 'N':
              {
                // N
                // 0
                if(render)
                  {
                    Event event = turtle;
                    //score.rescale(event);
                    score.push_back(event);
                  }
                else
                  {
                    updateActual(turtle);
                  }
              }
              break;
            case 'M':
              {
                // Mn
                // 01
                double a = 1.0;
                if(action.length () > 1)
                  {
                    a = Conversions::stringToDouble(action.substr(1));
                  }
                double step;
                for (int i = 0; i < Event::HOMOGENEITY; i++)
                  {
                    step = turtle[i] + (turtleStep[i] * a * turtleOrientation[i]);
                    turtle[i] = step;
                  }
              }
              break;
            case 'R':
              {
                // Rddn
                // 0123
                size_t d1 = getDimension(action[1]);
                size_t d2 = getDimension(action[2]);
                double n = 1.0;
                if(action.length() > 3)
                  {
                    n = Conversions::stringToDouble(action.substr(3));
                  }
                double a = angle * n;
                Eigen::MatrixXd rotation = createRotation (d1, d2, a);
                std::cerr << "Orientation before rotation: " << std::endl;
                for (int i = 0; i < turtleOrientation.size(); i++)
                  {
                    std::cerr << format("%9.3f ") % turtleOrientation(i);
                  }
                std::cerr << std::endl;
                std::cerr << "Rotation for angle " << a << ":" << std::endl;
                for (int i = 0; i < rotation.rows(); i++)
                  {
                    for (int j = 0; j < rotation.cols(); j++ )
                      {
                        std::cerr << format("%9.3f ") % rotation(i, j);
                      }
                    std::cerr << std::endl;
                  }
                turtleOrientation = rotation * turtleOrientation;
                std::cerr << "Orientation after rotation: " << std::endl;
                for (int i = 0; i < turtleOrientation.size(); i++)
                  {
                    std::cerr << format("%9.3f ") % turtleOrientation(i);
                  }
                std::cerr << std::endl;
                std::cerr << std::endl;
              }
              break;
            case 'T':
              {
                // Tdon
                // 0123
                size_t dimension = getDimension(action[1]);
                char operation = action[2];
                double n = 1.0;
                if(action.length() > 3)
                  {
                    n = Conversions::stringToDouble(action.substr(3));
                  }
                switch(operation)
                  {
                  case '=':
                    turtle[dimension] =  (turtleStep[dimension] * n);
                    break;
                  case '*':
                    turtle[dimension] = (turtle[dimension] * (turtleStep[dimension] * n));
                    break;
                  case '/':
                    turtle[dimension] = (turtle[dimension] / (turtleStep[dimension] * n));
                    break;
                  case '+':
                    turtle[dimension] = (turtle[dimension] + (turtleStep[dimension] * n));
                    break;
                  case '-':
                    turtle[dimension] = (turtle[dimension] - (turtleStep[dimension] * n));
                    break;
                  }
                if(dimension == Event::PITCHES)
                  {
                    turtle[dimension] = Conversions::modulus(turtle[dimension], 4096.0);
                      }
              }
              break;
            case 'S':
              {
                // Sdon
                // 0123
                size_t dimension = getDimension(action[1]);
                char operation = action[2];
                double n = 1.0;
                if(action.length() > 3)
                  {
                    n = Conversions::stringToDouble(action.substr(3));
                  }
                switch(operation)
                  {
                  case '=':
                    turtleStep[dimension] = n;
                    break;
                  case '*':
                    turtleStep[dimension] = (turtleStep[dimension] * n);
                    break;
                  case '/':
                    turtleStep[dimension] = (turtleStep[dimension] / n);
                    break;
                  case '+':
                    turtleStep[dimension] = (turtleStep[dimension] + n);
                    break;
                  case '-':
                    turtleStep[dimension] = (turtleStep[dimension] - n);
                    break;
                  }
                //std::cerr << "step for " << dimension << " = " << turtleStep[dimension] << std::endl;
                if(dimension == Event::PITCHES)
                  {
                    turtle[dimension] = Conversions::modulus(turtle[dimension], 4096.0);
                  }
              }
              break;
            case '[':
              {
                Event a = turtle;
                turtleStack.push(a);
                Event b = turtleStep;
                turtleStepStack.push(b);
                Event c = turtleOrientation;
                turtleOrientationStack.push(c);
              }
              break;
            case ']':
              {
                turtle = turtleStack.top();
                turtleStack.pop();
                turtleStep = turtleStepStack.top();
                turtleStepStack.pop();
                turtleOrientation = turtleOrientationStack.top();
                turtleOrientationStack.pop();
              }
              break;
            }
      }
    catch(void *x)
      {
        std::cout << x << std::endl;
      }
  }

  int Lindenmayer::getDimension (char dimension) const
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

  Eigen::MatrixXd Lindenmayer::createRotation (int dimension1, int dimension2, double angle) const
  {
    Eigen::MatrixXd rotation_ = Eigen::MatrixXd::Identity(Event::ELEMENT_COUNT, Event::ELEMENT_COUNT);
    rotation_(dimension1,dimension1) =  std::cos(angle);
    rotation_(dimension1,dimension2) = -std::sin(angle);
    rotation_(dimension2,dimension1) =  std::sin(angle);
    rotation_(dimension2,dimension2) =  std::cos(angle);
    return rotation_;
  }

  void Lindenmayer::updateActual(Event &event)
  {
    for(int i = 0, n = event.size(); i < n; i++)
      {
        if(score.scaleActualMinima[i] < event[i])
          {
            score.scaleActualMinima[i] = event[i];
          }
        if(score.scaleActualRanges[i] >= (score.scaleActualMinima[i] + event[i]))
          {
            score.scaleActualRanges[i] = (score.scaleActualMinima[i] + event[i]);
          }
      }
  }

  void Lindenmayer::rewrite()
  {
    System::inform("BEGAN Lindenmayer::rewrite()...");
    std::stringstream production(axiom);
    std::stringstream priorProduction;
    std::string symbol;
    std::string replacement;
    for (int i = 0; i < iterationCount; i++)
      {
        priorProduction.clear();
        priorProduction << production.str();
        production.clear();
        while (!priorProduction.eof())
          {
            priorProduction >> symbol;
            if(rules.find(symbol) == rules.end())
              {
                replacement = symbol;
              }
            else
              {
                replacement = rules[symbol];
              }
            production << replacement;
          }
      }
    System::inform("ENDED Lindenmayer::rewrite().");
  }

  double Lindenmayer::getAngle() const
  {
    return angle;
  }

  void Lindenmayer::setAngle(double angle)
  {
    this->angle = angle;
  }

  std::string Lindenmayer::getAxiom() const
  {
    return axiom;
  }

  void Lindenmayer::setAxiom(std::string axiom)
  {
    this->axiom = axiom;
  }

  void Lindenmayer::addRule(std::string command, std::string replacement)
  {
    rules[command] = replacement;
  }

  int Lindenmayer::getIterationCount() const
  {
    return iterationCount;
  }

  void Lindenmayer::setIterationCount(int count)
  {
    iterationCount = count;
  }

  void Lindenmayer::clear()
  {
    initialize();
    rules.clear();
    while(!turtleStack.empty())
      {
        turtleStack.pop();
      }
    while(!turtleStepStack.empty())
      {
        turtleStepStack.pop();
      }
    while(!turtleOrientationStack.empty())
      {
        turtleOrientationStack.pop();
      }
  }
}

