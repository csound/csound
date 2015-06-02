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
#if defined(MSVC)
#pragma warning(disable: 4786)
#endif
#include "CsoundFile.hpp"
#include <ctime>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <csound.h>

#if defined(HAVE_MUSICXML2)
#include "elements.h"
#include "factory.h"
#include "xml.h"
#include "xmlfile.h"
#include "xml_tree_browser.h"
#include "xmlreader.h"
#include "midicontextvisitor.h"

using namespace MusicXML2;
#endif

void PUBLIC gatherArgs(int argc, const char **argv, std::string &commandLine)
{
  for(int i = 0; i < argc; i++)
    {
      if(i == 0)
        {
          commandLine = argv[i];
        }
      else
        {
          commandLine += " ";
          commandLine += argv[i];
        }
    }
}

void PUBLIC scatterArgs(const std::string buffer, std::vector<std::string> &args, std::vector<char *> &argv)
{
  std::string separators = " \t\n\r";
  args.clear();
  argv.clear();
  size_t first = 0;
  size_t last = 0;
  for(;;) {
    first = buffer.find_first_not_of(separators, last);
    if (first == std::string::npos) {
      return;
    }
    last = buffer.find_first_of(separators, first);
    if (last == std::string::npos) {
      args.push_back(buffer.substr(first));
      argv.push_back(const_cast<char *>(args.back().c_str()));
      return;
    } else {
      args.push_back(buffer.substr(first, last - first));
      argv.push_back(const_cast<char *>(args.back().c_str()));
    }
  }
}

std::string PUBLIC &trim(std::string &value)
{
  size_t i = value.find_first_not_of(" \n\r\t");
  if(i != value.npos)
    {
      value.erase(0, i);
    }
  else
    {
      value.erase(value.begin(), value.end());
      return value;
    }
  i = value.find_last_not_of(" \n\r\t");
  if(i != value.npos)
    {
      value.erase(i + 1, value.npos);
    }
  return value;
}

std::string PUBLIC &trimQuotes(std::string &value)
{
  size_t i = value.find_first_not_of("\"");
  if(i != value.npos)
    {
      value.erase(0, i);
    }
  i = value.find_last_not_of("\"");
  if(i != value.npos)
    {
      value.erase(i + 1, value.npos);
    }
  return value;
}

/**
 *       Returns true if definition is a valid Csound instrument definition block.
 *       Also returns the part before the instr number, the instr number,
 *       the name (all text after the first comment on the same line as the instr number),
 *       and the part after the instr number, all by reference.
 */
bool PUBLIC parseInstrument(const std::string &definition, std::string &preNumber, std::string &id, std::string &name, std::string &postNumber);

char staticBuffer[0x1000];

/**
 * Considerably more efficient than std::getline.
 */
bool getline(std::istream& stream, std::string &buffer)
{
  stream.getline(staticBuffer, sizeof(staticBuffer));
  buffer = staticBuffer;
  return stream.good();
}

bool PUBLIC parseInstrument(const std::string &definition, std::string &preNumber, std::string &id, std::string &name, std::string &postName)
{
  preNumber.erase();
  name.erase();
  postName.erase();
  id.erase();
  int beginId = definition.find("instr");
  if(beginId == -1)
    {
      return false;
    }
  beginId += 5;
  int newline = definition.find("\n", beginId);
  int semicolon = definition.find(";", beginId);
  if(semicolon > newline)
    {
      semicolon = -1;
    }
  size_t endId = 0;
  size_t beginName = 0;
  size_t endName = 0;
  if(newline == -1)
    {
      return false;
    }
  if(semicolon == -1)
    {
      endId = newline;
      id = definition.substr(beginId, endId - beginId);
      trim(id);
      if(id.length() == 0)
        {
          return false;
        }
      if(!strchr("0123456789", id[0]))
        {
          name = id;
        }
    }
  else
    {
      endId = semicolon;
      beginName = semicolon + 1;
      endName = newline;
      id = definition.substr(beginId, endId - beginId);
      trim(id);
      if(id.length() == 0)
        {
          return false;
        }
      name = definition.substr(beginName, endName - beginName);
      trim(name);
    }
  postName = definition.substr(endName);
  return true;
}

CsoundFile::CsoundFile()
{
  removeAll();
}

int CsoundFile::load(std::string filename)
{
  removeAll();
  int returnValue = importFile(filename);
  this->filename = filename.c_str();
  return returnValue;
}

int CsoundFile::load(std::istream &stream)
{
  removeAll();
  return importFile(stream);
}

int CsoundFile::save(std::string filename) const
{
  int returnValue = false;
  std::ofstream stream(filename.c_str(), std::ios::binary);
  if(!((filename.find(".orc") == filename.npos) && (filename.find(".ORC") == filename.npos)))
    {
      returnValue += exportOrchestra(stream);
    }
  else if(!((filename.find(".sco") == filename.npos) && (filename.find(".SCO") == filename.npos)))
    {
      returnValue += exportScore(stream);
    }
  else if(!((filename.find(".mid") == filename.npos) && (filename.find(".MID") == filename.npos)))
    {
      returnValue += exportMidifile(stream);
    }
  else
    {
      returnValue += save(stream);
    }
  stream.close();
  return returnValue;
}

int CsoundFile::save(std::ostream &stream) const
{
  int returnValue = 0;
  stream << "<CsoundSynthesizer>" << std::endl;
  stream << "<CsOptions>" << std::endl;
  returnValue += exportCommand(stream);
  stream << "</CsOptions>" << std::endl;
  stream << "<CsInstruments>" << std::endl;
  returnValue += exportOrchestra(stream);
  stream << "</CsInstruments>" << std::endl;
  stream << "<CsScore>" << std::endl;
  returnValue += exportScore(stream);
  stream << "</CsScore>" << std::endl;
  if(arrangement.size() > 0)
    {
      stream << "<CsArrangement>" << std::endl;
      returnValue += exportArrangement(stream);
      stream << "</CsArrangement>" << std::endl;
    }
  if(midifile.size() > 0)
    {
      stream << "<CsMidifile>" << std::endl;
      stream << "<Size>" << std::endl;
      stream << midifile.size() << std::endl;
      stream << "</Size>" << std::endl;
      returnValue += exportMidifile(stream);
      stream << "</CsMidifile>" << std::endl;
    }
  stream << "</CsoundSynthesizer>" << std::endl;
  return returnValue;
}

#if defined(HAVE_MUSICXML2)

  class CsoundFileMidiWriter : public midiwriter
  {
  public:
    long tpq;
    double tempo;
    CsoundFileMidiWriter(CsoundFile *csoundFile_) : tpq(1000), tempo(1.0), csoundFile(*csoundFile_)
    {
      csoundFile.removeScore();
    }
    virtual void startPart (int instrCount)
    {
    }
    virtual void newInstrument (std::string instrName, int chan=-1)
    {
    }
    virtual void endPart (long date)
    {
    }
    virtual void newNote (long start_, int insno_, float key_, int velocity_, int duration_)
    {
      double insno = double(insno_ + 1.0);
      double start = double(start_) / double(tpq) * tempo;
      double duration = double(duration_) / double(tpq) * tempo;
      double key = key_;
      double velocity = velocity_;
      csoundFile.addNote(insno, start, duration, key, velocity);
    }
    virtual void tempoChange (long date, int bpm)
    {
      tempo = 60.0 / double(bpm);
    }
    virtual void pedalChange (long date, pedalType t, int value)
    {
    }
    virtual void volChange (long date, int chan, int vol)
    {
    }
    virtual void bankChange (long date, int chan, int bank)
    {
    }
    virtual void progChange (long date, int chan, int prog)
    {
    }
  protected:
    CsoundFile &csoundFile;
  };

#endif

int CsoundFile::importFile(std::string filename)
{
  struct stat statbuffer;
  int returnValue = stat(filename.c_str(), &statbuffer);
  if(returnValue)
    {
      return false;
    }
  std::ifstream stream(filename.c_str(), std::ios::binary);
  if((filename.find(".orc")   != filename.npos) || (filename.find(".ORC")    != filename.npos))
    {
      returnValue += importOrchestra(stream);
    }
  else if((filename.find(".sco") != filename.npos) || (filename.find(".SCO") != filename.npos))
    {
      returnValue += importScore(stream);
    }
  else if((filename.find(".mid") != filename.npos) || (filename.find(".MID") != filename.npos))
    {
      returnValue += importMidifile(stream);
    }
#if defined(HAVE_MUSICXML2)
  else if((filename.find(".xml") != filename.npos) || (filename.find(".XML") != filename.npos))
    {
      score.erase();
      xmlreader xmlReader;
      Sxmlelement sxmlElement;
      // Try to read an SXMLFile out of the MusicXML file.
      SXMLFile sxmlFile = xmlReader.read(filename.c_str());
      if (sxmlFile) {
        // Get the document tree of XML elements from the SXMLFile.
        sxmlElement = sxmlFile->elements();
      }
      if (sxmlElement) {
        // Create a CsoundFileMidiWriter that is attached to this Score.
        CsoundFileMidiWriter csoundFileMidiWriter(this);
        // Create a midicontextvisitor, which calls into an abstract midiwriter interface,
        // which is attached to our CsoundFileMidiWriter, which implements that midiwriter interface.
        midicontextvisitor midicontextvisitor_(csoundFileMidiWriter.tpq, &csoundFileMidiWriter);
        // Create an xml_tree_browser that is attached to our midicontextvisitor.
        xml_tree_browser xmlTreeBrowser(&midicontextvisitor_);
        // The xml_tree_browser will carry the midicontextvisitor to all the elements
        // of the document tree, in the proper order, calling newNote as appropriate.
        xmlTreeBrowser.browse(*sxmlElement);
      }
    }
#endif
  else
    {
      returnValue += importFile(stream);
    }
  stream.close();
  return returnValue;
}

int CsoundFile::importFile(std::istream &stream)
{
  std::string buffer;
  while(getline(stream, buffer))
    {
      if(buffer.find("<CsoundSynthesizer>") == 0)
        {
          while(getline(stream, buffer))
            {
              if(buffer.find("</CsoundSynthesizer>") == 0)
                {
                  return true;
                }
              else if(buffer.find("<CsOptions>") == 0)
                {
                  importCommand(stream);
                }
              else if(buffer.find("<CsInstruments>") == 0)
                {
                  importOrchestra(stream);
                }
              else if(buffer.find("<CsArrangement>") == 0)
                {
                  importArrangement(stream);
                }
              else if(buffer.find("<CsScore>") == 0)
                {
                  importScore(stream);
                }
              else if(buffer.find("<CsMidifile>") == 0)
                {
                  importMidifile(stream);
                }
            }
        }
    }
  return false;
}

int CsoundFile::importCommand(std::istream &stream)
{
  std::string buffer;
  while(getline(stream, buffer))
    {
      if(buffer.find("</CsOptions") != buffer.npos)
        {
          return true;
        }
      command.append(buffer);
    }
  return false;
}

int CsoundFile::exportCommand(std::ostream &stream) const
{
  stream << command.c_str() << std::endl;
  return stream.good();
}

int CsoundFile::importOrchestra(std::istream &stream)
{
  orchestra.erase();
  std::string buffer;
  while(getline(stream, buffer))
    {
      if(buffer.find("</CsInstruments>") == 0)
        {
          return true;
        }
      else
        {
          orchestra.append(buffer);
          orchestra.append("\n");
        }
    }
  return false;
}

int CsoundFile::exportOrchestra(std::ostream &stream) const
{
  stream << orchestra;
  stream.flush();
  return stream.good();
}

int CsoundFile::importScore(std::istream &stream)
{
  score.erase();
  std::string buffer;
  while(getline(stream, buffer))
    {
      if(buffer.find("</CsScore>") == 0)
        {
          return true;
        }
      else
        {
          score.append(buffer);
          score.append("\n");
        }
    }
  return false;
}

int CsoundFile::exportScore(std::ostream &stream) const
{
  stream << score << std::endl;
  return stream.good();
}

std::string CsoundFile::getFilename() const
{
  return filename.c_str();
}

void CsoundFile::setFilename(std::string name)
{
  filename = name;
}

std::string CsoundFile::getCommand() const
{
  return command;
}

void CsoundFile::setCommand(std::string value)
{
  command = value;
}

void CsoundFile::removeCommand()
{
  command.erase(command.begin(), command.end());
}

std::string CsoundFile::getOrcFilename() const
{
  std::string buffer;
  scatterArgs(command, const_cast< std::vector<std::string> & >(args), const_cast< std::vector<char *> &>(argv));
  if(args.size() >= 3)
    {
      buffer = args[args.size() - 2];
    }
  return buffer.c_str();
}

std::string CsoundFile::getScoFilename() const
{
  std::string buffer;
  scatterArgs(command, const_cast< std::vector<std::string> & >(args), const_cast< std::vector<char *> &>(argv));
  if(args.size() >= 3)
    {
      buffer = args[args.size() - 1];
    }
  return buffer;
}

std::string CsoundFile::getMidiFilename() const
{
  std::string buffer;
  scatterArgs(command, const_cast< std::vector<std::string> & >(args), const_cast< std::vector<char *> &>(argv));
  for(int i = 1, n = args.size() - 2; i < n; i++)
    {
      std::string buffer = args[i];
      if(buffer.find("F") != buffer.npos)
        {
          if(buffer.find("F") == buffer.length() - 1)
            {
              buffer = args[i + 1];
              return buffer.c_str();
            }
          else
            {
              buffer = buffer.substr(buffer.find("F") + 1);
              return buffer.c_str();
            }
        }
    }
  return buffer.c_str();
}

std::string CsoundFile::getOutputSoundfileName() const
{
  return "default";
}

std::string CsoundFile::getOrchestra() const
{
  return orchestra;
}

void CsoundFile::setOrchestra(std::string value)
{
  orchestra = value;
}

void CsoundFile::removeOrchestra()
{
  orchestra.erase();
}

void CsoundFile::removeScore()
{
  score.erase();
}

std::string CsoundFile::getScore() const
{
  return score;
}

void CsoundFile::setScore(std::string score)
{
  this->score = score;
}

void CsoundFile::addScoreLine(std::string scoreLine)
{
  score.append(scoreLine);
  if(scoreLine.find("\n") == std::string::npos)
    {
      score.append("\n");
    }
}

void CsoundFile::removeAll()
{
  filename.erase();
  command.erase();
  orchestra.erase();
  score.erase();
  arrangement.erase(arrangement.begin(), arrangement.end());
  removeMidifile();
}

int CsoundFile::importMidifile(std::istream &stream)
{
  //      Importing from a standard MIDI file
  //      (Chunk ID == MThd or RIFF).
  if(stream.peek() == 'M' || stream.peek() == 'R')
    {
      midifile.resize(0);
      char buffer;
      while(!(stream.get(buffer).eof()))
        {
          midifile.push_back(buffer);
        }
      return true;
    }
  //      Importing from a "csd" file.
  else
    {
      std::string buffer;
      while(getline(stream, buffer))
        {
          if(buffer.find("</CsMidifile>") == 0)
            {
              return true;
            }
          else if(buffer.find("<Size>") == 0)
            {
              getline(stream, buffer);
              int size = atoi(buffer.c_str());
              getline(stream, buffer);
              if(size > 0)
                {
                  midifile.resize(0);
                  char charbuffer = 0;
                  for(int i = 0; i < size; i++)
                    {
                      stream.get(charbuffer);
                      midifile.push_back(charbuffer);
                    }
                }
            }
        }
    }
  return false;
}

int CsoundFile::exportMidifile(std::ostream &stream) const
{
  for(int i = 0, n = midifile.size(); i < n; i++)
    {
      stream.put(midifile[i]);
    }
  return stream.good();
}

void CsoundFile::removeMidifile()
{
  midifile.resize(0);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double p11)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
  addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
  addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4, p5, p6, p7, p8, p9);
  addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4, p5, p6, p7, p8);
  addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4, p5, p6, p7);
  addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4, p5, p6);
  addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4, p5);
  addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g %-.10g", p1, p2, p3, p4);
  addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3)
{
  char note[0x100];
  sprintf(note, "i %-.10g %-.10g %-.10g", p1, p2, p3);
  addScoreLine(note);
}

bool isToken(std::string text, int position, std::string token)
{
  size_t tokenend = position + token.size();
  if(text.size() > tokenend)
    {
      if(!std::isspace(text[tokenend]))
        {
          return false;
        }
    }
  for(int i = position - 1; i >= 0; --i)
    {
      if(text[i] == ';')
        {
          return false;
        }
      else if(text[i] == '\n')
        {
          return true;
        }
    }
  return true;
}

int findToken(std::string text, std::string token, int position)
{
  int foundPosition = 0;
  while((foundPosition = (int) text.find(token, position)) != -1)
    {
      if(isToken(text, foundPosition, token))
        {
          return foundPosition;
        }
      position = foundPosition + 1;
    }
  return foundPosition;
}

int CsoundFile::getInstrumentCount() const
{
  int beginDefinition = 0;
  int endDefinition = 0;
  int index;
  for(index = 0; true; )
    {
      beginDefinition = findToken(orchestra, "instr", beginDefinition);
      if(beginDefinition == -1)
        {
          return index;
        }
      endDefinition = findToken(orchestra, "endin", beginDefinition);
      if(endDefinition == -1)
        {
          return index;
        }
      endDefinition += 6;
      std::string definition = orchestra.substr(beginDefinition, endDefinition - beginDefinition);
      std::string pre;
      std::string id;
      std::string name;
      std::string post;
      if(parseInstrument(definition, pre, id, name, post))
        {
          index++;
          beginDefinition++;
        }
      else
        {
          break;
        }
    }
  return index;
}

bool CsoundFile::getInstrument(int number, std::string &definition_) const
{
  int beginDefinition = 0;
  int endDefinition = 0;
  for(int index = 0; true; index++)
    {
      beginDefinition = findToken(orchestra, "instr", beginDefinition);
      if(beginDefinition == -1)
        {
          return false;
        }
      endDefinition = findToken(orchestra, "endin", beginDefinition);
      if(endDefinition == -1)
        {
          return false;
        }
      endDefinition += 6;
      std::string definition = orchestra.substr(beginDefinition, endDefinition - beginDefinition);
      std::string pre;
      std::string id;
      std::string name;
      std::string post;
      if(parseInstrument(definition, pre, id, name, post))
        {
          if(number == atof(id.c_str()))
            {
              definition_ = definition;
              return true;
            }
        }
      beginDefinition++;
    }
  return false;
}

std::string CsoundFile::getInstrument(std::string name) const
{
  std::string definition;
  (void) getInstrument(name, definition);
  return definition;
}

std::string CsoundFile::getInstrument(int number) const
{
  std::string definition;
  (void) getInstrument(number, definition);
  return definition;
}

std::string CsoundFile::getInstrumentBody(std::string name) const
{
  std::string definition;
  getInstrument(name, definition);
  std::string pre;
  std::string id;
  std::string name_;
  std::string post;
  (void) parseInstrument(definition, pre, id, name_, post);
  return definition;
}

std::string CsoundFile::getInstrumentBody(int number) const
{
  std::string definition;
  getInstrument(number, definition);
  std::string pre;
  std::string id;
  std::string name;
  std::string post;
  (void) parseInstrument(definition, pre, id, name, post);
  return post;
}

std::map<int, std::string> CsoundFile::getInstrumentNames() const
{
  std::map<int, std::string> instrumentNames;
  int beginDefinition = 0;
  int endDefinition = 0;
  for(int index = 0; true; index++)
    {
      beginDefinition = findToken(orchestra, "instr", beginDefinition);
      if(beginDefinition == -1)
        {
          return instrumentNames;
        }
      endDefinition = findToken(orchestra, "endin", beginDefinition);
      if(endDefinition == -1)
        {
          return instrumentNames;
        }
      endDefinition += 6;
      std::string definition = orchestra.substr(beginDefinition, endDefinition - beginDefinition);
      std::string pre;
      std::string id;
      std::string name;
      std::string post;
      if(parseInstrument(definition, pre, id, name, post))
        {
          instrumentNames[atof(id.c_str())] = name;
        }
      beginDefinition++;
    }
}

//bool CsoundFile::getInstrumentNumber(int number_, std::string &definition_) const
//{
//      int beginDefinition = 0;
//      int endDefinition = 0;
//      for(int index = 0; true; index++)
//      {
//              beginDefinition = findToken(orchestra, "instr", beginDefinition);
//              if(beginDefinition == -1)
//              {
//                      return false;
//              }
//              endDefinition = findToken(orchestra, "endin", beginDefinition);
//              if(endDefinition == -1)
//              {
//                      return false;
//              }
//              endDefinition += 6;
//              std::string definition = orchestra.substr(beginDefinition, endDefinition - beginDefinition);
//              std::string pre;
//              std::string id;
//              std::string name;
//              std::string post;
//              if(parseInstrument(definition, pre, number, name, post))
//              {
//                      if(number_ == number)
//                      {
//                              definition_ = definition;
//                              return true;
//                      }
//                      index++;
//              }
//              beginDefinition++;
//      }
//      return false;
//}

bool CsoundFile::getInstrument(std::string name_, std::string &definition_) const
{
  trim(name_);
  int beginDefinition = 0;
  int endDefinition = 0;
  for(;;)
    {
      beginDefinition = findToken(orchestra, "instr", beginDefinition);
      if(beginDefinition == -1)
        {
          return false;
        }
      endDefinition = findToken(orchestra, "endin", beginDefinition);
      if(endDefinition == -1)
        {
          return false;
        }
      endDefinition += 6;
      std::string definition = orchestra.substr(beginDefinition, endDefinition - beginDefinition);
      std::string pre;
      std::string id;
      std::string name;
      std::string post;
      if(parseInstrument(definition, pre, id, name, post))
        {
          if(name_.compare(name) == 0 || id.compare(name) == 0)
            {
              definition_ = definition;
              return true;
            }
        }
      beginDefinition++;
    }
  return false;
}

double CsoundFile::getInstrumentNumber(std::string name_) const
{
  trim(name_);
  int beginDefinition = 0;
  int endDefinition = 0;
  for(;;)
    {
      beginDefinition = findToken(orchestra, "instr", beginDefinition);
      if(beginDefinition == -1)
        {
          return false;
        }
      endDefinition = findToken(orchestra, "endin", beginDefinition);
      if(endDefinition == -1)
        {
          return false;
        }
      endDefinition += 6;
      std::string definition = orchestra.substr(beginDefinition, endDefinition - beginDefinition);
      std::string pre;
      std::string id;
      std::string name;
      std::string post;
      if(parseInstrument(definition, pre, id, name, post))
        {
          if(name_.compare(name) == 0 || id.compare(name) == 0)
            {
              return atof(id.c_str());
            }
        }
      beginDefinition++;
    }
  return -1.0;
}

bool CsoundFile::exportForPerformance() const
{
  std::string orcFilename = getOrcFilename();
  if(orcFilename.length() > 0)
    {
      exportArrangementForPerformance(orcFilename);
    }
  std::string scoFilename = getScoFilename();
  if(scoFilename.length() > 0)
    {
      save(scoFilename);
    }
  std::string midiFilename = getMidiFilename();
  if(midiFilename.length() > 0 && midifile.size() > 0)
    {
      save(midiFilename);
    }
  return true;
}

void CsoundFile::setCSD(const std::string xml)
{
  std::istringstream stringStream(xml);
  load(stringStream);
}

std::string CsoundFile::getCSD() const
{
  std::ostringstream stringStream;
  save(stringStream);
  return stringStream.str();
}

int CsoundFile::getArrangementCount() const
{
  return arrangement.size();
}

void CsoundFile::removeArrangement()
{
  arrangement.erase(arrangement.begin(), arrangement.end());
}

void CsoundFile::addArrangement(std::string instrumentName)
{
  arrangement.push_back(instrumentName);
}

std::string CsoundFile::getArrangement(int index) const
{
  return arrangement[index];
}

int CsoundFile::importArrangement(std::istream &stream)
{
  removeArrangement();
  std::string buffer;
  while(getline(stream, buffer))
    {
      if(buffer.find("</CsArrangement>") == 0)
        {
          return true;
        }
      else
        {
          trim(buffer);
          arrangement.push_back(buffer);
        }
    }
  return false;
}

int CsoundFile::exportArrangement(std::ostream &stream) const
{
  for(std::vector<std::string>::const_iterator it = arrangement.begin(); it != arrangement.end(); ++it)
    {
      stream << (*it).c_str() << std::endl;
    }
  return stream.good();
}

std::string CsoundFile::getOrchestraHeader() const
{
  int instrIndex = findToken(orchestra, "instr", 0);
  if(instrIndex == (int) orchestra.npos)
    {
      return "";
    }
  return orchestra.substr(0, instrIndex);
}

int CsoundFile::exportArrangementForPerformance(std::string filename) const
{
  std::ofstream stream(filename.c_str(), std::ios::binary);
  exportArrangementForPerformance(stream);
  stream.close();
  return stream.good();
}

int CsoundFile::exportArrangementForPerformance(std::ostream &stream) const
{
  int i,n;
  if(arrangement.size() > 0)
    {
      stream << "; ARRANGEMENT " << getOrcFilename().c_str() << std::endl;
      stream << getOrchestraHeader() << std::endl;
//       for(i = 0, n = arrangement.size(); i < n; ++i)
//         {
//           stream << "massign " << (i + 1) << " , " << (i + 1) << std::endl;
//           stream.flush();
//         }
      for(i = 0, n = arrangement.size(); i < n; ++i)
        {
          std::string instrumentName = arrangement[i];
          std::string definition;
          if(getInstrument(instrumentName, definition))
            {
              std::string preNumber;
              std::string id;
              std::string postNumber;
              if(parseInstrument(definition, preNumber, id, instrumentName, postNumber))
                {
                  stream << std::endl << "instr " << (i + 1) << " ; " << instrumentName << std::endl << postNumber << std::endl;
                  stream.flush();
                }
            }
        }
    }
  else
    {
      exportOrchestra(stream);
    }
  return stream.good();
}

//void CsoundFile::getInstrumentNames(std::vector<std::string> &names) const
//{
//      names.erase(names.begin(), names.end());
//      std::string definition;
//      for(int i = 0; getInstrument(i, definition); i++)
//      {
//              std::string preNumber;
//              int number;
//              std::string name;
//              std::string postName;
//              if(parseInstrument(definition, preNumber, number, name, postName))
//              {
//                      names.push_back(name);
//              }
//      }
//      std::sort(names.begin(), names.end());
//}

void CsoundFile::insertArrangement(int index, std::string instrument)
{
  arrangement.insert(arrangement.begin() + index, instrument);
}

void CsoundFile::removeArrangement(int index)
{
  arrangement.erase(arrangement.begin() + index);
}

void CsoundFile::setArrangement(int index, std::string instrument)
{
  arrangement[index] = instrument;
}

bool CsoundFile::loadOrcLibrary(const char *filename)
{
  if(!filename)
    {
      return false;
    }
  std::fstream stream;
  if(strlen(filename) > 0)
    {
      stream.open(filename, std::fstream::in | std::ios::binary);
    }
  else
    {
      std::string orcLibraryFilename = getenv("CSOUND_HOME");
      orcLibraryFilename.append("/");
      orcLibraryFilename.append("library.orc");
      stream.open(orcLibraryFilename.c_str(), std::fstream::in | std::ios::binary);
    }
  if(stream.good())
    {
      removeOrchestra();
      importOrchestra(stream);
      return true;
    }
  return false;
}

std::string CsoundFile::generateFilename()
{
  time_t time_ = 0;
  time(&time_);
  struct tm* tm_ = gmtime(&time_);
  char buffer[0x100];
  strftime(buffer, 0x100, "csound.%Y-%m-%d.%H-%M-%S.csd", tm_);
  filename = buffer;
  return filename;
}

