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
    threadCount(1)
  {
  }

  MusicModel::~MusicModel()
  {
    //clear();
  }

  void MusicModel::initialize()
  {
  }

  int MusicModel::generate()
  {
    int errorStatus = 0;
    cppSound->removeScore();
    if (children.size()) {
      score.clear();
    }
    traverse(getLocalCoordinates(), score);
    System::message("Generated %d events.\n", score.size());
    return errorStatus;
  }

  int MusicModel::render()
  {
    int errorStatus = generate();
    if (errorStatus) {
      return errorStatus;
    }
    errorStatus = perform();
    return errorStatus;
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
    //std::sprintf(buffer, "\ns %9.3f", extendSeconds);
    //cppSound->addScoreLine(buffer);
    std::sprintf(buffer, "\ne %9.3f\n", extendSeconds);
    cppSound->addScoreLine(buffer);
    //cppSound->exportForPerformance();
  }

  int MusicModel::perform()
  {
    int errorStatus = 0;
    cppSound->setCommand(getCsoundCommand());
    createCsoundScore(csoundScoreHeader);
    errorStatus = cppSound->perform();
    if (errorStatus == 1) {
      errorStatus = 0;
    }
    // The Csound command is managed from MusicModel,
    // not from CppSound. So we clear out what we set.
    cppSound->setCommand("");
    return errorStatus;
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

  void MusicModel::arrange(int silenceInstrumentNumber,
                           std::string csoundInstrumentName)
  {
    int csoundInstrumentNumber =
      cppSound->getInstrumentNumber(csoundInstrumentName);
    arrange(silenceInstrumentNumber, csoundInstrumentNumber);
  }

  void MusicModel::arrange(int silenceInstrumentNumber,
                           std::string csoundInstrumentName, double gain)
  {
    int csoundInstrumentNumber =
      cppSound->getInstrumentNumber(csoundInstrumentName);
    arrange(silenceInstrumentNumber, csoundInstrumentNumber, gain);
  }

  void MusicModel::arrange(int silenceInstrumentNumber,
                           std::string csoundInstrumentName, double gain, double pan)
  {
    int csoundInstrumentNumber =
      cppSound->getInstrumentNumber(csoundInstrumentName);
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
    if (command_.size() == 0) {
      char buffer[0x200];
      std::sprintf(buffer,
                   "csound --midi-key=4 --midi-velocity=5 -m195 -j%d -RWdfo %s",
           threadCount,
                   getOutputSoundfileName().c_str());
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

  int MusicModel::processArgs(const std::vector<std::string> &args)
  {
    System::inform("BEGAN MusicModel::processArgv()...\n");
    std::map<std::string, std::string> argsmap;
    std::string key;
    for (size_t i = 0, n = args.size(); i < n; ++i) {
      const std::string token = args[i];
      std::string value = "";
      if (token.find("--") == 0) {
        key = token;
        System::inform("argument[%2d]: %s\n", i, key.c_str());
      } else {
        value = token;
        System::inform("argument[%2d]: %s =  %s\n", i, key.c_str(), value.c_str());
      }
      argsmap[key] = value;
    }
    char command[0x200];
    int errorStatus = 0;
    bool postPossible = false;
    std::string playSoundfileName = getOutputSoundfileName();
    if ((argsmap.find("--dir") != argsmap.end()) && !errorStatus) {
      setOutputDirectory(argsmap["--dir"]);
    }
    if ((argsmap.find("--midi") != argsmap.end()) && !errorStatus) {
      errorStatus = generate();
      if (errorStatus) {
        return errorStatus;
      }
      getScore().save(getMidiFilename().c_str());
    }
    if ((argsmap.find("--notation") != argsmap.end()) && !errorStatus) {
        translateToNotation();
    }
    if ((argsmap.find("--audio") != argsmap.end()) && !errorStatus) {
      postPossible = false;
      const char *audiosystem = argsmap["--audio"].c_str();
      const char *devicename = argsmap["--device"].c_str();
      std::sprintf(command,
                   "csound --midi-key=4 --midi-velocity=5 -m195 -+rtaudio=%s -o %s",
                   audiosystem, devicename);
      System::inform("Csound command: %s\n", command);
      setCsoundCommand(command);
      errorStatus = render();
    }
    if ((argsmap.find("--csound") != argsmap.end()) && !errorStatus) {
      postPossible = true;
      errorStatus = render();
    }
    if ((argsmap.find("--pianoteq") != argsmap.end()) && !errorStatus) {
      std::sprintf(command, "Pianoteq --midi=%s\n", getMidiFilename().c_str());
      System::inform("Executing command: %s\n", command);
      errorStatus = std::system(command);
    }
    if ((argsmap.find("--pianoteq-wav") != argsmap.end()) && !errorStatus) {
      postPossible = true;
      std::sprintf(command, "Pianoteq --headless --midi %s --rate 48000 --wav %s\n", getMidiFilename().c_str(), getOutputSoundfileName().c_str());
      System::inform("Executing command: %s\n", command);
      errorStatus = std::system(command);
    }
    if ((argsmap.find("--playmidi") != argsmap.end()) && !errorStatus) {
      std::sprintf(command, "%s %s\n", argsmap["--playmidi"].c_str(), getMidiFilename().c_str());
      System::inform("Executing command: %s\n", command);
      errorStatus = std::system(command);
    }
    if ((argsmap.find("--post") != argsmap.end()) && !errorStatus && postPossible) {
      errorStatus = translateMaster();
      playSoundfileName = getNormalizedSoundfileName();
    }
    if ((argsmap.find("--playwav") != argsmap.end()) && !errorStatus) {
      std::sprintf(command, "%s %s\n", argsmap["--playwav"].c_str(), playSoundfileName.c_str());
      System::inform("Csound command: %s\n", command);
      errorStatus = std::system(command);
    }
    System::inform("ENDED MusicModel::processArgv().\n");
    return errorStatus;
  }

  void MusicModel::stop()
  {
    std::cout << "MusicModel::stop()..." << std::endl;
    cppSound->stop();
  }
}
