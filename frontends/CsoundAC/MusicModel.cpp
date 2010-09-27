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
#include "MusicModel.hpp"
#include "Exception.hpp"
#include "Composition.hpp"
#include "System.hpp"

namespace csound
{
  MusicModel::MusicModel() :
    cppSound(&cppSound_)
  {
  }

  MusicModel::~MusicModel()
  {
    //clear();
  }

  void MusicModel::initialize()
  {
  }

  void MusicModel::generate()
  {
    cppSound->removeScore();
    if (children.size()) {
      score.clear();
    }
    traverse(getLocalCoordinates(), score);
    System::message("Generated %d events.\n", score.size());
  }

  void MusicModel::render()
  {
    generate();
    createCsoundScore(csoundScoreHeader);
    perform();
  }

  void MusicModel::createCsoundScore(std::string addToScore, double extendSeconds)
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

  void MusicModel::perform()
  {
    cppSound->perform();
  }



  void MusicModel::clear()
  {
    Node::clear();
    MusicModel::clear();
    cppSound->removeScore();
  }

  void MusicModel::setCppSound(CppSound *cppSound)
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

  CppSound *MusicModel::getCppSound()
  {
    return cppSound;
  }

  void MusicModel::setCsoundOrchestra(std::string orchestra)
  {
    cppSound->setOrchestra(orchestra);
  }

  std::string MusicModel::getCsoundOrchestra() const
  {
    return cppSound->getOrchestra();
  }

  void MusicModel::setCsoundScoreHeader(std::string header)
  {
    csoundScoreHeader = header;
  }

  std::string MusicModel::getCsoundScoreHeader() const
  {
    return csoundScoreHeader;
  }

  void MusicModel::arrange(int oldInstrumentNumber, int newInstrumentNumber)
  {
    score.arrange(oldInstrumentNumber, newInstrumentNumber);
  }

  void MusicModel::arrange(int oldInstrumentNumber,
                            int newInstrumentNumber,
                            double gain)
  {
    score.arrange(oldInstrumentNumber, newInstrumentNumber, gain);
  }

  void MusicModel::arrange(int oldInstrumentNumber,
                            int newInstrumentNumber,
                            double gain,
                            double pan)
  {
    score.arrange(oldInstrumentNumber, newInstrumentNumber, gain, pan);
  }

  void MusicModel::arrange(int silenceInstrumentNumber, std::string csoundInstrumentName)
  {
    int csoundInstrumentNumber = cppSound->getInstrumentNumber(csoundInstrumentName);
    arrange(silenceInstrumentNumber, csoundInstrumentNumber);
  }

  void MusicModel::arrange(int silenceInstrumentNumber, std::string csoundInstrumentName, double gain)
  {
    int csoundInstrumentNumber = cppSound->getInstrumentNumber(csoundInstrumentName);
    arrange(silenceInstrumentNumber, csoundInstrumentNumber, gain);
   }

  void MusicModel::arrange(int silenceInstrumentNumber, std::string csoundInstrumentName, double gain, double pan)
  {
    int csoundInstrumentNumber = cppSound->getInstrumentNumber(csoundInstrumentName);
    arrange(silenceInstrumentNumber, csoundInstrumentNumber, gain, pan);
  }

  void MusicModel::removeArrangement()
  {
    score.removeArrangement();
  }

  void MusicModel::setCsoundCommand(std::string command)
  {
    cppSound->setCommand(command);
  }

  std::string MusicModel::getCsoundCommand() const
  {
    return cppSound->getCommand();
  }

  long MusicModel::getThis()
  {
    return (long) this;
  }

  Node *MusicModel::getThisNode()
  {
    return (Node *)this;
  }

}

