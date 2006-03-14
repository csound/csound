/**
 * S C O R E   G E N E R A T O R   V S T 
 *
 * A VST plugin for writing score generators in Python.
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
#include <Python.h>
#include "ScoreGeneratorVst.hpp"
#include "ScoreGeneratorVstFltk.hpp"
#include "System.hpp"

static char *dupstr(const char *string)
{
  if (string == 0) {
    return 0;
  }
  size_t len = std::strlen(string);
  char *copy = (char *)std::malloc(len + 1);
  std::strncpy(copy, string, len);
  copy[len] = '\0';
  return copy;
}

ScoreGeneratorVst::ScoreGeneratorVst(audioMasterCallback audioMaster) :
  AudioEffectX(audioMaster, kNumPrograms, 0),
  model(&model_),
  vstSr(0),
  vstPriorSampleBlockStart(0),
  vstCurrentSampleBlockStart(0),
  vstCurrentSampleBlockEnd(0),
  ScoreGeneratorVstFltk(0)
{
  setNumInputs(kNumInputs);             // stereo in
  setNumOutputs(kNumOutputs);           // stereo out
  setUniqueID('sGsT');  // identify
  canMono();                            // makes sense to feed both inputs with the same signal
  canProcessReplacing();        // supports both accumulating and replacing output
  wantEvents();
  open();
  ScoreGeneratorVstFltk = new ScoreGeneratorVstFltk(this);
  setEditor(ScoreGeneratorVstFltk);
  programsAreChunks(true);
  curProgram = 0;
  bank.resize(kNumPrograms);
  for(size_t i = 0; i < bank.size(); i++)
    {
      char buffer[0x24];
      sprintf(buffer, "Program%d", (int)(i + 1));
      bank[i].name = buffer;
    }
}


ScoreGeneratorVst::~ScoreGeneratorVst()
{
  Shell::close();
}

void ScoreGeneratorVst::setProgramName(char *name)
{
  bank[curProgram].name = name;
}

void ScoreGeneratorVst::getProgramName(char *name)
{
  strcpy(name, bank[curProgram].name.c_str());
}

AEffEditor* ScoreGeneratorVst::getEditor()
{
  return editor;
}

void ScoreGeneratorVst::openView(bool doRun)
{
  editor->open(0);
  if(doRun) {
    run();
  }
}

void ScoreGeneratorVst::closeView()
{
  editor->close();
}

int ScoreGeneratorVst::runScript(std::string script_)
{
  log("BEGAN ScoreGeneratorVst::runScript()...\n");
  int result = 0;
  try
    {
      char *script__ = const_cast<char *>(script_.c_str());
      log("==============================================================================================================\n");
      result = PyRun_SimpleString(script__);
      if(result)
	{
	  PyErr_Print();
	}
    }
  catch(...)
    {
      log("Unidentified exception in ScoreGeneratorVst::runScript().\n");
    }
  log("==============================================================================================================\n");
  log("PyRun_SimpleString returned %d.\n", result);
  log("ENDED ScoreGeneratorVst::runScript().\n");
  return result;
}


PyObject *ScoreGeneratorVst::scoregen_addEvent(PyObject *self, PyObject *args)
{
  double time = 0.;
  double duration = 0.;
  double status = 0.;
  double channel = 0.;
  double key = 0.;
  double velocity = 0.;

    if (!PyArg_ParseTuple(args, "dddddd", &time, &duration, &status, &channel, &key, &velocity))
        return NULL;
    events = addEvent
    return Py_BuildValue("i", events);
}


void ScoreGeneratorVst::open()
{
  int result = 0;
  Shell::open();
  // Create scoregen module.
  // Create scoregen.addEvent.
  // Create scoregen.write (using log).
  
  std::string filename_ = getFilename();
}

void ScoreGeneratorVst::setProgram(long program)
{
  log("RECEIVED ScoreGeneratorVst::setProgram(%d)...\n", program);
  if(program < kNumPrograms && program >= 0)
    {
      curProgram = program;
      setText(bank[curProgram].text);
    }
}

void ScoreGeneratorVst::suspend()
{
  log("RECEIVED ScoreGeneratorVst::suspend()...\n");
}

void ScoreGeneratorVst::resume()
{
  log("RECEIVED ScoreGeneratorVst::resume()...\n");
  wantEvents(true);
}

long ScoreGeneratorVst::processEvents(VstEvents *vstEvents)
{
  return 0;
}

void ScoreGeneratorVst::process(float **hostInput, float **hostOutput, long frames)
{
  synchronizeScore();
  sendEvents(frames);
}

void ScoreGeneratorVst::processReplacing(float **hostInput, float **hostOutput, long frames)
{
  synchronizeScore();
  sendEvents(frames);
}

void ScoreGeneratorVst::reset()
{
  vstSr = updateSampleRate();
  vstPriorSampleBlockStart = 0;
  vstCurrentSampleBlockStart = 0;
  vstCurrentSampleBlockEnd = 0;
  vstMidiEventsIterator = vstMidiEvents.begin();
  vstEvents.numEvents = 0;
}

void ScoreGeneratorVst::synchronizeScore()
{
  vstPriorSampleBlockStart = vstCurrentSampleBlockStart;
  VstTimeInfo *vstTimeInfo = getTimeInfo(kVstTransportPlaying);
  if ((vstTimeInfo->flags & kVstTransportPlaying) == kVstTransportPlaying)
    {
      vstCurrentSampleBlockStart = vstTimeInfo->samplePos;
    }
  if (vstPriorSampleBlockStart > vstCurrentSampleBlockStart) {
    vstMidiEventsIterator = vstMidiEvents.begin();
    vstEvents.numEvents = 0;
  }
}

void ScoreGeneratorVst::sendEvents(long frames)
{
  if (frames == 0) {
    return;
  }
  vstMidiEventsBuffer.clear();
  vstCurrentSampleBlockEnd = vstCurrentSampleBlockStart + frames;
  for(;;) {
    if (vstMidiEventsIterator == vstMidiEvents.end()) {
      return;
    }
    const VstMidiEvent &currentVstMidiEvent = *vstMidiEventsIterator;
    if (currentVstMidiEvent.deltaFrames < vstCurrentSampleBlockStart || currentVstMidiEvent.deltaFrames >= vstCurrentSampleBlockEnd) {
      return;
    }
    VstMidiEvent outputEvent = currentVstMidiEvent;
    outputEvent.deltaFrames = currentVstMidiEvent.deltaFrames - vstCurrentSampleBlockStart;
    vstMidiEventsBuffer.push_back(outputEvent);
    ++vstMidiEventsIterator;
  }
  vstEvents.events = &vstMidiEventsBuffer.front();
  vstEvents.numEvents = vstMidiEventsBuffer.size();
  sendVstEventsToHost(&vstEvents);
}

bool ScoreGeneratorVst::getInputProperties(long index, VstPinProperties* properties)
{
  if(index < kNumInputs)
    {
      sprintf(properties->label, "My %ld In", index + 1);
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

bool ScoreGeneratorVst::getOutputProperties(long index, VstPinProperties* properties)
{
  if(index < kNumOutputs)
    {
      sprintf(properties->label, "My %ld Out", index + 1);
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

bool ScoreGeneratorVst::getProgramNameIndexed(long category, long index, char* text)
{
  if(index < kNumPrograms)
    {
      strcpy(text, bank[curProgram].name.c_str());
      return true;
    }
  return false;
}

bool ScoreGeneratorVst::getEffectName(char* name)
{
  strcpy(name, "ScoreGeneratorVst");
  return true;
}

bool ScoreGeneratorVst::getVendorString(char* text)
{
  strcpy(text, "Irreducible Productions");
  return true;
}

bool ScoreGeneratorVst::getProductString(char* text)
{
  strcpy(text, "ScoreGeneratorVst");
  return true;
}

long ScoreGeneratorVst::canDo(char* text)
{
  csound::System::inform("RECEIVED ScoreGeneratorVst::canDo('%s')...\n", text);
  if(strcmp(text, "receiveVstTimeInfo") == 0)
    {
      return 1;
    }
  if(strcmp(text, "receiveVstEvents") == 0)
    {
      return 0;
    }
  if(strcmp(text, "receiveVstMidiEvents") == 0)
    {
      return 0;
    }
  if(strcmp(text, "sendVstMidiEvents") == 0)
    {
      return 1;
    }
  if(strcmp(text, "plugAsChannelInsert") == 0)
    {
      return 1;
    }
  if(strcmp(text, "plugAsSend") == 0)
    {
      return 1;
    }
  if(strcmp(text, "sizeWindow") == 0)
    {
      return 1;
    }
  if(strcmp(text, "asyncProcessing") == 0)
    {
      return 0;
    }
  if(strcmp(text, "2in2out") == 0)
    {
      return 1;
    }
  return 0;
}

bool ScoreGeneratorVst::keysRequired()
{
  log("RECEIVED ScoreGeneratorVst::keysRequired...\n");
  return 1;
}

long ScoreGeneratorVst::getProgram()
{
  return curProgram;
}

bool ScoreGeneratorVst::copyProgram(long destination)
{
  log("RECEIVED ScoreGeneratorVst::copyProgram(%d)...\n", destination);
  if(destination < kNumPrograms)
    {
      bank[destination] = bank[curProgram];
      return true;
    }
  return false;
}

long ScoreGeneratorVst::getChunk(void** data, bool isPreset)
{
  log("BEGAN ScoreGeneratorVst::getChunk(%d)...\n", (int) isPreset);
  ((ScoreGeneratorVstFltk *)getEditor())->updateModel();
  long returnValue = 0;
  static std::string bankBuffer;
  bank[curProgram].text = getText();
  if(isPreset)
    {
      *data = (void *)bank[curProgram].text.c_str();
      returnValue = (long) strlen((char *)*data) + 1;
    }
  else
    {
      std::ostringstream stream;
      int n = bank.size();
      stream << n << "\n";
      for(std::vector<Preset>::iterator it = bank.begin(); it != bank.end(); ++it)
        {
          Preset &preset = (*it);
          stream << preset.name.c_str() << "\n";
          stream << preset.text.size() << "\n";
          for(std::string::iterator jt = preset.text.begin(); jt != preset.text.end(); ++jt)
            {
              stream.put(*jt);
            }
          stream << "\n";
        }
      bankBuffer = stream.str();
      *data = (void *)bankBuffer.c_str();
      returnValue = bankBuffer.size();
    }
  log("ENDED ScoreGeneratorVst::getChunk, returned %d...\n", returnValue);
  return returnValue;
}

long ScoreGeneratorVst::setChunk(void* data, long byteSize, bool isPreset)
{
  log("RECEIVED ScoreGeneratorVst::setChunk(%d, %d)...\n", byteSize, (int) isPreset);
  long returnValue = 0;
  if(isPreset)
    {
      bank[curProgram].text = dupstr((const char *)data);
      setText(bank[curProgram].text);
      returnValue = byteSize;
    }
  else
    {
#if defined(__GNUC__)
      std::string inputBuffer = (const char *)data;
      std::istringstream stream(inputBuffer);
#else
      std::istringstream stream(dupstr((const char *)data), byteSize);
#endif
      std::string buffer;
      stream >> buffer;
      stream >> std::ws;
      int n = atoi(buffer.c_str());
      bank.resize(n);
      for(int i = 0; i < n; i++)
        {
          Preset preset;
          stream >> preset.name;
          stream >> std::ws;
          stream >> buffer;
          stream >> std::ws;
          int length = atoi(buffer.c_str());
          preset.text.resize(length);
          char c;
          for(int j = 0; j < length; j++)
            {
              stream.get(c);
              preset.text[j] = c;
            }
          bank[i] = preset;
        }
      returnValue = byteSize;
    }
  setProgram(curProgram);
  editor->update();
  return returnValue;
}

std::string ScoreGeneratorVst::getText()
{
  log("BEGAN ScoreGeneratorVst::getText...");
  std::string buffer;
  buffer = getScript();
  log("ENDED ScoreGeneratorVst::getText.");
  return buffer;
}

void ScoreGeneratorVst::setText(const std::string text)
{
  setScript(text);
}

void ScoreGeneratorVst::openFile(std::string filename_)
{
  WaitCursor wait;
  load(filename_);
  setFilename(filename_);
  bank[getProgram()].text = getText();
  editor->update();
  log("Opened file: '%s'.\n",
      filename.c_str());
  std::string drive, base, file, extension;
  csound::System::parsePathname(filename_, drive, base, file, extension);
  chdir(base.c_str());
}

int ScoreGeneratorVst::generate()
{
  clearEvents();
  Shell::run();
  sortEvents();
  alive = true;
}

void ScoreGeneratorVst::clearEvents()
{
  alive = false;
  vstEvents.numEvents = 0;
  vstMidiEvents.clear();
  reset();
}

/**
 * The user calls this function to append events to the generated score. 
 * The events are sorted before performance. 
 * During performance, the stored events are sent to the host 
 * at a time relative to the beginning of the track or part.
 * If the event is a note on event and duration is greater than 0,
 * a matching note off event is also created.
 */
void ScoreGeneratorVst::addEvent(double start, double duration, double status, double channel, double data1, double data2)
{
  midiopcode = char(status) & char(0xf0);
  midichannel = char(channel) & char(0xf);
  midistatus = midiopcode | midichannel;
  char detune = 0;
  // Round down.
  if (midiopcode == 0x90) {
    midikey = char(data1 + 0.5) & char(0xf0);
    detune = char((data1 - double(midikey)) / 100.);
  }
  midivelocity = char(data2) & char(0xf0);
  VstMidiEvent noteon;
  vstMidiEvent.type      = kVstMidiType;
  noteon.byteSize        = 24;
  noteon.deltaFrames     = long(vstSr * start);
  noteon.flags           = 0;
  noteon.noteLength      = long(vstSr * duration);
  noteon.noteOffset      = 0;
  noteon.midiData[0]     = midistatus;
  noteon.midiData[1]     = midikey;
  noteon.midiData[2]     = midivelocity;
  noteon.midiData[3]     = 0;
  noteon.detune          = detune;
  vstMidiEvents.push_back(noteon);
//   if (duration > 0) {
//     double stop = start + duration;
//     VstMidiEvent noteoff = noteon;
//     noteoff.deltaFrames = long(vstSr * stop);
//     noteoff.midiData[0] = midistatus - char(0x10);
//     noteoff.midiData[2] = char(0);
//     vstMidiEvents.push_back(noteoff);
//   }
}

/**
 * First, sorts stored events by time;
 * second, translates absolute times to delta times.
 */
void ScoreGeneratorVst::sortEvents()
{
  std::sort(vstMidiEvents.begin(), vstMidiEvents.end(), MidiSort());
  for (size_t i = 1, n = vstMidiEvents.size(); i < n; i++) {
    vstMidiEvents[i].deltaFrames = vstMidiEvents[i].deltaFrames - vstMidiEvents
  }
}

/**
 * This is the only other function that is exposed in Python. It enables Python to write to the messages browser
 * instead of the console stdout.
 */
void ScoreGeneratorVst::log(char *message)
{
  if (scoreGeneratorVstFltk) {
    scoreGeneratorVstFltk->log(message);
  }
}

