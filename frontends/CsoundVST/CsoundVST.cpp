/*
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
#include "CsoundVST.hpp"
#include "CsoundVstFltk.hpp"
#include <Python.h>

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

double CsoundVST::inputScale = 32767.0;
double CsoundVST::outputScale = (1.0 / 32767.0);
void *CsoundVST::fltkWaitThreadId = 0;

CsoundVST::CsoundVST(audioMasterCallback audioMaster) :
  AudioEffectX(audioMaster, kNumPrograms, 0),
  isSynth(true),
  isMultiThreaded(true),
  csoundFrameI(0),
  csoundLastFrame(0),
  channelI(0),
  channelN(0),
  hostFrameI(0),
  vstSr(0),
  vstCurrentSampleBlockStart(0),
  vstCurrentSampleBlockEnd(0),
  vstCurrentSamplePosition(0),
  vstPriorSamplePosition(0),
  csoundVstFltk(0)
{
  if (fltkWaitThreadId == 0)
    {
      Fl::lock();
      fltkWaitThreadId == csoundGetCurrentThreadId();
    }
  setNumInputs(kNumInputs);             // stereo in
  setNumOutputs(kNumOutputs);           // stereo out
  setUniqueID('cVsT');  // identify
  canProcessReplacing();        // supports both accumulating and replacing output
  open();
  csoundVstFltk = new CsoundVstFltk(this);
  int number = 0;
  csoundVstFltk->preferences.get("IsSynth", number, 0);
  if(audioMaster)
    {
      if(number)
        {
          AudioEffectX::isSynth(true);
        }
      else
        {
          AudioEffectX::isSynth(false);
        }
    }
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

CsoundVST::CsoundVST() :
  AudioEffectX(0, kNumPrograms, 0),
  isSynth(true),
  isMultiThreaded(true),
  csoundFrameI(0),
  csoundLastFrame(0),
  channelI(0),
  channelN(0),
  hostFrameI(0),
  vstSr(0),
  vstCurrentSampleBlockStart(0),
  vstCurrentSampleBlockEnd(0),
  vstCurrentSamplePosition(0),
  vstPriorSamplePosition(0),
  csoundVstFltk(0)
{
  if (fltkWaitThreadId == 0)
    {
      Fl::lock();
      fltkWaitThreadId == csoundGetCurrentThreadId();
    }
  setNumInputs(2);              // stereo in
  setNumOutputs(2);             // stereo out
  setUniqueID('cVsT');  // identify
  canProcessReplacing();        // supports both accumulating and replacing output
  //open();
  csoundVstFltk = new CsoundVstFltk(this);
  bank.resize(kNumPrograms);
  curProgram = 0;
  for(size_t i = 0; i < bank.size(); i++)
    {
      char buffer[0x24];
      sprintf(buffer, "Program%d", i + 1);
      bank[i].name = buffer;
    }
}

CsoundVST::~CsoundVST()
{
}

bool CsoundVST::getIsSynth() const
{
  return isSynth;
}

void CsoundVST::setIsSynth(bool isSynth)
{
  this->isSynth = isSynth;
}

bool CsoundVST::getIsVst() const
{
  if (audioMaster) {
    return true;
  } else {
    return false;
  }
}

void CsoundVST::setProgramName(char *name)
{
  bank[curProgram].name = name;
}

void CsoundVST::getProgramName(char *name)
{
  strcpy(name, bank[curProgram].name.c_str());
}

AEffEditor* CsoundVST::getEditor()
{
  return editor;
}

void CsoundVST::openView(bool doRun)
{
  editor->open(0);
  if(doRun) {
    fltkrun();
  }
}

void CsoundVST::closeView()
{
  editor->close();
}

int CsoundVST::midiDeviceOpen(CSOUND *csound, void **userData,
                              const char *devName)
{
  *userData = csoundGetHostData(csound);
  return 0;
}

uintptr_t CsoundVST::performanceThreadRoutine()
{
  stop();
  Reset();
  // Translate csd-style command lines to orc/sco style.
  std::string command = getCommand();
  std::string vstcommand;
  bool updateCommand = false;
  if (command.find("-") == 0 || command.length() == 0) {
    updateCommand = true;
    vstcommand = "csound ";
  }
  vstcommand.append(command);
  if (command.find(".orc") == std::string::npos && command.find(".sco") == std::string::npos) {
    updateCommand = true;
    vstcommand.append(" temp.orc temp.sco");
  }
  if (updateCommand) {
    setCommand(vstcommand);
    std::string buffer = getCommand();
    csoundVstFltk->commandInput->value(buffer.c_str());
  }
  exportForPerformance();
  Message("Saved as: '%s' and '%s'.\n", getOrcFilename().c_str(), getScoFilename().c_str());
  reset();
  PreCompile();
  // FLTK flags is the sum of any of the following values:
  //   1:  disable widget opcodes by setting up dummy opcodes instead
  //   2:  disable FLTK graphs
  //   4:  disable the use of a separate thread for widget opcodes
  //   8:  disable the use of Fl::lock() and Fl::unlock()
  //  16:  disable the use of Fl::awake()
  // 128:  disable widget opcodes by not registering any opcodes
  // 256:  disable the use of Fl::wait() (implies no widget thread)
  CreateGlobalVariable("FLTK_Flags", sizeof(int));
  int *fltkFlags = (int *)QueryGlobalVariable("FLTK_Flags");
  //*fltkFlags = 274;
  *fltkFlags = 286;
  if(getIsVst())
    {
      Message("Classic VST performance.\n");
      SetExternalMidiInOpenCallback(&CsoundVST::midiDeviceOpen);
      SetExternalMidiReadCallback(&CsoundVST::midiRead);
      if(compile())
	{
	  Message("Csound compilation failed.\n");
	  reset();
	  stop();
	}
    }
  else
    {
      Message("Classic performance.\n");
      perform();
    }
  return 0;
}

bool CsoundVST::getIsMultiThreaded() const
{
  return isMultiThreaded;
}

void CsoundVST::setIsMultiThreaded(bool isMultiThreaded)
{
  this->isMultiThreaded = isMultiThreaded;
}

uintptr_t performanceThreadRoutine_(void *data)
{
  return ((CsoundVST *)data)->performanceThreadRoutine();
}

static int threadYieldCallback(CSOUND *csound)
{
  CsoundVST *csoundVst_ = (CsoundVST *) csoundGetHostData(csound);
  if (csoundVst_) {
    csoundVst_->fltkwait();
  }
  return 1;
}

static int nonThreadYieldCallback(CSOUND *)
{
  return 1;
}

int CsoundVST::performance()
{
  int result = true;
  if(getIsVst())
    {
      Message("VST performance.\n");
      SetYieldCallback(nonThreadYieldCallback);
      performanceThreadRoutine();
    }
  else if(getIsMultiThreaded())
    {
      Message("Multi-threaded performance.\n");
      SetYieldCallback(threadYieldCallback);
      void *result_ = csoundCreateThread(performanceThreadRoutine_, this);
      if(result_) {
	result = true;
      } else {
	result = false;
      }
      Message("Created Csound performance thread.\n");
    }
  else
    {
      Message("Single-threaded performance.\n");
      SetYieldCallback(nonThreadYieldCallback);
      performanceThreadRoutine();
    }
  return result;
}

void CsoundVST::open()
{
  //Message("BEGAN CsoundVST::open()...\n");
  SetHostData(this);
  //SetMessageCallback(Message);
  std::string filename_ = getFilename();
  if(filename_.length() > 0)
    {
      setFilename(filename_);
    }
  // setFLTKThreadLocking(false);
  //Message("ENDED CsoundVST::open().\n");
}

void CsoundVST::reset()
{
  csoundFrameI = 0;
  csoundLastFrame = 0;
  channelI = 0;
  channelN = 0;
  hostFrameI = 0;
  vstSr = 0;
  vstCurrentSampleBlockStart = 0;
  vstCurrentSampleBlockEnd = 0;
  vstCurrentSamplePosition = 0;
  vstPriorSamplePosition = 0;
  midiEventQueue.clear();
}

void CsoundVST::setProgram(VstInt32 program)
{
  //Message("RECEIVED CsoundVST::setProgram(%d)...\n", program);
  if(program < kNumPrograms && program >= 0)
    {
      curProgram = program;
      setText(bank[curProgram].text);
    }
}

void CsoundVST::suspend()
{
  Message("RECEIVED CsoundVST::suspend()...\n");
  stop();
}

void CsoundVST::resume()
{
  Message("RECEIVED CsoundVST::resume()...\n");
  performance();
}

VstInt32 CsoundVST::processEvents(VstEvents *vstEvents)
{
  if(getIsGo())
    {
      for(int i = 0; i < vstEvents->numEvents; i++)
        {
          if(vstEvents->events[i]->type == kVstMidiType)
            {
              VstMidiEvent *vstMidiEvent = (VstMidiEvent *)vstEvents->events[i];
              midiEventQueue.push_back(*vstMidiEvent);
            }
        }
      return 1;
    }
  else
    {
      return 0;
    }
}

int CsoundVST::midiRead(CSOUND *csound, void *userData,
                        unsigned char *midiData, int nbytes)
{
  CsoundVST *csoundVST = (CsoundVST *)userData;
  int cnt = 0;
  while (!csoundVST->midiEventQueue.empty() && cnt <= (nbytes - 3)) {
    const VstMidiEvent &event = csoundVST->midiEventQueue.front();
    midiData[cnt + 0] = (unsigned char) event.midiData[0];
    midiData[cnt + 1] = (unsigned char) event.midiData[1];
    midiData[cnt + 2] = (unsigned char) event.midiData[2];
    csoundVST->midiEventQueue.pop_front();
    //~ Message("CsoundVST::midiRead(%x, %x, %x)\n",
    //~ (int) midiData[cnt + 0],
    //~ (int) midiData[cnt + 1],
    //~ (int) midiData[cnt + 2]);
    switch ((int) midiData[cnt] & 0xF0) {
    case 0x80:    /* note off */
    case 0x90:    /* note on */
    case 0xA0:    /* polyphonic pressure */
    case 0xB0:    /* control change */
    case 0xE0:    /* pitch bend */
      cnt += 3;
      break;
    case 0xC0:    /* program change */
    case 0xD0:    /* channel pressure */
      cnt += 2;
      break;
    case 0xF0:
      switch ((int) midiData[cnt]) {
      case 0xF8:    /* timing clock */
      case 0xFA:    /* start */
      case 0xFB:    /* continue */
      case 0xFC:    /* stop */
      case 0xFF:    /* reset */
        cnt++;
      }
      break;      /* ignore any other message */
    }
  }
  return cnt;
}

void CsoundVST::process(float **hostInput, float **hostOutput, VstInt32 hostFrameN)
{
  if(getIsGo())
    {
      synchronizeScore();
      MYFLT *csoundInput = GetSpin();
      MYFLT *csoundOutput = GetSpout();
      size_t csoundLastFrame = GetKsmps() - 1;
      size_t channelN = GetNchnls();
      size_t channelI;
      for(VstInt32 hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++)
        {
          for(channelI = 0; channelI < channelN; channelI++)
            {
              csoundInput[(csoundFrameI * channelN) + channelI] = hostInput[channelI][hostFrameI];
            }
          if(csoundFrameI == 0)
            {
              performKsmps(true);
            }
          for(channelI = 0; channelI < channelN; channelI++)
            {
              hostOutput[channelI][hostFrameI] += csoundOutput[(csoundFrameI * channelN) + channelI] * outputScale;
              csoundOutput[(csoundFrameI * channelN) + channelI] = 0.0;
            }
          csoundFrameI++;
          if(csoundFrameI > csoundLastFrame)
            {
              csoundFrameI = 0;
            }
        }
    }
  else 
    {
      for (VstInt32 hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++) 
	{
	  for (channelI = 0; channelI < channelN; channelI++) 
	    {
	      hostOutput[channelI][hostFrameI] += hostInput[channelI][hostFrameI];
	    }
	}
    }
}

void CsoundVST::processReplacing(float **hostInput, float **hostOutput, VstInt32 hostFrameN)
{
  if(getIsGo())
    {
      synchronizeScore();
      MYFLT *csoundInput = GetSpin();
      MYFLT *csoundOutput = GetSpout();
      size_t csoundLastFrame = GetKsmps() - 1;
      size_t channelN = GetNchnls();
      size_t channelI;
      for(VstInt32 hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++)
        {
          for(channelI = 0; channelI < channelN; channelI++)
            {
              csoundInput[(csoundFrameI * channelN) + channelI] = hostInput[channelI][hostFrameI];
            }
          if(csoundFrameI == 0)
            {
              performKsmps(true);
            }
          for(channelI = 0; channelI < channelN; channelI++)
            {
              hostOutput[channelI][hostFrameI] = csoundOutput[(csoundFrameI * channelN) + channelI] *  outputScale;
              csoundOutput[(csoundFrameI * channelN) + channelI] = 0.0;
            }
          csoundFrameI++;
          if(csoundFrameI > csoundLastFrame)
            {
              csoundFrameI = 0;
            }
        }
    }
  else 
    {
      for (VstInt32 hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++) 
	{
	  for (channelI = 0; channelI < channelN; channelI++) 
	    {
	      hostOutput[channelI][hostFrameI] = hostInput[channelI][hostFrameI];
	    }
	}
    }
}

void CsoundVST::synchronizeScore()
{
  vstPriorSamplePosition = vstCurrentSamplePosition;
  VstTimeInfo *vstTimeInfo = getTimeInfo(kVstTransportPlaying);
  if ((vstTimeInfo->flags & kVstTransportPlaying) == kVstTransportPlaying)
    {
      vstSr = double(vstTimeInfo->sampleRate);
      vstCurrentSamplePosition = (int) vstTimeInfo->samplePos;
      vstCurrentSampleBlockStart = vstTimeInfo->samplePos / vstSr;
      if((vstCurrentSamplePosition && !vstPriorSamplePosition) ||
         (vstCurrentSamplePosition < vstPriorSamplePosition))
        {
          if(getIsGo())
            {
              SetScorePending(1);
              RewindScore();
              SetScoreOffsetSeconds(vstCurrentSampleBlockStart);
              Message("Score synchronized at %f...\n", vstCurrentSampleBlockStart);
            }
        }
    }
  else
    {
      if(getIsGo())
        {
          SetScorePending(0);
        }
    }
}

bool CsoundVST::getInputProperties(VstInt32 index, VstPinProperties* properties)
{
  if(index < kNumInputs)
    {
      sprintf(properties->label, "My %d In", index + 1);
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

bool CsoundVST::getOutputProperties(VstInt32 index, VstPinProperties* properties)
{
  if(index < kNumOutputs)
    {
      sprintf(properties->label, "My %d Out", index + 1);
      properties->flags = kVstPinIsStereo | kVstPinIsActive;
      return true;
    }
  return false;
}

bool CsoundVST::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
{
  if(index < kNumPrograms)
    {
      strcpy(text, bank[curProgram].name.c_str());
      return true;
    }
  return false;
}

bool CsoundVST::getEffectName(char* name)
{
  strcpy(name, "CsoundVST");
  return true;
}

bool CsoundVST::getVendorString(char* text)
{
  strcpy(text, "Irreducible Productions");
  return true;
}

bool CsoundVST::getProductString(char* text)
{
  strcpy(text, "CsoundVST");
  return true;
}

VstInt32 CsoundVST::canDo(char* text)
{
  Message("RECEIVED CsoundVST::canDo('%s')...\n", text);
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
      return 1;
    }
  if(strcmp(text, "2in2out") == 0)
    {
      return 1;
    }
  return 0;
}

bool CsoundVST::keysRequired()
{
  Message("RECEIVED CsoundVST::keysRequired...\n");
  return 1;
}

VstInt32 CsoundVST::getProgram()
{
  //Message("RECEIVED CsoundVST::getProgram...\n");
  //bank[curProgram].text = getText();
  return curProgram;
}

bool CsoundVST::copyProgram(VstInt32 destination)
{
  Message("RECEIVED CsoundVST::copyProgram(%d)...\n", destination);
  if(destination < kNumPrograms)
    {
      bank[destination] = bank[curProgram];
      return true;
    }
  return false;
}

VstInt32 CsoundVST::getChunk(void** data, bool isPreset)
{
  Message("BEGAN CsoundVST::getChunk(%d)...\n", (int) isPreset);
  ((CsoundVstFltk *)getEditor())->updateModel();
  VstInt32 returnValue = 0;
  static std::string bankBuffer;
  bank[curProgram].text = getText();
  if(isPreset)
    {
      *data = (void *)bank[curProgram].text.c_str();
      returnValue = (VstInt32) strlen((char *)*data) + 1;
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
  Message("ENDED CsoundVST::getChunk, returned %d...\n", returnValue);
  return returnValue;
}

VstInt32 CsoundVST::setChunk(void* data, VstInt32 byteSize, bool isPreset)
{
  Message("RECEIVED CsoundVST::setChunk(%d, %d)...\n", byteSize, (int) isPreset);
  VstInt32 returnValue = 0;
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
  ((CsoundVstFltk *)getEditor())->updateModel();
  return returnValue;
}

std::string CsoundVST::getText()
{
  //Message("BEGAN CsoundVST::getText...\n");
  std::string buffer = getCSD();
  //Message("ENDED CsoundVST::getText.\n");
  return buffer;
}

void CsoundVST::setText(const std::string text)
{
  setCSD(text);
}

void CsoundVST::openFile(std::string filename_)
{
  WaitCursor wait;
  load(filename_);
  bank[getProgram()].text = getText();
  ((CsoundVstFltk *)getEditor())->update();
  setFilename(filename_);
}

void CsoundVST::fltklock()
{
  if (fltkWaitThreadId != csoundGetCurrentThreadId()) {
    Fl::lock();
  }
}

void CsoundVST::fltkunlock()
{
  if (fltkWaitThreadId != csoundGetCurrentThreadId()) {
    Fl::awake();
    Fl::unlock();
  } else {
    Fl::awake();
  }
}

void CsoundVST::fltkflush()
{
  fltklock();
  Fl::flush();
  fltkunlock();
}

void CsoundVST::fltkwait()
{
  if (fltkWaitThreadId == csoundGetCurrentThreadId() || getIsVst()) {
    Fl::wait(0.0);
  }
}

int CsoundVST::fltkrun()
{
  int status = -100;
  //Message("BEGAN CsoundVST::run()...\n");
  if (!getIsVst()) {
    status = Fl::run();
  } else {
    Message("Sorry, can't run FLTK if running as a VST plugin.\n");
  }
  //Message("ENDED CsoundVST::fltkrun().\n");
  return status;
}

extern "C"
{
  CsoundVST* SILENCE_PUBLIC CreateCsoundVST()
  {
    CsoundVST *csoundVST = new CsoundVST;
    std::fprintf(stderr, "CreateCsoundVST: created 0x%x\n", csoundVST);
    return csoundVST;
  }
}

