/**
* C S O U N D   V S T
*
* A VST plugin version of Csound, with Python scripting.
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
#include <algorithm>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <cs.h>
#include <oload.h>

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

bool parseInstrument(const std::string &definition, std::string &preNumber, std::string &id, std::string &name, std::string &postName)
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

std::string CsoundFile::getFilename()
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

void CsoundFile::removeCommand(void)
{
        command.erase(command.begin(), command.end());
}

std::string CsoundFile::getOrcFilename() const
{
        int argc;
        char **argv;
        std::string buffer;
        scatterArgs(command, &argc, &argv);
        if(argc >= 3)
        {
                buffer = argv[argc - 2];
        }
        deleteArgs(argc, argv);
        return buffer.c_str();
}

std::string CsoundFile::getScoFilename() const
{
        int argc;
        char **argv;
        std::string buffer;
        scatterArgs(command, &argc, &argv);
        if(argc >= 3)
        {
                buffer = argv[argc - 1];
        }
        deleteArgs(argc, argv);
        return buffer;
}

std::string CsoundFile::getMidiFilename() const
{
        int argc;
        char **argv;
        std::string buffer;
        scatterArgs(command, &argc, &argv);
        for(int i = 1, n = argc - 2; i < n; i++)
        {
                std::string buffer = argv[i];
                if(buffer.find("F") != buffer.npos)
                {
                        if(buffer.find("F") == buffer.length() - 1)
                        {
                                buffer = argv[i + 1];
                                deleteArgs(argc, argv);
                                return buffer.c_str();
                        }
                        else
                        {
                                buffer = buffer.substr(buffer.find("F") + 1);
                                deleteArgs(argc, argv);
                                return buffer.c_str();
                        }
                }
        }
        deleteArgs(argc, argv);
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
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p4, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p5, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p6, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p7, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p8, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p9, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p10, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p11, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10)
{
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p4, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p5, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p6, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p7, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p8, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p9, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p10, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9)
{
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p4, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p5, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p6, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p7, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p8, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p9, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8)
{
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p4, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p5, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p6, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p7, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p8, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6, double p7)
{
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p4, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p5, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p6, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p7, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5, double p6)
{
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p4, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p5, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p6, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4, double p5)
{
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p4, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p5, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3, double p4)
{
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p4, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

void CsoundFile::addNote(double p1, double p2, double p3)
{
        std::string note = "i ";
        gcvt(p1, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p2, 10, staticBuffer);
        note += staticBuffer;
        note += " ";
        gcvt(p3, 10, staticBuffer);
        note += staticBuffer;
        addScoreLine(note);
}

bool isToken(std::string text, int position, std::string token)
{
        size_t tokenend = position + token.size();
        if(text.size() > tokenend)
        {
                if(!isspace(text[tokenend]))
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

bool CsoundFile::exportForPerformance(void)
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

std::string CsoundFile::getCSD(void) const
{
        std::ostringstream stringStream;
        save(stringStream);
        return stringStream.str();
}

int CsoundFile::getArrangementCount(void) const
{
        return arrangement.size();
}

void CsoundFile::removeArrangement(void)
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

std::string CsoundFile::getOrchestraHeader(void) const
{
        int instrIndex = findToken(orchestra, "instr", 0);
        if(instrIndex == orchestra.npos)
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
        if(arrangement.size() > 0)
        {
                stream << "; ARRANGEMENT " << getOrcFilename().c_str() << std::endl;
                stream << getOrchestraHeader() << std::endl;
                for(int i = 0, n = arrangement.size(); i < n; ++i)
                {
                        stream << "massign " << (i + 1) << " , " << (i + 1) << std::endl;
                        stream.flush();
                }
                for(int i = 0, n = arrangement.size(); i < n; ++i)
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
};

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

