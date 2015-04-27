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
#include <cstring>
#include <cstdlib>

const char *getNameForOpcode(int opcode)
{
    static bool initialized = false;
    static std::map<int, const char*> namesForOpcodes;
    if (!initialized) {
        namesForOpcodes[effOpen] = "effOpen"; // = 0,           ///< no arguments  @see AudioEffect::open
        namesForOpcodes[effClose] = "effClose"; //,             ///< no arguments  @see AudioEffect::close
        namesForOpcodes[effSetProgram] = "effSetProgram"; //,           ///< [value]: new program number  @see AudioEffect::setProgram
        namesForOpcodes[effGetProgram] = "effGetProgram"; //,           ///< [return value]: current program number  @see AudioEffect::getProgram
        namesForOpcodes[effSetProgramName] = "effSetProgramName"; //,   ///< [ptr]: char* with new program name, limited to #kVstMaxProgNameLen  @see AudioEffect::setProgramName
        namesForOpcodes[effGetProgramName] = "effGetProgramName"; //,   ///< [ptr]: char buffer for current program name, limited to #kVstMaxProgNameLen  @see AudioEffect::getProgramName
        namesForOpcodes[effGetParamLabel] = "effGetParamLabel"; //,     ///< [ptr]: char buffer for parameter label, limited to #kVstMaxParamStrLen  @see AudioEffect::getParameterLabel
        namesForOpcodes[effGetParamDisplay] = "effGetParamDisplay"; //, ///< [ptr]: char buffer for parameter display, limited to #kVstMaxParamStrLen  @see AudioEffect::getParameterDisplay
        namesForOpcodes[effGetParamName] = "effGetParamName"; //,       ///< [ptr]: char buffer for parameter name, limited to #kVstMaxParamStrLen  @see AudioEffect::getParameterName
        namesForOpcodes[effGetVu] = "effGetVu"; //),    ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effSetSampleRate] = "effSetSampleRate"; //,     ///< [opt]: new sample rate for audio processing  @see AudioEffect::setSampleRate
        namesForOpcodes[effSetBlockSize] = "effSetBlockSize"; //,       ///< [value]: new maximum block size for audio processing  @see AudioEffect::setBlockSize
        namesForOpcodes[effMainsChanged] = "effMainsChanged"; //,       ///< [value]: 0 means "turn off", 1 means "turn on"  @see AudioEffect::suspend @see AudioEffect::resume
        namesForOpcodes[effEditGetRect] = "effEditGetRect"; //,         ///< [ptr]: #ERect** receiving pointer to editor size  @see ERect @see AEffEditor::getRect
        namesForOpcodes[effEditOpen] = "effEditOpen"; //,               ///< [ptr]: system dependent Window pointer, e.g. HWND on Windows  @see AEffEditor::open
        namesForOpcodes[effEditClose] = "effEditClose"; //,             ///< no arguments @see AEffEditor::close
        namesForOpcodes[effEditDraw] = "effEditDraw"; //),      ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effEditMouse] = "effEditMouse"; //),    ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effEditKey] = "effEditKey"; //),        ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effEditIdle] = "effEditIdle"; //,               ///< no arguments @see AEffEditor::idle
        namesForOpcodes[effEditTop] = "effEditTop"; //),        ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effEditSleep] = "effEditSleep"; //),    ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effIdentify] = "effIdentify"; //),      ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effGetChunk] = "effGetChunk"; //,               ///< [ptr]: void** for chunk data address [index]: 0 for bank, 1 for program  @see AudioEffect::getChunk
        namesForOpcodes[effSetChunk] = "effSetChunk"; //,               ///< [ptr]: chunk data [value]: byte size [index]: 0 for bank, 1 for program  @see AudioEffect::setChunk
        namesForOpcodes[effNumOpcodes] = "effNumOpcodes"; //
        namesForOpcodes[effProcessEvents] = "effProcessEvents"; // = effSetChunk + 1            ///< [ptr]: #VstEvents*  @see AudioEffectX::processEvents
        namesForOpcodes[effCanBeAutomated] = "effCanBeAutomated"; //                                            ///< [index]: parameter index [return value]: 1=true0=false  @see AudioEffectX::canParameterBeAutomated
        namesForOpcodes[effString2Parameter] = "effString2Parameter"; //                                        ///< [index]: parameter index [ptr]: parameter string [return value]: true for success  @see AudioEffectX::string2parameter
        namesForOpcodes[effGetNumProgramCategories] = "effGetNumProgramCategories"; //) ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effGetProgramNameIndexed] = "effGetProgramNameIndexed"; //                              ///< [index]: program index [ptr]: buffer for program namelimited to #kVstMaxProgNameLen [return value]: true for success  @see AudioEffectX::getProgramNameIndexed
        namesForOpcodes[effCopyProgram] = "effCopyProgram"; //) ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effConnectInput] = "effConnectInput"; //)       ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effConnectOutput] = "effConnectOutput"; //)     ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effGetInputProperties] = "effGetInputProperties"; //                                    ///< [index]: input index [ptr]: #VstPinProperties* [return value]: 1 if supported  @see AudioEffectX::getInputProperties
        namesForOpcodes[effGetOutputProperties] = "effGetOutputProperties"; //                          ///< [index]: output index [ptr]: #VstPinProperties* [return value]: 1 if supported  @see AudioEffectX::getOutputProperties
        namesForOpcodes[effGetPlugCategory] = "effGetPlugCategory"; //                                  ///< [return value]: category  @see VstPlugCategory @see AudioEffectX::getPlugCategory
        namesForOpcodes[effGetCurrentPosition] = "effGetCurrentPosition"; //)   ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effGetDestinationBuffer] = "effGetDestinationBuffer"; //)       ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effOfflineNotify] = "effOfflineNotify"; //                                              ///< [ptr]: #VstAudioFile array [value]: count [index]: start flag  @see AudioEffectX::offlineNotify
        namesForOpcodes[effOfflinePrepare] = "effOfflinePrepare"; //                                            ///< [ptr]: #VstOfflineTask array [value]: count  @see AudioEffectX::offlinePrepare
        namesForOpcodes[effOfflineRun] = "effOfflineRun"; //                                                    ///< [ptr]: #VstOfflineTask array [value]: count  @see AudioEffectX::offlineRun
        namesForOpcodes[effProcessVarIo] = "effProcessVarIo"; //                                                ///< [ptr]: #VstVariableIo*  @see AudioEffectX::processVariableIo
        namesForOpcodes[effSetSpeakerArrangement] = "effSetSpeakerArrangement"; //                              ///< [value]: input #VstSpeakerArrangement* [ptr]: output #VstSpeakerArrangement*  @see AudioEffectX::setSpeakerArrangement
        namesForOpcodes[effSetBlockSizeAndSampleRate] = "effSetBlockSizeAndSampleRate"; //)     ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effSetBypass] = "effSetBypass"; //                                                      ///< [value]: 1 = bypass0 = no bypass  @see AudioEffectX::setBypass
        namesForOpcodes[effGetEffectName] = "effGetEffectName"; //                                              ///< [ptr]: buffer for effect namelimited to #kVstMaxEffectNameLen  @see AudioEffectX::getEffectName
        namesForOpcodes[effGetErrorText] = "effGetErrorText"; //)       ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effGetVendorString] = "effGetVendorString"; //                                  ///< [ptr]: buffer for effect vendor stringlimited to #kVstMaxVendorStrLen  @see AudioEffectX::getVendorString
        namesForOpcodes[effGetProductString] = "effGetProductString"; //                                        ///< [ptr]: buffer for effect vendor stringlimited to #kVstMaxProductStrLen  @see AudioEffectX::getProductString
        namesForOpcodes[effGetVendorVersion] = "effGetVendorVersion"; //                                        ///< [return value]: vendor-specific version  @see AudioEffectX::getVendorVersion
        namesForOpcodes[effVendorSpecific] = "effVendorSpecific"; //                                            ///< no definitionvendor specific handling  @see AudioEffectX::vendorSpecific
        namesForOpcodes[effCanDo] = "effCanDo"; //                                                              ///< [ptr]: "can do" string [return value]: 0: "don't know" -1: "no" 1: "yes"  @see AudioEffectX::canDo
        namesForOpcodes[effGetTailSize] = "effGetTailSize"; //                                          ///< [return value]: tail size (for example the reverb time of a reverb plug-in); 0 is default (return 1 for 'no tail')
        namesForOpcodes[effIdle] = "effIdle"; //)                               ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effGetIcon] = "effGetIcon"; //)                 ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effSetViewPosition] = "effSetViewPosition"; //) ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effGetParameterProperties] = "effGetParameterProperties"; //                            ///< [index]: parameter index [ptr]: #VstParameterProperties* [return value]: 1 if supported  @see AudioEffectX::getParameterProperties
        namesForOpcodes[effKeysRequired] = "effKeysRequired"; //)       ///< \deprecated deprecated in VST 2.4
        namesForOpcodes[effGetVstVersion] = "effGetVstVersion"; //                                              ///< [return value]: VST version  @see AudioEffectX::getVstVersion
        namesForOpcodes[effEditKeyDown] = "effEditKeyDown"; //                                          ///< [index]: ASCII character [value]: virtual key [opt]: modifiers [return value]: 1 if key used  @see AEffEditor::onKeyDown
        namesForOpcodes[effEditKeyUp] = "effEditKeyUp"; //                                                      ///< [index]: ASCII character [value]: virtual key [opt]: modifiers [return value]: 1 if key used  @see AEffEditor::onKeyUp
        namesForOpcodes[effSetEditKnobMode] = "effSetEditKnobMode"; //                                  ///< [value]: knob mode 0: circular, 1: circular relativ, 2: linear (CKnobMode in VSTGUI)  @see AEffEditor::setKnobMode
        namesForOpcodes[effGetMidiProgramName] = "effGetMidiProgramName"; //                                    ///< [index]: MIDI channel [ptr]: #MidiProgramName* [return value]: number of used programs0 if unsupported  @see AudioEffectX::getMidiProgramName
        namesForOpcodes[effGetCurrentMidiProgram] = "effGetCurrentMidiProgram"; //                              ///< [index]: MIDI channel [ptr]: #MidiProgramName* [return value]: index of current program  @see AudioEffectX::getCurrentMidiProgram
        namesForOpcodes[effGetMidiProgramCategory] = "effGetMidiProgramCategory"; //                            ///< [index]: MIDI channel [ptr]: #MidiProgramCategory* [return value]: number of used categories, 0 if unsupported  @see AudioEffectX::getMidiProgramCategory
        namesForOpcodes[effHasMidiProgramsChanged] = "effHasMidiProgramsChanged"; //                            ///< [index]: MIDI channel [return value]: 1 if the #MidiProgramName(s) or #MidiKeyName(s) have changed  @see AudioEffectX::hasMidiProgramsChanged
        namesForOpcodes[effGetMidiKeyName] = "effGetMidiKeyName"; //                                            ///< [index]: MIDI channel [ptr]: #MidiKeyName* [return value]: true if supported, false otherwise  @see AudioEffectX::getMidiKeyName
        namesForOpcodes[effBeginSetProgram] = "effBeginSetProgram"; //                                  ///< no arguments  @see AudioEffectX::beginSetProgram
        namesForOpcodes[effEndSetProgram] = "effEndSetProgram"; //                                              ///< no arguments  @see AudioEffectX::endSetProgram
        namesForOpcodes[effGetSpeakerArrangement] = "effGetSpeakerArrangement"; //                              ///< [value]: input #VstSpeakerArrangement* [ptr]: output #VstSpeakerArrangement*  @see AudioEffectX::getSpeakerArrangement
        namesForOpcodes[effShellGetNextPlugin] = "effShellGetNextPlugin"; //                                    ///< [ptr]: buffer for plug-in name, limited to #kVstMaxProductStrLen [return value]: next plugin's uniqueID  @see AudioEffectX::getNextShellPlugin
        namesForOpcodes[effStartProcess] = "effStartProcess"; //                                                ///< no arguments  @see AudioEffectX::startProcess
        namesForOpcodes[effStopProcess] = "effStopProcess"; //                                          ///< no arguments  @see AudioEffectX::stopProcess
        namesForOpcodes[effSetTotalSampleToProcess] = "effSetTotalSampleToProcess"; //              ///< [value]: number of samples to process, offline only!  @see AudioEffectX::setTotalSampleToProcess
        namesForOpcodes[effSetPanLaw] = "effSetPanLaw"; //                                                      ///< [value]: pan law [opt]: gain  @see VstPanLawType @see AudioEffectX::setPanLaw
        namesForOpcodes[effBeginLoadBank] = "effBeginLoadBank"; //                                              ///< [ptr]: #VstPatchChunkInfo* [return value]: -1: bank can't be loaded, 1: bank can be loaded, 0: unsupported  @see AudioEffectX::beginLoadBank
        namesForOpcodes[effBeginLoadProgram] = "effBeginLoadProgram"; //                                        ///< [ptr]: #VstPatchChunkInfo* [return value]: -1: prog can't be loaded, 1: prog can be loaded, 0: unsupported  @see AudioEffectX::beginLoadProgram
        namesForOpcodes[effSetProcessPrecision] = "effSetProcessPrecision"; //                          ///< [value]: @see VstProcessPrecision  @see AudioEffectX::setProcessPrecision
        namesForOpcodes[effGetNumMidiInputChannels] = "effGetNumMidiInputChannels"; //                  ///< [return value]: number of used MIDI input channels (1-15)  @see AudioEffectX::getNumMidiInputChannels
        namesForOpcodes[effGetNumMidiOutputChannels] = "effGetNumMidiOutputChannels"; //                        ///< [return value]: number of used MIDI output channels (1-15)  @see AudioEffectX::getNumMidiOutputChannels
    }
    std::map<int, const char *>::iterator it = namesForOpcodes.find(opcode);
    if (it == namesForOpcodes.end()) {
        return "No name for this opcode.";
    } else {
        return it->second;
    }
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
    CSOUNDVST_PRINT_OPCODES(0),
    csoundVstFltk(0)
{
    CSOUNDVST_PRINT_OPCODES = std::getenv("CSOUNDVST_PRINT_OPCODES");
    Message("CSOUNDVST_PRINT_OPCODES: 0x%p\n", CSOUNDVST_PRINT_OPCODES);
    if (fltkWaitThreadId == 0) {
        Fl::lock();
        fltkWaitThreadId == csoundGetCurrentThreadId();
    }
    setNumInputs(kNumInputs);
    setNumOutputs(kNumOutputs);
    setUniqueID('cVsT');
    canProcessReplacing();
    setIsSynth(true);
    open();
    csoundVstFltk = new CsoundVstFltk(this);
    int number = 0;
    csoundVstFltk->preferences.get("IsSynth", number, 0);
    if(audioMaster) {
        if(number) {
            AudioEffectX::isSynth(true);
        } else {
            AudioEffectX::isSynth(false);
        }
    }
    programsAreChunks(true);
    curProgram = 0;
    bank.resize(kNumPrograms);
    for(size_t i = 0; i < bank.size(); i++) {
        char buffer[0x24];
        sprintf(buffer, "Program%d", (int)(i + 1));
        bank[i].name = buffer;
    }
    setCommand("csound -f -h -+rtmidi=null -M0 -d -n -m7 --midi-key-oct=4 --midi-velocity=5");
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
    CSOUNDVST_PRINT_OPCODES(0),
    csoundVstFltk(0)
{
    CSOUNDVST_PRINT_OPCODES = std::getenv("CSOUNDVST_PRINT_OPCODES");
    Message("CSOUNDVST_PRINT_OPCODES: 0x%p\n", CSOUNDVST_PRINT_OPCODES);
    if (fltkWaitThreadId == 0) {
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
    for(size_t i = 0; i < bank.size(); i++) {
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

int CsoundVST::midiDeviceClose(CSOUND *csound, void *userData)
{
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
        //vstcommand.append(" temp.orc temp.sco");
    }
    if (updateCommand) {
        setCommand(vstcommand);
        std::string buffer = getCommand();
        csoundVstFltk->commandInput->value(buffer.c_str());
    }
    reset();
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
    //*fltkFlags = 286;
    *fltkFlags = 256 + 16 + 8 + 4 + 2;
    // It used to be possible to compile with an empty orc and sco without
    // crashing, but not any longer. So we test for a valid orc (contains
    // "instr" and "endin").
    const std::string &testorc = getOrchestra();
    if ((testorc.find("instr") == std::string::npos) ||
            (testorc.find("endin") == std::string::npos)) {
        Message("Csound orchestra missing or invalid.\n");
        reset();
        stop();
        return 0;
    }
    if(getIsVst()) {
        Message("Compiling for VST performance.\n");
        SetHostImplementedAudioIO(1, 0);
        SetHostImplementedMIDIIO(1);
        SetExternalMidiInOpenCallback(&CsoundVST::midiDeviceOpen);
        SetExternalMidiReadCallback(&CsoundVST::midiRead);
        SetExternalMidiInCloseCallback(&CsoundVST::midiDeviceClose);
        if(compile()) {
            Message("Csound compilation failed.\n");
            reset();
            stop();
        }
        int initialDelayFrames = GetKsmps();
        setInitialDelay(initialDelayFrames);
    } else {
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
    if(getIsVst()) {
        Message("VST performance.\n");
        SetYieldCallback(nonThreadYieldCallback);
        performanceThreadRoutine();
    } else if(getIsMultiThreaded()) {
        Message("Multi-threaded performance.\n");
        SetYieldCallback(threadYieldCallback);
        void *result_ = csoundCreateThread(performanceThreadRoutine_, this);
        if(result_) {
            result = true;
        } else {
            result = false;
        }
        Message("Created Csound performance thread.\n");
    } else {
        Message("Single-threaded performance.\n");
        SetYieldCallback(nonThreadYieldCallback);
        performanceThreadRoutine();
    }
    return result;
}

void CsoundVST::open()
{
    Message("BEGAN CsoundVST::open()...\n");
    SetHostData(this);
    std::string filename_ = getFilename();
    if(filename_.length() > 0) {
        setFilename(filename_);
    }
    // setFLTKThreadLocking(false);
    Message("ENDED CsoundVST::open().\n");
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
    if(program < kNumPrograms && program >= 0) {
        curProgram = program;
        setText(bank[curProgram].text);
    }
}

void CsoundVST::suspend()
{
    stop();
}

void CsoundVST::resume()
{
    CsoundVstFltk *ed = (CsoundVstFltk *)getEditor();
    if (ed) {
        if (ed->isOpen()) {
            ed->updateModel();
        }
    }
    performance();
}


VstInt32 CsoundVST::processEvents(VstEvents *vstEvents)
{
    if(getIsGo()) {
        for(int i = 0; i < vstEvents->numEvents; i++) {
            if(vstEvents->events[i]->type == kVstMidiType) {
                VstMidiEvent *vstMidiEvent = (VstMidiEvent *)vstEvents->events[i];
                midiEventQueue.push_back(*vstMidiEvent);
            }
        }
        return 1;
    } else {
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
        //csoundVST->Message("CsoundVST::midiRead(%x, %x, %x)\n",
        //(int) midiData[cnt + 0],
        //(int) midiData[cnt + 1],
        //(int) midiData[cnt + 2]);
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
    if(getIsGo()) {
        synchronizeScore();
        MYFLT *csoundInput = GetSpin();
        MYFLT *csoundOutput = GetSpout();
        size_t csoundLastFrame = GetKsmps() - 1;
        size_t channelN = GetNchnls();
        size_t channelI;
        for(VstInt32 hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++) {
            for(channelI = 0; channelI < channelN; channelI++) {
                csoundInput[(csoundFrameI * channelN) + channelI] = hostInput[channelI][hostFrameI];
            }
            for(channelI = 0; channelI < channelN; channelI++) {
                hostOutput[channelI][hostFrameI] += csoundOutput[(csoundFrameI * channelN) + channelI] * outputScale;
                csoundOutput[(csoundFrameI * channelN) + channelI] = 0.0;
            }
            csoundFrameI++;
            if(csoundFrameI > csoundLastFrame) {
                csoundFrameI = 0;
                performKsmps();
            }
        }
    } else {
        for (VstInt32 hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++) {
            for (channelI = 0; channelI < kNumOutputs; channelI++) {
                hostOutput[channelI][hostFrameI] += hostInput[channelI][hostFrameI];
            }
        }
    }
}

void CsoundVST::processReplacing(float **hostInput, float **hostOutput, VstInt32 hostFrameN)
{
    if(getIsGo()) {
        synchronizeScore();
        MYFLT *csoundInput = GetSpin();
        MYFLT *csoundOutput = GetSpout();
        size_t csoundLastFrame = GetKsmps() - 1;
        size_t channelN = GetNchnls();
        size_t channelI;
        for(VstInt32 hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++) {
            for(channelI = 0; channelI < channelN; channelI++) {
                csoundInput[(csoundFrameI * channelN) + channelI] = hostInput[channelI][hostFrameI];
            }
            for(channelI = 0; channelI < channelN; channelI++) {
                hostOutput[channelI][hostFrameI] = csoundOutput[(csoundFrameI * channelN) + channelI] *  outputScale;
                csoundOutput[(csoundFrameI * channelN) + channelI] = 0.0;
            }
            csoundFrameI++;
            if(csoundFrameI > csoundLastFrame) {
                csoundFrameI = 0;
                performKsmps();
            }
        }
    } else {
        for (VstInt32 hostFrameI = 0; hostFrameI < hostFrameN; hostFrameI++) {
            for (channelI = 0; channelI < kNumOutputs; channelI++) {
                hostOutput[channelI][hostFrameI] = hostInput[channelI][hostFrameI];
            }
        }
    }
}

void CsoundVST::synchronizeScore()
{
    vstPriorSamplePosition = vstCurrentSamplePosition;
    VstTimeInfo *vstTimeInfo = getTimeInfo(kVstTransportPlaying);
    if ((vstTimeInfo->flags & kVstTransportPlaying) == kVstTransportPlaying) {
        vstSr = double(vstTimeInfo->sampleRate);
        vstCurrentSamplePosition = (int) vstTimeInfo->samplePos;
        vstCurrentSampleBlockStart = vstTimeInfo->samplePos / vstSr;
        if((vstCurrentSamplePosition && !vstPriorSamplePosition) ||
                (vstCurrentSamplePosition < vstPriorSamplePosition)) {
            //if(getIsGo()) {
                // This is a workaround to enable alwayson to stay on in the VST context.
                SetScoreOffsetSeconds(vstCurrentSampleBlockStart);
                csoundCompileOrc(csound, getOrchestra().c_str());
            //}
        }
    }
}

bool CsoundVST::getInputProperties(VstInt32 index, VstPinProperties* properties)
{
    if(index < kNumInputs) {
        sprintf(properties->label, "My %d In", index + 1);
        properties->flags = kVstPinIsStereo | kVstPinIsActive;
        return true;
    }
    return false;
}

bool CsoundVST::getOutputProperties(VstInt32 index, VstPinProperties* properties)
{
    if(index < kNumOutputs) {
        sprintf(properties->label, "My %d Out", index + 1);
        properties->flags = kVstPinIsStereo | kVstPinIsActive;
        return true;
    }
    return false;
}

bool CsoundVST::getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text)
{
    if(index < kNumPrograms) {
        strcpy(text, bank[index].name.c_str());
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
    if(strcmp(text, "receiveVstTimeInfo") == 0) {
        return 1;
    }
    if(strcmp(text, "receiveVstEvents") == 0) {
        return 1;
    }
    if(strcmp(text, "receiveVstMidiEvent") == 0) {
        return 1;
    }
    return 0;
}

bool CsoundVST::keysRequired()
{
    return 1;
}

VstInt32 CsoundVST::getProgram()
{
    //bank[curProgram].text = getText();
    return curProgram;
}

bool CsoundVST::copyProgram(VstInt32 destination)
{
    if(destination < kNumPrograms) {
        bank[destination] = bank[curProgram];
        return true;
    }
    return false;
}

VstInt32 CsoundVST::getChunk(void** data, bool isPreset)
{
    ((CsoundVstFltk *)getEditor())->updateModel();
    VstInt32 returnValue = 0;
    static std::string bankBuffer;
    bank[curProgram].text = getText();
    if(isPreset) {
        bankBuffer = bank[curProgram].text;
        *data = (void *)bankBuffer.c_str();
        returnValue = bankBuffer.size();
    } else {
        std::ostringstream stream;
        int n = bank.size();
        stream << n << "\n";
        for(std::vector<Preset>::iterator it = bank.begin(); it != bank.end(); ++it) {
            Preset &preset = (*it);
            stream << preset.name.c_str() << "\n";
            stream << preset.text.size() << "\n";
            for(std::string::iterator jt = preset.text.begin(); jt != preset.text.end(); ++jt) {
                stream.put(*jt);
            }
            stream << "\n";
        }
        bankBuffer = stream.str();
        *data = (void *)bankBuffer.c_str();
        returnValue = bankBuffer.size();
    }
    return returnValue;
}

VstInt32 CsoundVST::setChunk(void* data, VstInt32 byteSize, bool isPreset)
{
    VstInt32 returnValue = 0;
    if(isPreset) {
        Preset preset;
        preset.text.resize(byteSize);
        for(int j = 0; j < byteSize; j++) {
            preset.text[j] = ((const char *)data)[j];
        }
        bank[curProgram] = preset;
        returnValue = byteSize;
    } else {
        std::string inputBuffer = (const char *)data;
        std::istringstream stream(inputBuffer);
        std::string buffer;
        stream >> buffer;
        stream >> std::ws;
        int n = atoi(buffer.c_str());
        bank.resize(n);
        for(int i = 0; i < n; i++) {
            Preset preset;
            stream >> preset.name;
            stream >> std::ws;
            stream >> buffer;
            stream >> std::ws;
            int length = atoi(buffer.c_str());
            preset.text.resize(length);
            char c;
            for(int j = 0; j < length; j++) {
                stream.get(c);
                preset.text[j] = c;
            }
            bank[i] = preset;
        }
        returnValue = byteSize;
    }
    setProgram(curProgram);
    ((CsoundVstFltk *)getEditor())->update();
    return returnValue;
}

std::string CsoundVST::getText()
{
    std::string buffer = getCSD();
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
    if (!getIsVst()) {
        status = Fl::run();
    } else {
        Message("Sorry, can't run FLTK if running as a VST plugin.\n");
    }
    return status;
}

VstIntPtr CsoundVST::dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
    if (CSOUNDVST_PRINT_OPCODES) {
        switch(opcode) {
        case 19:
        case 25:
            break;
        default:
            Message("dispatcher(%4d %-30s %6d 0x%p 0x%p %9.4f)\n", opcode, getNameForOpcode(opcode), index, value, ptr, opt);
        }
    }
    return AudioEffectX::dispatcher(opcode, index, value, ptr, opt);
}

extern "C"
{
    PUBLIC AEffect* VSTPluginMain(audioMasterCallback audioMaster)
    {
        if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0)) {
            return 0;
        }
        AudioEffect* effect = new CsoundVST(audioMaster);
        if (!effect) {
            return 0;
        }
        return effect->getAeffect();
    }
    SILENCE_PUBLIC CsoundVST* CreateCsoundVST()
    {
        CsoundVST *csoundVST = new CsoundVST;
        std::fprintf(stderr, "CreateCsoundVST: created 0x%p\n", csoundVST);
        return csoundVST;
    }
    SILENCE_PUBLIC void RunCsoundVST(const char *filename)
    {
        CsoundVST *csoundVST = new CsoundVST;
        AEffEditor *editor = csoundVST->getEditor();
        editor->open(0);
        if (filename) {
            csoundVST->openFile(filename);
        }
        csoundVST->fltkrun();
    }
}
