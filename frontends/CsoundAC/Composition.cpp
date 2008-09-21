/**
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
#if defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning (disable:4786)
#endif
#include "Composition.hpp"
#include "System.hpp"

namespace csound
{
  Composition::Composition() :
    tonesPerOctave(12.0),
    conformPitches(false),
    cppSound(&cppSound_)
  {
  }

  Composition::~Composition()
  {
  }

  void Composition::render()
  {
    generate();
    createCsoundScore(csoundScoreHeader);
    perform();
  }

  void Composition::createCsoundScore(std::string addToScore, double extendSeconds)
  {
    System::inform("addToScore.length(): %d\n", addToScore.length());
    if (addToScore.length() > 2) {
      cppSound->removeScore();
      cppSound->addScoreLine(addToScore);
    }
    cppSound->addScoreLine(score.getCsoundScore(tonesPerOctave, conformPitches));
    char buffer[0x100];
    std::sprintf(buffer, "\ns %9.3f", extendSeconds);
    cppSound->addScoreLine(buffer);
    std::sprintf(buffer, "\ne %9.3f", extendSeconds);
    cppSound->addScoreLine(buffer);
    cppSound->exportForPerformance();
  }

  void Composition::perform()
  {
    cppSound->perform();
  }

  void Composition::generate()
  {
  }

  void Composition::clear()
  {
    score.clear();
    cppSound->removeScore();
  }

  Score &Composition::getScore()
  {
    return score;
  }

  void Composition::setCppSound(CppSound *cppSound)
  {
    if(!cppSound)
      {
        this->cppSound = &cppSound_;
      }
    else
      {
        this->cppSound = cppSound;
      }
  }

  CppSound *Composition::getCppSound()
  {
    return cppSound;
  }

  void Composition::write(const char *text)
  {
    System::message(text);
  }

  void Composition::setTonesPerOctave(double tonesPerOctave)
  {
    this->tonesPerOctave = tonesPerOctave;
  }

  double Composition::getTonesPerOctave() const
  {
    return tonesPerOctave;
  }

  void Composition::setConformPitches(bool conformPitches)
  {
    this->conformPitches = conformPitches;
  }

  bool Composition::getConformPitches() const
  {
    return conformPitches;
  }

  void Composition::setCsoundOrchestra(std::string orchestra)
  {
    cppSound->setOrchestra(orchestra);
  }

  std::string Composition::getCsoundOrchestra() const
  {
    return cppSound->getOrchestra();
  }

  void Composition::setCsoundScoreHeader(std::string header)
  {
    csoundScoreHeader = header;
  }

  std::string Composition::getCsoundScoreHeader() const
  {
    return csoundScoreHeader;
  }

  void Composition::arrange(int oldInstrumentNumber, int newInstrumentNumber)
  {
    score.arrange(oldInstrumentNumber, newInstrumentNumber);
  }

  void Composition::arrange(int oldInstrumentNumber,
                            int newInstrumentNumber,
                            double gain)
  {
    score.arrange(oldInstrumentNumber, newInstrumentNumber, gain);
  }

  void Composition::arrange(int oldInstrumentNumber,
                            int newInstrumentNumber,
                            double gain,
                            double pan)
  {
    score.arrange(oldInstrumentNumber, newInstrumentNumber, gain, pan);
  }

  void Composition::arrange(int silenceInstrumentNumber, std::string csoundInstrumentName)
  {
    int csoundInstrumentNumber = cppSound->getInstrumentNumber(csoundInstrumentName);
    arrange(silenceInstrumentNumber, csoundInstrumentNumber);
  }

  void Composition::arrange(int silenceInstrumentNumber, std::string csoundInstrumentName, double gain)
  {
    int csoundInstrumentNumber = cppSound->getInstrumentNumber(csoundInstrumentName);
    arrange(silenceInstrumentNumber, csoundInstrumentNumber, gain);
   }

  void Composition::arrange(int silenceInstrumentNumber, std::string csoundInstrumentName, double gain, double pan)
  {
    int csoundInstrumentNumber = cppSound->getInstrumentNumber(csoundInstrumentName);
    arrange(silenceInstrumentNumber, csoundInstrumentNumber, gain, pan);
  }

  void Composition::removeArrangement()
  {
    score.removeArrangement();
  }

  void Composition::setCsoundCommand(std::string command)
  {
    cppSound->setCommand(command);
  }

  std::string Composition::getCsoundCommand() const
  {
    return cppSound->getCommand();
  }
}
