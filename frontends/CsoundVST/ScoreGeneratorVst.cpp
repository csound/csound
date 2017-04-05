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

bool debug = false;

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

ScoreGeneratorVst::ScoreGeneratorVst() :
  AudioEffectX(0, kNumPrograms, 0),
  vstEventsPointer(0),
  vstFramesPerSecond(0),
  vstSecondsPerFrame(0),
  vstCurrentBlockStart(0),
  vstCurrentBlockStartFrame(0),
  vstInputLatency(0),
  vstInputLatencySeconds(0),
  vstTransportActive(false),
  alive(0),
  scoreGeneratorVstFltk(0),
  score(0)
{
  open();
  scoreGeneratorVstFltk = new ScoreGeneratorVstFltk(this);
  setEditor(scoreGeneratorVstFltk);
  programsAreChunks(true);
  curProgram = 0;
  bank.resize(kNumPrograms);
  vstEventsPointer = (VstEvents *) calloc(100000, sizeof(VstEvents *));
  for(size_t i = 0; i < bank.size(); i++) {
      char buffer[0x24];
      sprintf(buffer, "Program%d", (int)(i + 1));
      bank[i].name = buffer;
  }
}

ScoreGeneratorVst::ScoreGeneratorVst(audioMasterCallback audioMaster) :
  AudioEffectX(audioMaster, kNumPrograms, 0),
  vstEventsPointer(0),
  vstFramesPerSecond(0),
  vstSecondsPerFrame(0),
  vstCurrentBlockStart(0),
  vstCurrentBlockStartFrame(0),
  vstInputLatency(0),
  vstInputLatencySeconds(0),
  vstTransportActive(false),
  alive(0),
  scoreGeneratorVstFltk(0),
  score(0)
{
  setNumInputs(kNumInputs);             // stereo in
  setNumOutputs(kNumOutputs);           // stereo out
  setUniqueID('sGsT');  // identify
  canMono(true);                            // makes sense to feed both inputs with the same signal
  canProcessReplacing(true);        // supports both accumulating and replacing output
  wantEvents();
  open();
  isSynth(false);
  scoreGeneratorVstFltk = new ScoreGeneratorVstFltk(this);
  setEditor(scoreGeneratorVstFltk);
  programsAreChunks(true);
  curProgram = 0;
  bank.resize(kNumPrograms);
  vstEventsPointer = (VstEvents *) calloc(100000, sizeof(VstEvents *));
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
    csound::Shell::runScript();
  }
}

void ScoreGeneratorVst::closeView()
{
  editor->close();
}

void ScoreGeneratorVst::setProgram(VstInt32 program)
{
  if (debug) logv("RECEIVED ScoreGeneratorVst::setProgram(%d)...\n", program);
  if(program < kNumPrograms && program >= 0)
    {
      curProgram = program;
      setText(bank[curProgram].text);
    }
}

void ScoreGeneratorVst::suspend()
{
  if (debug) log("RECEIVED ScoreGeneratorVst::suspend()...\n");
}

void ScoreGeneratorVst::resume()
{
  if (debug) log("RECEIVED ScoreGeneratorVst::resume()...\n");
  generate();
  wantEvents(true);
}

VstInt32 ScoreGeneratorVst::processEvents(VstEvents *vstEvents)
{
  return 1;
}

void ScoreGeneratorVst::process(float **hostInput, float **hostOutput, VstInt32 frames)
{
  synchronizeScore(frames);
  if (alive) {
    sendEvents(frames);
  }
}

void ScoreGeneratorVst::processReplacing(float **hostInput, float **hostOutput, VstInt32 frames)
{
  synchronizeScore(frames);
  if (alive) {
    sendEvents(frames);
  }
}

bool ScoreGeneratorVst::getInputProperties(VstInt32 index, VstPinProperties* properties)
{
  if(index < kNumInputs)
    {
      sprintf(properties->label, "My %ld In", index + 1);
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

bool ScoreGeneratorVst::getOutputProperties(VstInt32 index, VstPinProperties* properties)
{
  if(index < kNumOutputs)
    {
      sprintf(properties->label, "My %ld Out", index + 1);
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

VstPlugCategory ScoreGeneratorVst::getPlugCategory()
{
  return kPlugCategEffect;
}

bool ScoreGeneratorVst::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
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

VstInt32 ScoreGeneratorVst::canDo(char* text)
{
  if (debug) logv("RECEIVED ScoreGeneratorVst::canDo('%s')...\n", text);
  if(strcmp(text, "receiveVstTimeInfo") == 0)
    {
      return 1;
    }
  if(strcmp(text, "receiveVstEvents") == 0)
    {
      return 1;
    }
  if(strcmp(text, "receiveVstMidiEvents") == 0)
    {
      return 1;
    }
  if(strcmp(text, "sendVstEvents") == 0)
    {
      return 1;
    }
  if(strcmp(text, "sendVstMidiEvent") == 0)
    {
      return 1;
    }
  if(strcmp(text, "plugAsChannelInsert") == 0)
    {
      return -1;
    }
  if(strcmp(text, "plugAsSend") == 0)
    {
      return -1;
    }
  if(strcmp(text, "sizeWindow") == 0)
    {
      return 1;
    }
  if(strcmp(text, "asyncProcessing") == 0)
    {
      return -1;
    }
  if(strcmp(text, "2in2out") == 0)
    {
      return 1;
    }
  return -1;
}

bool ScoreGeneratorVst::keysRequired()
{
  if (debug) log("RECEIVED ScoreGeneratorVst::keysRequired...\n");
  return 1;
}

VstInt32 ScoreGeneratorVst::getProgram()
{
  return curProgram;
}

bool ScoreGeneratorVst::copyProgram(VstInt32 destination)
{
  if (debug) logv("RECEIVED ScoreGeneratorVst::copyProgram(%d)...\n", destination);
  if(destination < kNumPrograms)
    {
      bank[destination] = bank[curProgram];
      return true;
    }
  return false;
}

VstInt32 ScoreGeneratorVst::getChunk(void** data, bool isPreset)
{
  if (debug) logv("BEGAN ScoreGeneratorVst::getChunk(%d)...\n", (int) isPreset);
  ((ScoreGeneratorVstFltk *)getEditor())->updateModel();
  VstInt32 returnValue = 0;
  static std::string bankBuffer;
  bank[curProgram].text = getText();
  if(isPreset)
    {
      bankBuffer = bank[curProgram].text.c_str();
      *data = (void *)bankBuffer.c_str();
      returnValue = bankBuffer.size();
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
  if (debug) logv("ENDED ScoreGeneratorVst::getChunk, returned %d...\n", returnValue);
  return returnValue;
}

VstInt32 ScoreGeneratorVst::setChunk(void* data, VstInt32 byteSize, bool isPreset)
{
  if (debug) logv("RECEIVED ScoreGeneratorVst::setChunk(%d, %d)...\n", byteSize, (int) isPreset);
  VstInt32 returnValue = 0;
  if(isPreset)
    {
      bank[curProgram].text.assign((const char *)data, byteSize);
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
  ((ScoreGeneratorVstFltk *)getEditor())->updateModel();
  return returnValue;
}

std::string ScoreGeneratorVst::getText()
{
  if (debug) log("BEGAN ScoreGeneratorVst::getText...\n");
  std::string text;
  text = getScript();
  if (debug) {
    logv("Text = %s\n", text.c_str());
    log("ENDED ScoreGeneratorVst::getText.\n");
  }
  return text;
}

void ScoreGeneratorVst::setText(const std::string text)
{
  if (debug) log("BEGAN ScoreGeneratorVst::setText...\n");
  setScript(text);
  if (debug) {
    logv("Text = %s\n", text.c_str());
    log("ENDED ScoreGeneratorVst::setText.\n");
  }
}

void ScoreGeneratorVst::openFile(std::string filename_)
{
  WaitCursor wait;
  load(filename_);
  setFilename(filename_);
  bank[getProgram()].text = getText();
  ((ScoreGeneratorVstFltk *)getEditor())->updateModel();
  logv("Opened file: '%s'.\n",
      filename.c_str());
  std::string drive, base, file, extension;
  csound::System::parsePathname(filename_, drive, base, file, extension);
  chdir(base.c_str());
}

void ScoreGeneratorVst::open()
{
  int result = 0;
  Shell::open();
  char *argv[] = {"",""};
  PySys_SetArgv(1, argv);
  /* Sanitize sys.path */
  PyRun_SimpleString("import sys; sys.path = filter(None, sys.path)");
  PyObject *mainModule = PyImport_ImportModule("__main__");
  result = runScript("import sys\n");
  if(result)
    {
      PyErr_Print();
    }
  result = runScript("import scoregen\n");
  if(result)
    {
      PyErr_Print();
    }
  result = runScript("score = scoregen.ScoreGenerator()\n");
  if(result)
    {
      PyErr_Print();
    }
  score = PyObject_GetAttrString(mainModule, "score");
  Py_INCREF(score);
  PyObject *pyThis = PyCObject_FromVoidPtr(this, 0);
  /* PyObject *pyResult = */ PyObject_CallMethod(score,  "setScoreGeneratorVst",  "O", pyThis);
  result = runScript("sys.stdout = sys.stderr = score\n");
  if(result)
    {
      PyErr_Print();
    }
  std::string filename_ = getFilename();
}

int ScoreGeneratorVst::runScript(std::string script_)
{
  log("BEGAN ScoreGeneratorVst::runScript()...\n");
  int result = 0;
  try {
      char *script__ = const_cast<char *>(script_.c_str());
      log("==============================================================================================================\n");
      result = PyRun_SimpleString(script__);
      if(result) {
          PyErr_Print();
        }
    } catch(...) {
      log("Unidentified exception in ScoreGeneratorVst::runScript().\n");
    }
  log("==============================================================================================================\n");
  logv("PyRun_SimpleString returned %d.\n", result);
  log("ENDED ScoreGeneratorVst::runScript().\n");
  return result;
}

void ScoreGeneratorVst::reset()
{
  alive = false;
  vstFramesPerSecond = updateSampleRate();
  if (debug) logv("Reset: %f frames/second.\n", vstFramesPerSecond);
  if (vstFramesPerSecond) {
    vstSecondsPerFrame = 1.0 / vstFramesPerSecond;
    if (debug) logv("Reset: %f seconds/frame.\n", vstSecondsPerFrame);
  }
  vstCurrentBlockStart = 0.;
  vstCurrentBlockStartFrame = 0.;
  vstEventsPointer->numEvents = 0;
  vstTransportActive = false;
}

int ScoreGeneratorVst::generate()
{
  alive = false;
  log("BEGAN ScoreGeneratorVst::generate()...\n");
  reset();
  clearEvents();
  csound::Shell::runScript();
  log("ENDED ScoreGeneratorVst::generate().\n");
  alive = true;
  return true;
}

void ScoreGeneratorVst::clearEvents()
{
  vstEventsPointer->numEvents = 0;
  scoreGeneratorEvents.clear();
}

size_t ScoreGeneratorVst::event(double start, double duration, double status, double channel, double data1, double data2)
{
  int midiopcode = (int) status;
  int midichannel = (int) channel;
  int midistatus = midiopcode + midichannel;
  int midikey = 0;
  char detune = 0;
  // Round down.
  if (midiopcode == 144) {
    midikey = (int) (data1 + 0.5);
    //detune = int((data1 - double(midikey)) / 100.);
  } else {
    midikey = (int) data1;
  }
  int midivelocity = (int) data2;
  ScoreGeneratorEvent noteon;
  noteon.start = start;
  noteon.duration = duration;
  noteon.vstMidiEvent.type            = kVstMidiType;
  noteon.vstMidiEvent.byteSize        = 24;
  noteon.vstMidiEvent.deltaFrames     = 0;
  noteon.vstMidiEvent.flags           = 0;
  noteon.vstMidiEvent.noteLength      = 0;
  noteon.vstMidiEvent.noteOffset      = 0;
  noteon.vstMidiEvent.midiData[0]     = midistatus;
  noteon.vstMidiEvent.midiData[1]     = midikey;
  noteon.vstMidiEvent.midiData[2]     = midivelocity;
  noteon.vstMidiEvent.midiData[3]     = 0;
  noteon.vstMidiEvent.detune          = detune;
  noteon.vstMidiEvent.noteOffVelocity = 0;
  int onframe = int(noteon.start * vstFramesPerSecond);
  scoreGeneratorEvents.insert(std::make_pair(start, noteon));
  //if (duration > 0) {
  //  ScoreGeneratorEvent noteoff = noteon;
  //  noteoff.start = start + duration;
  //  noteoff.vstMidiEvent.midiData[0] = midistatus - 16;
  //  noteoff.vstMidiEvent.midiData[2] = 0;
  //  int offframe = noteoff.start * vstFramesPerSecond;
  //  scoreGeneratorEvents.insert(std::make_pair(offframe, noteoff));
  //}
  if (debug) {
      logv("Added event: frame=%8d, time=%12.4f, duration=%12.4f, status=%3d, key=%3d, velocity=%3d, detune=%3d\n",
      onframe,
      noteon.start,
      noteon.duration,
      (unsigned char) noteon.vstMidiEvent.midiData[0],
      (unsigned char) noteon.vstMidiEvent.midiData[1],
      (unsigned char) noteon.vstMidiEvent.midiData[2],
      (unsigned char) noteon.vstMidiEvent.detune);
  }
  return scoreGeneratorEvents.size();
}

bool ScoreGeneratorVst::synchronizeScore(VstInt32 frames)
{
  //vstInputLatency = getInputLatency();
  VstTimeInfo *vstTimeInfo = getTimeInfo(kVstTransportPlaying);
  vstFramesPerSecond = double(vstTimeInfo->sampleRate);
  vstSecondsPerFrame = double(1.0) / vstFramesPerSecond;
  //vstInputLatencySeconds = vstInputLatency * vstSecondsPerFrame;
  vstCurrentBlockStartFrame = double(vstTimeInfo->samplePos);
  vstCurrentBlockStart = vstCurrentBlockStartFrame * vstSecondsPerFrame;
  vstTransportActive = ((vstTimeInfo->flags & kVstTransportPlaying) == kVstTransportPlaying);
  //~ if (debug) {
    //~ logv("synchronizeScore: frames=%8d vstFramesPerSecond=%8.2f vstSecondsPerFrame=%12.8f vstCurrentBlockStartFrame=%8.3f vstCurrentBlockStart=%12.5f active=%4d\n",
      //~ frames,
      //~ vstFramesPerSecond,
      //~ vstSecondsPerFrame,
      //~ vstCurrentBlockStartFrame,
      //~ vstCurrentBlockStart,
      //~ vstTransportActive);
  //~ }
  return vstTransportActive;
}

void ScoreGeneratorVst::sendEvents(VstInt32 frames)
{
    if (!vstTransportActive || scoreGeneratorEvents.size() == 0) {
        return;
    }
    double lowerTime = vstCurrentBlockStart;
    double frameSeconds = vstSecondsPerFrame * double(frames);
    double upperTime = vstCurrentBlockStart + frameSeconds;
    vstEventsPointer->numEvents = 0;
    std::multimap<double, ScoreGeneratorEvent>::iterator lowerI = scoreGeneratorEvents.lower_bound(lowerTime);
    std::multimap<double, ScoreGeneratorEvent>::iterator upperI = scoreGeneratorEvents.upper_bound(upperTime);
    for(int i = 0; (lowerI != upperI) && (lowerI != scoreGeneratorEvents.end()); ++lowerI, ++i) {
        VstMidiEvent *vstMidiEventPointer = &lowerI->second.vstMidiEvent;
        vstMidiEventPointer->noteLength = int(lowerI->second.duration * vstFramesPerSecond);
        vstMidiEventPointer->deltaFrames = int((lowerI->second.start - lowerTime) * vstFramesPerSecond);
        vstEventsPointer->events[vstEventsPointer->numEvents++] = (VstEvent *)vstMidiEventPointer;
        if (debug) {
            logv("Sent event %5d: frame=%12.4f, time=%12.4f, frames=%8d, deltaFrames=%8d, length=%8d, status=%3d, key=%3d, velocity=%3d, detune=%3d\n",
                i,
                vstCurrentBlockStartFrame,
                vstCurrentBlockStart,
                frames,
                vstMidiEventPointer->deltaFrames,
                vstMidiEventPointer->noteLength,
                (unsigned char) vstMidiEventPointer->midiData[0],
                (unsigned char) vstMidiEventPointer->midiData[1],
                (unsigned char) vstMidiEventPointer->midiData[2],
                (unsigned char) vstMidiEventPointer->detune);
        }
    }
    if (vstEventsPointer->numEvents > 0) {
      bool result = sendVstEventsToHost(vstEventsPointer);
      if (debug) {
          logv("sendVstEventsToHost(%d events) returned %d\n", vstEventsPointer->numEvents, result);
      }
    }
}

void ScoreGeneratorVst::log(char *message)
{
  std::cerr << message;
  if (scoreGeneratorVstFltk) {
    scoreGeneratorVstFltk->log(message);
  }
}

void ScoreGeneratorVst::logv(char *format,...)
{
  char buffer[0x100];
  va_list marker;
  va_start(marker, format);
  vsprintf(buffer, format, marker);
  log(buffer);
  va_end(marker);
}

