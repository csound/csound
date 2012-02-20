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
    cppSound->setCommand(getCsoundCommand());
    createCsoundScore(csoundScoreHeader);
    cppSound->perform();
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
	char buffer[0x200];
	std::sprintf(buffer, "csound --midi-key=4 midi-velocity=5 -m99 -RWdfo %s temp.orc temp.sco", getOutputSoundfileName().c_str());
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

  /**
   * Pass the invoking program's command-line arguments to processArgs()
   * and it will perform the following commands:
   *
   * --midi          Render generated score as MIDI file and play it.
   * --csound        Render generated score using set Csound orchestra.
   * --pianoteq      Play generated MIDI sequence file with Pianoteq.
   * --pianoteq-wav  Render score to soundfile using Pianoteq,
   *                 post-process it, and play it.
   * --playmidi      Play generated MIDI filev
   * --playwav       Play rendered normalized output soundfile.
   * --post          Post-process Csound output soundfile:
   *                 normalize, CD, MP3, tag, and play it.
   */
  void MusicModel::processArgs(const std::vector<std::string> &args)
  {
    std::map<std::string, std::string> argsmap;
    for (size_t i = 0, n = args.size(); i < n; )
      {
	if (args[i].find("--") == 0) 
	  {
	    const std::string key = args[i];
	    ++i;
	    if (args[i].find("--") == std::string::npos) 
	      {
		std::string value = args[i];
		++i;
		argsmap[key] = value;
	      }
	    else
	      {
		argsmap[key] = "";
	      }
	  }
      }
    char command[0x200];
    bool postPossible = false;
    if (argsmap.find("--midi") != argsmap.end())
      {
	generate();
	cppSound->save(getMidiFilename().c_str());
      }
    if (argsmap.find("--csound") != argsmap.end())
      {
	postPossible = true;
	render();
      }
    if (argsmap.find("--pianoteq") != argsmap.end())
      {
	std::sprintf(command, "Pianoteq --midi %s", getMidiFilename().c_str());
	int result = std::system(command);
      }
    if (argsmap.find("--pianoteq-wav") != argsmap.end())
      {
	postPossible = true;
	std::sprintf(command, "Pianoteq --headless --midi %s --rate 48000 --wav %s", getMidiFilename().c_str(), getOutputSoundfileName().c_str());
	int result = std::system(command);
      }
    if (argsmap.find("--playmidi") != argsmap.end())
      {
	std::sprintf(command, "%s %s", argsmap["--playmidi"].c_str(), getMidiFilename().c_str());
	int result = std::system(command);
      }
    if (argsmap.find("--playwav") != argsmap.end())
      {
	std::sprintf(command, "%s %s", argsmap["--playwav"].c_str(), getOutputSoundfileName().c_str());
	int result = std::system(command);
      }
    if (argsmap.find("--post") != argsmap.end())
      {
	if (postPossible) 
	  {
	    translateMaster();
	  }
      }
  }
  void MusicModel::stop()
  {
    cppSound->stop();
  }
}

