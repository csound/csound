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
#include <cstdlib>

namespace csound
{
  Composition::Composition() :
    tonesPerOctave(12.0),
    conformPitches(false)
  {
  }

  Composition::~Composition()
  {
  }

  void Composition::render()
  {
    clear();
    generate();
    timestamp = makeTimestamp();
    perform();
  }

  void Composition::renderAll()
  {
    clear();
    generate();
    performAll();
  }

  void Composition::perform()
  {
  }

  void Composition::generate()
  {
  }

  void Composition::clear()
  {
    score.clear();
  }

  Score &Composition::getScore()
  {
    return score;
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

  std::string Composition::getFilename() const
  {
    return filename;
  }

  void Composition::setFilename(std::string filename)
  {
    this->filename = filename;
  }

  std::string Composition::generateFilename()
  {
    char buffer[0x100];
    snprintf(buffer, 0x100, "silence.%s.py", makeTimestamp().c_str());
    return buffer;
  }

  std::string Composition::getMidiFilename()
  {
    std::string name = getFilename();
    name.append(".mid");
    return name;
  }

  std::string Composition::getOutputSoundfileName()
  {
    std::string name = getFilename();
    name.append(".wav");
    return name;
  }

  std::string Composition::getNormalizedSoundfileName()
  {
    std::string name = getFilename();
    name.append(".norm.wav");
    return name;
  }

  std::string Composition::getCdSoundfileName()
  {
    std::string name = getFilename();
    name.append(".cd.wav");
    return name;
  }

  std::string Composition::getMp3SoundfileName()
  {
    std::string name = getFilename();
    name.append(".mp3");
    return name;
  }

  std::string Composition::getMusicXmlFilename()
  {
    std::string name = getFilename();
    name.append(".xml");
    return name;
  }

  std::string Composition::makeTimestamp()
  {
    time_t time_ = 0;
    time(&time_);
    struct tm* tm_ = gmtime(&time_);
    char buffer[0x100];
    strftime(buffer, 0x100, "%Y-%m-%d.%H-%M-%S", tm_);
    return buffer;
  }

  std::string Composition::getTimestamp() const
  {
    return timestamp;
  }
  
  void tagFile(Composition &composition, std::string filename)
  {
    std::string command = "bwfmetaedit";
    command = command + " --OriginationDate=" + composition.getTimestamp().substr(0, 10);
    command = command + " --ICRD=" + composition.getTimestamp().substr(0, 10);
    if (composition.getTitle().length() > 1) {
        command = command + " --Description=" + composition.getTitle();
        command = command + " --INAM=" + composition.getTitle();
    }
    if (composition.getCopyright().length() > 1) {
        command = command + " --ICOP=" + composition.getCopyright();
    }
    if (composition.getArtist().length() > 1) {
        command = command + " --Originator=" + composition.getArtist();
        command = command + " --IART=" + composition.getArtist();
    }
    if (composition.getAlbum().length() > 1) {
        command = command + " --IPRD=" + composition.getAlbum();
    }
    if (composition.getLicense().length() > 1) {
        command = command + " --ICMT=" + composition.getLicense();
    }    
    command = command + " " + filename.c_str();
    System::inform("tagFile(): %s\n", command.c_str());
    int result = std::system(command.c_str());
  }

  void Composition::performMaster()
  {
    System::inform("BEGAN Composition::performMaster()...\n");
    timestamp = makeTimestamp();
    score.save(getMidiFilename());
    score.save(getMusicXmlFilename());
    perform();
    System::inform("ENDED Composition::performMaster().\n");
  }

  void Composition::performAll()
  {
    System::inform("BEGAN Composition::performAll()...\n");
    performMaster();
    translateMaster();
    System::inform("ENDED Composition::performAll().\n");
  }

  void Composition::translateMaster()
  {
    System::inform("ENDED Composition::translateMaster().\n");
    tagFile(*this, getOutputSoundfileName());
    normalizeOutputSoundfile();
    translateToCdAudio();
    translateToMp3();
    System::inform("ENDED Composition::translateMaster().\n");
  }

  void Composition::normalizeOutputSoundfile(double levelDb)
  {
    char buffer[0x100];
    std::snprintf(buffer, 
		  0x100, 
		  "sox %s -V3 -b 32 -e floating-point %s gain -n %f\n",
		  getOutputSoundfileName().c_str(),
		  getNormalizedSoundfileName().c_str(),
		  levelDb);
    int result = std::system(buffer);
    System::inform("Composition::normalizeOutputSoundfile(): %s", buffer);
    tagFile(*this, getNormalizedSoundfileName());
  }

  void Composition::translateToCdAudio(double levelDb)
  {
    char buffer[0x100];
    std::snprintf(buffer, 0x100, "sox %s -V3 -b 16 %s gain -n %f rate 44100\n",
		  getOutputSoundfileName().c_str(),
		  getCdSoundfileName().c_str(),
		  levelDb);
    System::inform("Composition::translateToCdAudio(): %s", buffer);
    int result = std::system(buffer);
    tagFile(*this, getCdSoundfileName());
  }

  void Composition::translateToMp3(double bitrate, double levelDb)
  {
    char buffer[0x100];
    std::snprintf(buffer, 
		  0x100, 
		  "lame -V3 --preset cd --tt %s --ta %s --tl %s --tc %s %s %s\n",
		  getTitle().c_str(),
		  getArtist().c_str(),
		  getAlbum().c_str(),
		  getCopyright().c_str(),
		  getCdSoundfileName().c_str(),
		  getMp3SoundfileName().c_str());
    System::inform("Composition::translateToMp3(): %s", buffer);
    int result = std::system(buffer);
  }

  std::string Composition::getArtist() const
  {
    return artist;
  }

  void Composition::setArtist(std::string value)
  {
    artist = value;
  }

  std::string Composition::getTitle() const
  {
    return title;
  }

  void Composition::setTitle(std::string value)
  {
    title = value;
  }

  std::string Composition::getCopyright() const
  {
    return copyright;
  }

  void Composition::setCopyright(std::string value)
  {
    copyright = value;
  }

  std::string Composition::getAlbum() const
  {
    return album;
  }

  void Composition::setAlbum(std::string value)
  {
    album = value;
  }

  std::string Composition::getLicense() const
  {
    return license;
  }

  void Composition::setLicense(std::string value)
  {
    license = value;
  }
}
