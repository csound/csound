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
#include <map>

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

  int Composition::render()
  {
    clear();
    int errorStatus = generate();
    timestamp = makeTimestamp();
    if (errorStatus) {
      return errorStatus;
    }
    errorStatus = perform();
    return errorStatus;
  }

  int Composition::renderAll()
  {
    clear();
    int errorStatus = generate();
    if (errorStatus) {
      return errorStatus;
    }
    errorStatus = performAll();
    return errorStatus;
  }

  int Composition::perform()
  {
    return 0;
  }

  int Composition::generate()
  {
    return 0;
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

  std::string Composition::getOutputDirectory() const
  {
    return outputDirectory;
  }

  void Composition::setOutputDirectory(std::string directory)
  {
    outputDirectory = directory;
  }

  std::string Composition::getFilename() const
  {
    return filename;
  }

  void Composition::setFilename(std::string filename)
  {
    this->filename = filename;
  }

  std::string Composition::getFilePathname() const
  {
    return outputDirectory + filename;
  }

  std::string Composition::generateFilename()
  {
    char buffer[0x100];
    snprintf(buffer, 0x100, "silence.%s.py", makeTimestamp().c_str());
    return buffer;
  }

  std::string Composition::getMidiFilename() const
  {
    std::string name = getFilePathname();
    name.append(".mid");
    return name;
  }

  std::string Composition::getOutputSoundfileName() const
  {
    if (outputFilename.empty())
      {
        std::string name_ = getFilePathname();
        name_.append(".wav");
        return name_;
      }
    else
      {
        return outputFilename;
      }
  }

  std::string Composition::getNormalizedSoundfileName() const
  {
    std::string name = getFilePathname();
    name.append(".norm.wav");
    return name;
  }

  std::string Composition::getCdSoundfileName() const
  {
    std::string name = getFilePathname();
    name.append(".cd.wav");
    return name;
  }

  std::string Composition::getMp3SoundfileName() const
  {
    std::string name = getFilePathname();
    name.append(".mp3");
    return name;
  }

  std::string Composition::getMusicXmlFilename() const
  {
    std::string name = getFilePathname();
    name.append(".xml");
    return name;
  }

  std::string Composition::getFomusFilename() const
  {
    std::string name = getFilePathname();
    name.append(".fms");
    return name;
  }

  std::string Composition::getLilypondFilename() const
  {
    std::string name = getFilePathname();
    name.append(".ly");
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

  int Composition::tagFile(std::string filename) const
  {
    std::string command = "bwfmetaedit";
    command = command + " --OriginationDate=" + getTimestamp().substr(0, 10);
    command = command + " --ICRD=" + getTimestamp().substr(0, 10);
    if (getTitle().length() > 1) {
      command = command + " --Description=" + getTitle();
      command = command + " --INAM=" + getTitle();
    }
    if (getCopyright().length() > 1) {
      command = command + " --ICOP=" + getCopyright();
    }
    if (getArtist().length() > 1) {
      command = command + " --Originator=" + getArtist();
      command = command + " --IART=" + getArtist();
    }
    if (getAlbum().length() > 1) {
      command = command + " --IPRD=" + getAlbum();
    }
    if (getLicense().length() > 1) {
      command = command + " --ICMT=" + getLicense();
    }
    command = command + " " + filename.c_str();
    System::inform("tagFile(): %s\n", command.c_str());
    return std::system(command.c_str());
  }

  int Composition::performMaster()
  {
    System::inform("BEGAN Composition::performMaster()...\n");
    timestamp = makeTimestamp();
    score.save(getMidiFilename());
    // Not implemented fully yet.
    //score.save(getMusicXmlFilename());
    int errorStatus = perform();
    System::inform("ENDED Composition::performMaster().\n");
    return errorStatus;
  }

  int Composition::performAll()
  {
    System::inform("BEGAN Composition::performAll()...\n");
    int errorStatus = performMaster();
    if (errorStatus) {
      return errorStatus;
    }
    errorStatus = translateMaster();
    System::inform("ENDED Composition::performAll().\n");
    return errorStatus;
  }

  int Composition::translateMaster()
  {
    System::inform("ENDED Composition::translateMaster().\n");
    int errorStatus = tagFile(getOutputSoundfileName());
    if (errorStatus) {
      return errorStatus;
    }
    errorStatus = normalizeOutputSoundfile();
    if (errorStatus) {
      return errorStatus;
    }
    errorStatus = translateToCdAudio();
    if (errorStatus) {
      return errorStatus;
    }
    errorStatus = translateToMp3();
    System::inform("ENDED Composition::translateMaster().\n");
    return errorStatus;
  }

  int Composition::normalizeOutputSoundfile(double levelDb)
  {
    char buffer[0x100];
    std::snprintf(buffer,
                  0x100,
                  "sox %s -V3 -b 32 -e floating-point %s gain -n %f\n",
                  getOutputSoundfileName().c_str(),
                  getNormalizedSoundfileName().c_str(),
                  levelDb);
    int errorStatus = std::system(buffer);
    if (errorStatus) {
      return errorStatus;
    }
    System::inform("Composition::normalizeOutputSoundfile(): %s", buffer);
    errorStatus = tagFile(getNormalizedSoundfileName());
    return errorStatus;
  }

  int Composition::translateToCdAudio(double levelDb)
  {
    char buffer[0x100];
    std::snprintf(buffer, 0x100, "sox %s -V3 -b 16 %s gain -n %f rate 44100\n",
                  getOutputSoundfileName().c_str(),
                  getCdSoundfileName().c_str(),
                  levelDb);
    System::inform("Composition::translateToCdAudio(): %s", buffer);
    int errorStatus = std::system(buffer);
    if (errorStatus) {
      return errorStatus;
    }
    errorStatus = tagFile(getCdSoundfileName());
    return errorStatus;
  }

  int Composition::translateToMp3(double bitrate, double levelDb)
  {
    char buffer[0x100];
    std::snprintf(buffer,
                  0x100,
                  "lame --verbose --disptime 2 --nohist --preset cd --tt %s --ta %s --tl %s --tc %s %s %s\n",
                  getTitle().c_str(),
                  getArtist().c_str(),
                  getAlbum().c_str(),
                  getCopyright().c_str(),
                  getCdSoundfileName().c_str(),
                  getMp3SoundfileName().c_str());
    System::inform("Composition::translateToMp3(): %s", buffer);
    int errorStatus = std::system(buffer);
    return errorStatus;
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

  int Composition::translateToNotation(const std::vector<std::string> partNames, std::string header)
  {
    std::string filename = getFomusFilename();
    std::ofstream stream;
    stream.open(filename.c_str(), std::ifstream::binary);
    char buffer[0x200];
    std::sprintf(buffer, "title = %s\n", getTitle().c_str());
    stream << buffer;
    if (getArtist().length() > 1) {
      std::sprintf(buffer, "author = %s\n", getArtist().c_str());
      stream << buffer;
    }
    stream << "beat = 1/64" << std::endl;
    stream << "timesig (4 4)" << std::endl;
    stream << "lily-papersize = 11x17" << std::endl;
    if (header.size() > 1) {
      stream << header.c_str();
    }
    if (partNames.size() > 0) {
      for (size_t partI = 0, partN = partNames.size(); partI < partN; ++partI) {
        std::sprintf(buffer, "part <id = %zu name = %s>\n", partI, partNames[partI].c_str());
        stream << buffer;
      }
    } else {
      for (size_t partI = 0, partN = 100; partI < partN; ++partI) {
        std::sprintf(buffer, "part <id = %zu name = Part%zu>\n", partI, partI);
        stream << buffer;
      }
    }
    std::map<int, std::vector<Event> > eventsForParts;
    for (size_t eventI = 0, eventN = score.size(); eventI < eventN; ++eventI) {
        const Event &event = score[eventI];
        if (event.isNoteOn()) {
            double duration = event.getDuration() * 32.0;
            duration = Conversions::round(duration);
            if (duration > 0) {
              int part = int(event.getInstrument() + 1);
              eventsForParts[part].push_back(event);
            }
        }
    }
    for (std::map<int, std::vector<Event> >::iterator it = eventsForParts.begin(); it != eventsForParts.end(); ++it) {
        int part = it->first;
        std::vector<Event> &events = it->second;
        for (std::vector<Event>::iterator eventI = events.begin(); eventI != events.end(); ++eventI) {
            Event &event = *eventI;
            if (eventI == events.begin()) {
                std::sprintf(buffer, "part %d\n", part);
            } else {
                double duration = event.getDuration() * 32.0;
                duration = Conversions::round(duration);
                std::sprintf(buffer, "time %g dur %g pitch %g;\n", event.getTime() * 32.0, duration, event.getKey());
            }
            stream << buffer;
        }
    }
    stream.close();
    std::sprintf(buffer, "fomus --verbose -i %s -o %s.xml", getFomusFilename().c_str(), getTitle().c_str());
    int errorStatus = std::system(buffer);
    return errorStatus;
  }

  int Composition::processArgv(int argc, const char **argv)
  {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
      {
        args.push_back(argv[i]);
      }
    return processArgs(args);
  }

  int Composition::processArgs(const std::vector<std::string> &args)
  {
    return renderAll();
  }

  void Composition::setOutputSoundfileName(std::string name)
  {
    outputFilename = name;
  }

  void Composition::clearOutputSoundfileName()
  {
    outputFilename.clear();
  }
}
