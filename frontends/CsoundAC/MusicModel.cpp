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
#include <cstdio>
#include <cstdlib>

namespace csound
{
  MusicModel::MusicModel() :
    cppSound(&cppSound_),
    go(false)
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
    //std::sprintf(buffer, "\ne %9.3f", extendSeconds);
    //cppSound->addScoreLine(buffer);
    cppSound->exportForPerformance();
  }

  void MusicModel::perform()
  {
    go = true;
    cppSound->setCommand(getCsoundCommand());
    createCsoundScore(csoundScoreHeader);
    cppSound->perform();
    // The Csound command is managed from MusicModel,
    // not from CppSound. So we clear out what we set.
    cppSound->setCommand("");
  }

  void MusicModel::clear()
  {
    Node::clear();
    Composition::clear();
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
    std::string command_ = cppSound->getCommand();
    if (command_.size() == 0)
      {
        const char *temp_path = std::getenv("TEMP");
        if (temp_path == 0)
          {
            temp_path = "";
          }
        std::string orcname = std::tmpnam(0);
        std::string sconame = std::tmpnam(0);
        char buffer[0x200];
        std::sprintf(buffer,
                     "csound --midi-key=4 --midi-velocity=5 -m167 -RWdfo %s %s%s.orc %s%s.sco",
                     getOutputSoundfileName().c_str(), temp_path, orcname.c_str(), temp_path, sconame.c_str());
        command_ = buffer;
      }
    return command_;
  }

  long MusicModel::getThis()
  {
    return (long) this;
  }

  Node *MusicModel::getThisNode()
  {
    return (Node *)this;
  }

  void MusicModel::processArgs(const std::vector<std::string> &args)
  {
    System::inform("BEGAN MusicModel::processArgv()...\n");
    std::map<std::string, std::string> argsmap;
    for (size_t i = 0, n = args.size(); i < n; ++i)
      {
	const std::string token = args[i];
	std::string key;
	std::string value = "";
	if (token.find("--") == 0) 
	  {
	    key = token;
	  }
	else
	  {
	    value = token;
	  }
	argsmap[key] = value;
	System::inform("argument[%2d]: %s =  %s\n", i, key.c_str(), value.c_str());
      }
    char command[0x200];
    go = true;
    bool postPossible = false;
    if ((argsmap.find("--midi") != argsmap.end()) && go)
      {
        generate();
        getScore().save(getMidiFilename().c_str());
      }
    if ((argsmap.find("--csound") != argsmap.end()) && go)
      {
        postPossible = true;
        render();
      }
    if ((argsmap.find("--pianoteq") != argsmap.end()) && go)
      {
        std::sprintf(command, "Pianoteq --midi %s\n", getMidiFilename().c_str());
	System::inform("Executing command: %s", command);
        int result = std::system(command);
      }
    if ((argsmap.find("--pianoteq-wav") != argsmap.end()) && go)
      {
        postPossible = true;
        std::sprintf(command, "Pianoteq --headless --midi %s --rate 48000 --wav %s\n", getMidiFilename().c_str(), getOutputSoundfileName().c_str());
	System::inform("Executing command: %s", command);
        int result = std::system(command);
      }
    if ((argsmap.find("--playmidi") != argsmap.end()) && go)
      {
        std::sprintf(command, "%s %s\n", argsmap["--playmidi"].c_str(), getMidiFilename().c_str());
	System::inform("Executing command: %s", command);
        int result = std::system(command);
      }
    if ((argsmap.find("--playwav") != argsmap.end()) && go)
      {
        std::sprintf(command, "%s %s\n", argsmap["--playwav"].c_str(), getOutputSoundfileName().c_str());
	System::inform("Executing command: %s", command);
        int result = std::system(command);
      }
    if ((argsmap.find("--post") != argsmap.end()) && go && postPossible)
      {
        translateMaster();
      }
    System::inform("ENDED MusicModel::processArgv().\n");
  }
  void MusicModel::stop()
  {
    std::cout << "MusicModel::stop()..." << std::endl;
    go = false;
    cppSound->stop();
  }
}
