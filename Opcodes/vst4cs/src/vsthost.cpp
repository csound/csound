//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from his Vst Host program
//  and from the vst~ object by Thomas Grill,
//  which in turn borrows from the Psycle tracker.
//  VST is a trademark of Steinberg Media Technologies GmbH.
//  VST Plug-In Technology by Steinberg.
//
//  Copyright (C) 2004 Andres Cabrera, Michael Gogins
//
//  The vst4cs library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The vst4cs library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with The vst4cs library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
//  02111-1307 USA

#include "vsthost.h"
#include <cmath>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>
#ifdef WIN32
#pragma GCC diagnostic ignored "-fpermissive"
#endif

#ifdef MSVC
#pragma warning(disable:4786) //gab
#define round int
#endif

void VSTPlugin::initializeOpcodes()
{
    if (opcodeRefCount()) {
      opcodeRefCount()++;
      return;
    }
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effOpen, "effOpen"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effClose, "effClose"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetProgram, "effSetProgram"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetProgram, "effGetProgram"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetProgramName,
                                          "effSetProgramName"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetProgramName,
                                          "effGetProgramName"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetParamLabel,
                                          "effGetParamLabel"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetParamDisplay,
                                          "effGetParamDisplay"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetParamName,
                                          "effGetParamName"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetVu, "effGetVu"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetSampleRate,
                                          "effSetSampleRate"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetBlockSize,
                                          "effSetBlockSize"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effMainsChanged,
                                          "effMainsChanged"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditGetRect,
                                          "effEditGetRect"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditOpen, "effEditOpen"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditClose, "effEditClose"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditDraw, "effEditDraw"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditMouse, "effEditMouse"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditKey, "effEditKey"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditIdle, "effEditIdle"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditTop, "effEditTop"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditSleep, "effEditSleep"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effIdentify, "effIdentify"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetChunk, "effGetChunk"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetChunk, "effSetChunk"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effProcessEvents,
                                          "effProcessEvents"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effCanBeAutomated,
                                          "effCanBeAutomated"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effString2Parameter,
                                          "effString2Parameter"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetNumProgramCategories,
                                          "effGetNumProgramCategories"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetProgramNameIndexed,
                                          "effGetProgramNameIndexed"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effCopyProgram,
                                          "effCopyProgram"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effConnectInput,
                                          "effConnectInput"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effConnectOutput,
                                          "effConnectOutput"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetInputProperties,
                                          "effGetInputProperties"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetOutputProperties,
                                          "effGetOutputProperties"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetPlugCategory,
                                          "effGetPlugCategory"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetCurrentPosition,
                                          "effGetCurrentPosition"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetDestinationBuffer,
                                          "effGetDestinationBuffer"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effOfflineNotify,
                                          "effOfflineNotify"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effOfflinePrepare,
                                          "effOfflinePrepare"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effOfflineRun,
                                          "effOfflineRun"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effProcessVarIo,
                                          "effProcessVarIo"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetSpeakerArrangement,
                                          "effSetSpeakerArrangement"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetBlockSizeAndSampleRate,
                                          "effSetBlockSizeAndSampleRate"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetBypass,
                                          "effSetBypass"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetEffectName,
                                          "effGetEffectName"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetErrorText,
                                          "effGetErrorText"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetVendorString,
                                          "effGetVendorString"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetProductString,
                                          "effGetProductString"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetVendorVersion,
                                          "effGetVendorVersion"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effVendorSpecific,
                                          "effVendorSpecific"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effCanDo,
                                          "effCanDo"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetTailSize,
                                          "effGetTailSize"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effIdle,
                                          "effIdle"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetIcon,
                                          "effGetIcon"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetViewPosition,
                                          "effSetViewPosition"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetParameterProperties,
                                          "effGetParameterProperties"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effKeysRequired,
                                          "effKeysRequired"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetVstVersion,
                                          "effGetVstVersion"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditKeyDown,
                                          "effEditKeyDown"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEditKeyUp,
                                          "effEditKeyUp"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetEditKnobMode,
                                          "effSetEditKnobMode"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetMidiProgramName,
                                          "effGetMidiProgramName"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetCurrentMidiProgram,
                                          "effGetCurrentMidiProgram"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetMidiProgramCategory,
                                          "effGetMidiProgramCategory"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effHasMidiProgramsChanged,
                                          "effHasMidiProgramsChanged"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetMidiKeyName,
                                          "effGetMidiKeyName"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effBeginSetProgram,
                                          "effBeginSetProgram"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effEndSetProgram,
                                          "effEndSetProgram"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effGetSpeakerArrangement,
                                          "effGetSpeakerArrangement"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effShellGetNextPlugin,
                                          "effShellGetNextPlugin"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effStartProcess,
                                          "effStartProcess"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effStopProcess,
                                          "effStopProcess"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetTotalSampleToProcess,
                                          "effSetTotalSampleToProcess"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effSetPanLaw,
                                          "effSetPanLaw"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effBeginLoadBank,
                                          "effBeginLoadBank"));
    dispatchOpcodes().insert(std::pair<long,
                             std::string>((long) effBeginLoadProgram,
                                          "effBeginLoadProgram"));

    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterAutomate,
                                        "audioMasterAutomate"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterVersion,
                                        "audioMasterVersion"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterCurrentId,
                                        "audioMasterCurrentId"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterIdle,
                                        "audioMasterIdle"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterPinConnected,
                                        "audioMasterPinConnected"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterWantMidi,
                                        "audioMasterWantMidi"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetTime,
                                        "audioMasterGetTime"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterProcessEvents,
                                        "audioMasterProcessEvents"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterSetTime,
                                        "audioMasterSetTime"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterTempoAt,
                                        "audioMasterTempoAt"));
    masterOpcodes().insert(std::pair<long,
                           std::string>(
                                (long)audioMasterGetNumAutomatableParameters,
                                "audioMasterGetNumAutomatableParameters"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetParameterQuantization,
                                        "audioMasterGetParameterQuantization"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterIOChanged,
                                        "audioMasterIOChanged"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterNeedIdle,
                                        "audioMasterNeedIdle"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterSizeWindow,
                                        "audioMasterSizeWindow"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetSampleRate,
                                        "audioMasterGetSampleRate"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetBlockSize,
                                        "audioMasterGetBlockSize"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetInputLatency,
                                        "audioMasterGetInputLatency"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetOutputLatency,
                                        "audioMasterGetOutputLatency"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetPreviousPlug,
                                        "audioMasterGetPreviousPlug"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetNextPlug,
                                        "audioMasterGetNextPlug"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterWillReplaceOrAccumulate,
                                        "audioMasterWillReplaceOrAccumulate"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetCurrentProcessLevel,
                                        "audioMasterGetCurrentProcessLevel"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetAutomationState,
                                        "audioMasterGetAutomationState"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterOfflineStart,
                                        "audioMasterOfflineStart"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterOfflineRead,
                                        "audioMasterOfflineRead"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterOfflineWrite,
                                        "audioMasterOfflineWrite"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterOfflineGetCurrentPass,
                                        "audioMasterOfflineGetCurrentPass"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterOfflineGetCurrentMetaPass,
                                        "audioMasterOfflineGetCurrentMetaPass"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterSetOutputSampleRate,
                                        "audioMasterSetOutputSampleRate"));
    //     masterOpcodes().insert(std::pair<long,
    //std::string>(
    //         (long) audioMasterGetSpeakerArrangement,
    //
    //             "audioMasterGetSpeakerArrangement"));
    masterOpcodes().insert(std::pair<long,
                           std::string>(
                                (long) audioMasterGetOutputSpeakerArrangement,
                                "audioMasterGetOutputSpeakerArrangement"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetVendorString,
                                        "audioMasterGetVendorString"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetProductString,
                                        "audioMasterGetProductString"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetVendorVersion,
                                        "audioMasterGetVendorVersion"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterVendorSpecific,
                                        "audioMasterVendorSpecific"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterSetIcon,
                                        "audioMasterSetIcon"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterCanDo,
                                        "audioMasterCanDo"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetLanguage,
                                        "audioMasterGetLanguage"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterOpenWindow,
                                        "audioMasterOpenWindow"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterCloseWindow,
                                        "audioMasterCloseWindow"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetDirectory,
                                        "audioMasterGetDirectory"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterUpdateDisplay,
                                        "audioMasterUpdateDisplay"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterBeginEdit,
                                        "audioMasterBeginEdit"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterEndEdit,
                                        "audioMasterEndEdit"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterOpenFileSelector,
                                        "audioMasterOpenFileSelector"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterCloseFileSelector,
                                        "audioMasterCloseFileSelector"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterEditFile,
                                        "audioMasterEditFile"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long) audioMasterGetChunkFile,
                                        "audioMasterGetChunkFile"));
    masterOpcodes().insert(std::pair<long,
                           std::string>((long)audioMasterGetInputSpeakerArrangement,
                                        "audioMasterGetInputSpeakerArrangement"));

    opcodeRefCount()++;
}

VSTPlugin::VSTPlugin(CSOUND *csound_)
  : csound(csound_),
    libraryHandle(0),
    aeffect(0),
    hasEditor(false),
    editor(0),
    window(0),
    windowHandle(0),
    pluginIsSynth(true),
    overwrite(false),
    edited(false),
    showParameters(false),
    framesPerSecond(0),
    framesPerBlock(0)
{
    memset(&vstTimeInfo, 0, sizeof(VstTimeInfo));
    initializeOpcodes();
}

VSTPlugin::~VSTPlugin()
{
    Free();
    opcodeRefCount()--;
}

bool VSTPlugin::AddMIDI(int data, int deltaFrames, int detune)
{
    if (!aeffect)
      return false;
    vstMidiEvents.resize(vstMidiEvents.size() + 1);
    VstMidiEvent &vstMidiEvent = vstMidiEvents.back();
    vstMidiEvent.type = kVstMidiType;
    vstMidiEvent.byteSize = 24;
    vstMidiEvent.deltaFrames = deltaFrames;
    vstMidiEvent.flags = 0;
    vstMidiEvent.detune = detune;
    vstMidiEvent.noteLength = 0;
    vstMidiEvent.noteOffset = 0;
    vstMidiEvent.reserved1 = 0;
    vstMidiEvent.reserved2 = 0;
    vstMidiEvent.noteOffVelocity = 0;
    vstMidiEvent.midiData[0] = data & 255;
    if ((data & 240) == 144) {
      if (!(data & (int) 0x7F0000)) {
        vstMidiEvent.midiData[0] -= 16;
        data |= (int) 0x400000;
      }
    }
    vstMidiEvent.midiData[1] = (data >> 8) & 127;
    vstMidiEvent.midiData[2] = (data >> 16) & 127;
    vstMidiEvent.midiData[3] = 0;
    return true;
}

void VSTPlugin::SendMidi()
{
    if (aeffect && vstMidiEvents.size() > 0) {
      vstEventsBuffer.resize(sizeof(VstEvents) +
                             (sizeof(VstEvent *) * vstMidiEvents.size()));
      VstEvents *vstEvents = (VstEvents *) &vstEventsBuffer.front();

      vstEvents->numEvents = vstMidiEvents.size();
      vstEvents->reserved = 0;
      for (size_t i = 0, n = vstEvents->numEvents; i < n; i++) {
        vstEvents->events[i] = (VstEvent *) &vstMidiEvents[i];
        Debug("VSTPlugin::SendMidi(queue size %d status %d data1 %d "
              "data2 %d detune %d delta %d\n",
              vstEvents->numEvents,
              ((VstMidiEvent *)vstEvents->events[i])->midiData[0],
              ((VstMidiEvent *)vstEvents->events[i])->midiData[1],
              ((VstMidiEvent *)vstEvents->events[i])->midiData[2],
              ((VstMidiEvent *)vstEvents->events[i])->detune,
              ((VstMidiEvent *)vstEvents->events[i])->deltaFrames);
      }
      Dispatch(effProcessEvents, 0, 0, vstEvents, 0.0f);
      vstMidiEvents.resize(0);
    }
}

bool VSTPlugin::DescribeValue(int parameter, char *value)
{
    Debug("VSTPlugin::DescribeValue.\n");
    if (aeffect) {
      if (parameter < aeffect->numParams) {
        char par_display[0x100];
        char par_label[0x100];
        Dispatch(effGetParamDisplay,parameter,0,par_display,0.0f);
        Dispatch(effGetParamLabel,parameter,0,par_label,0.0f);
        strcpy(value, par_display);
        return true;
      }
    }
    return false;
}

int VSTPlugin::Instantiate(const char *libraryName_)
{
    Debug("VSTPlugin::Instance...\n");
#ifdef __MACH__
    CFStringRef vstBundlePath =
      CFStringCreateWithCString(kCFAllocatorDefault,
                                libraryName_, kCFStringEncodingMacRoman );
    CFURLRef vstBundleURL =
      CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                    vstBundlePath,
                                    kCFURLPOSIXPathStyle,
                                    true);
    CFBundleRef vstBundle = CFBundleCreate(kCFAllocatorDefault, vstBundleURL);

    CFRelease(vstBundlePath);
    CFRelease(vstBundleURL);

    if (vstBundle == NULL)
#else
      if (csound->OpenLibrary(&libraryHandle, libraryName_) != 0)
#endif
        {
          Log("WARNING! '%s' was not found or is invalid.\n", libraryName_);
          return VSTINSTANCE_ERR_NO_VALID_FILE;
        }
    Debug("Loaded plugin library '%s'.\n", libraryName_);

#ifdef __MACH__
    short bundleRes = CFBundleOpenBundleResourceMap(vstBundle);
    /* For VST SDK 2.4 and later. */
    PVSTMAIN main =
      (PVSTMAIN) CFBundleGetFunctionPointerForName(vstBundle,
                                                   CFSTR("VSTPluginMain"));
    /* For VST SDK 2.3 and earlier. */
    if (!main) {
      main = CFBundleGetFunctionPointerForName(vstBundle, CFSTR("main_macho"));
    }
#else
    /* For VST SDK 2.4 and later. */
    PVSTMAIN main = (PVSTMAIN) csound->GetLibrarySymbol(libraryHandle,
                                                        "VSTPluginMain");
    if (!main) {
      /* For VST SDK 2.3 and earlier. */
      main = (PVSTMAIN) csound->GetLibrarySymbol(libraryHandle, "main");
    }
#endif
    if (!main) {
      Log("Failed to find 'main' function.\n");
      csound->CloseLibrary(libraryHandle);
      aeffect = NULL;
      return VSTINSTANCE_ERR_NO_VST_PLUGIN;
    }
    Debug("Found 'main' function at 0x%x.\n", main);
    aeffect = main((audioMasterCallback) VSTPlugin::Master);
    if (!aeffect) {
      Log("VST plugin: unable to create effect.\n");
      csound->CloseLibrary(libraryHandle);
      return VSTINSTANCE_ERR_REJECTED;
    }
    aeffect->user = this;
    Debug("Created effect '%x'.\n", aeffect);
    if (aeffect->magic != kEffectMagic) {
      Log("VST plugin: Instance query rejected by 0x%x\n", aeffect);
      csound->CloseLibrary(libraryHandle);
      aeffect = NULL;
      return VSTINSTANCE_ERR_REJECTED;
    }
    if (!Dispatch(effGetProductString, 0, 0, &productName, 0.0f)) {
      strcpy(productName, libraryName);
    }
    return VSTINSTANCE_NO_ERROR;
}

void VSTPlugin::Info()
{
    int i =0;
    Log("====================================================\n");
    Log("Loaded plugin: %s\n", productName);
    if (!aeffect->dispatcher(aeffect,
                             effGetVendorString, 0, 0, &vendorName, 0.0f)) {
      strcpy(vendorName, "Unknown vendor");
    }
    Log("Vendor name: %s \n", vendorName);
    pluginVersion = aeffect->version;
    Log("Version: %d \n", pluginVersion);
    pluginIsSynth = (aeffect->flags & effFlagsIsSynth)?true:false;
    Log("Is synthesizer? %s\n",(pluginIsSynth==true?"Yes":"No"));
    overwrite = (aeffect->flags & effFlagsCanReplacing)?true:false;
    hasEditor = (aeffect->flags & effFlagsHasEditor)?true:false;
    Log("Number of inputs: %i\n", getNumInputs());
    Log("Number of outputs: %i\n", getNumOutputs());
    long nparams=NumParameters();
    Log("Number of parameters: %d\n",nparams);
    char buffer[256];
    for (i = 0; i < nparams; i++) {
      strcpy(buffer, "No parameter");
      GetParamName(i, buffer);
      if (strcmp("unused",buffer)==1)
        Log("  Parameter%5i: %s\n",i, buffer);
    }
    csound->Message(csound, "Number of programs: %d\n",
                    (int) aeffect->numPrograms);
    for (i = 0; i < aeffect->numPrograms; i++) {
      strcpy(buffer, "No program");
      GetProgramName(0, i, buffer);
      if (strcmp("default",buffer)==1)
        Log("  Program%7i: %s\n", i, buffer);
    }
    csound->Message(csound,
                    "----------------------------------------------------\n");
}

int VSTPlugin::getNumInputs(void)
{
    Debug("VSTPlugin::getNumInputs.\n");
    return aeffect->numInputs;
}

int VSTPlugin::getNumOutputs(void)
{
    Debug("VSTPlugin::getNumOutputs.\n");
    return aeffect->numOutputs;
}

void VSTPlugin::Free() // Called also in destruction
{
    Debug("VSTPlugin::Free.\n");
    CloseEditor();
    if (aeffect) {
      Dispatch(effMainsChanged, 0, 0, NULL, 0.0f);
      Dispatch(effClose,        0, 0, NULL, 0.0f);
      aeffect->user = 0;
      aeffect = 0;
#ifndef __MACH__
      csound->CloseLibrary(libraryHandle);
#endif
      libraryHandle = 0;
    }
}

void VSTPlugin::Init()
{
    size_t i;
    Debug("VSTPlugin::Init.\n");
    framesPerSecond = (size_t) ((long) (csound->GetSr(csound) + FL(0.5)));
    framesPerBlock = csound->GetKsmps(csound);
    Log("VSTPlugin::Init framesPerSecond %d framesPerBlock %d "
        "channels %d in / %d out.\n",
        framesPerSecond, framesPerBlock, getNumInputs(), getNumOutputs());
    inputs_.resize((size_t) getNumInputs());
    outputs_.resize((size_t) getNumOutputs());
    for (i = 0; i < inputs_.size(); i++) {
      inputs_[i].resize(framesPerBlock);
      inputs.push_back(&inputs_[i].front());
    }
    for (i = 0; i < outputs_.size(); i++) {
      outputs_[i].resize(framesPerBlock);
      outputs.push_back(&outputs_[i].front());
    }
    Dispatch(effOpen        ,  0, 0, NULL, 0.0f);
    Dispatch(effSetProgram  ,  0, 0, NULL, 0.0f);
    Dispatch(effMainsChanged,  0, 1, NULL, 0.0f);
    Dispatch(effSetSampleRate, 0, 0, 0, (float) framesPerSecond );
    Dispatch(effSetBlockSize,  0, framesPerBlock, NULL, 0.0f );
}

bool VSTPlugin::SetParameter(int parameter, float value)
{
    Debug("VSTPlugin::SetParameter(%d, %f).\n", parameter, value);
    if (aeffect) {
      if ((parameter >= 0) && (parameter <= aeffect->numParams)) {
        aeffect->setParameter(aeffect, parameter, value);
        return true;
      }
      else return false;
    }
    return false;
}

bool VSTPlugin::SetParameter(int parameter, int value)
{
    Debug("VSTPlugin::SetParameter(%d, %d).\n", parameter, value);
    return SetParameter(parameter,value/65535.0f);
}

int VSTPlugin::GetCurrentProgram()
{
    Debug("VSTPlugin::GetCurrentProgram.\n");
    if (aeffect)
      return Dispatch(effGetProgram, 0, 0, NULL, 0.0f);
    else
      return 0;
}

void VSTPlugin::SetCurrentProgram(int prg)
{
    Debug("VSTPlugin::SetCurrentProgram((%d).\n", prg);
    if (aeffect)
      Dispatch(effSetProgram, 0, prg, NULL, 0.0f);
}

bool VSTPlugin::replace()
{
    return overwrite;
}

void VSTPlugin::EditorIdle()
{
    Dispatch(effEditIdle, 0, 0, windowHandle, 0.0f);
}

ERect VSTPlugin::GetEditorRect()
{
    ERect *rect_ = 0;
    Dispatch(effEditGetRect,0,0, &rect_,0.0f);
    if (rect_)
      rect = *rect_;
    else {
      rect.right = 600;
      rect.bottom = 400;
    }
    return rect;
}

void VSTPlugin::OpenEditor()
{
    if (windowHandle)
      return;
    if (aeffect->flags & effFlagsHasEditor == 1) {
      GetEditorRect();
      Debug("ERect top %d left %d right %d bottom %d.\n", rect.top, rect.left,
            rect.right, rect.bottom);
      window = new Fl_Window(rect.right, rect.bottom, GetName());
      Debug("Window 0x%x.\n", window);
      window->show();
#ifdef WIN32
      windowHandle = fl_xid(window);
      Debug("windowHandle 0x%x.\n", windowHandle);
      SetEditWindow(windowHandle);
#elif defined (LINUX)
      //TODO: Fill appropriate code here
      //  windowHandle = (void *) fl_find(fl_xid(ST(fl_windows)[panelNum].panel));
      Log("vstedit not implemented on Linux yet.\n");
#elif defined (MACOSX)
      //TODO: Fill appropriate code here
      Log("vstedit not implemented on Mac yet.\n");
#endif
    }
    else
      Log("Plugin:%s has no GUI.\n", productName);
}

void VSTPlugin::CloseEditor()
{
    if (!windowHandle)
      return;
    OnEditorClose();
    if (aeffect->flags & effFlagsHasEditor == 1) {
      OnEditorClose();
      //window->hide();
      //delete window;
      //window = 0;
      windowHandle = 0;
    }
}

void VSTPlugin::SetEditWindow(void *h)
{
    windowHandle = h;
    Dispatch(effEditOpen,0,0, windowHandle,0.0f);
}

void VSTPlugin::OnEditorClose()
{
    Dispatch(effEditClose,0,0, windowHandle,0.0f);
}

void VSTPlugin::SetShowParameters(bool s)
{
    showParameters = s;
}

bool VSTPlugin::ShowParams()
{
    return showParameters;
}

bool VSTPlugin::GetProgramName(int cat, int parameter, char *buf)
{
    if (aeffect) {
      if (parameter < NumPrograms()) {
        Dispatch(effGetProgramNameIndexed, parameter, cat, buf, 0.0f);
        return true;
      }
    }
    return false;
}

int VSTPlugin::GetNumCategories()
{
    if (aeffect)
      return Dispatch(effGetNumProgramCategories, 0, 0, NULL, 0.0f);
    else
      return 0;
}

void VSTPlugin::StopEditing()
{
    edited = false;
}

void VSTPlugin::Log(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (csound) {
      csound->MessageV(csound, 0, format, args);
    }
    else {
      vfprintf(stdout, format, args);
    }
    va_end(args);
}

void VSTPlugin::Debug(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (csound) {
      if ((csound->GetMessageLevel(csound) & 7) == 7 ||
          csound->GetDebug(csound)) {
        csound->MessageV(csound, 0, format, args);
      }
    }
    else {
      vfprintf(stdout, format, args);
    }
    va_end(args);
}

long VSTPlugin::OnGetVersion(AEffect *effect)
{
#if defined(VST_2_3_EXTENSIONS)
    return 2300L;
#elif defined(VST_2_2_EXTENSIONS)
    return 2200L;
#elif defined(VST_2_1_EXTENSIONS)
    return 2100L;
#else
    return 2L;
#endif
}

int VSTPlugin::GetNumParams(void)
{
    return aeffect->numParams;
}

void VSTPlugin::GetParamName(int numparam, char *name)
{
    if (numparam < aeffect->numParams)
      Dispatch(effGetParamName, numparam, 0, name, 0.0f);
    else
      strcpy(name, "Parameter out of range.");
}

float VSTPlugin::GetParamValue(int numparam)
{
    if (numparam < aeffect->numParams)
      return (aeffect->getParameter(aeffect, numparam));
    else
      return -1.0;
}

char* VSTPlugin::GetName(void)
{
    return productName;
}

unsigned long VSTPlugin::GetVersion(void)
{
    return pluginVersion;
}

char* VSTPlugin::GetVendorName(void)
{
    return vendorName;
}

char* VSTPlugin::GetDllName(void)
{
    return libraryName;
}

long VSTPlugin::NumParameters(void)
{
    return aeffect->numParams;
}

float VSTPlugin::GetParameter(long parameter)
{
    return aeffect->getParameter(aeffect, parameter);
}

int VSTPlugin::NumPrograms(void)
{
    return aeffect->numPrograms;
}

VstTimeInfo *VSTPlugin::GetTime(void)
{
    Debug("VSGPlugin::GetTime().\n");
    if (csound)
      vstTimeInfo.samplePos = csound->GetCurrentTimeSamples(csound);
    else
      vstTimeInfo.samplePos = 0;
    vstTimeInfo.sampleRate = framesPerSecond;
    Debug("&vstTimeInfo 0x%x sampleRate %f samplePos %f.\n",
          &vstTimeInfo, vstTimeInfo.sampleRate, vstTimeInfo.samplePos);
    return &vstTimeInfo;
}

bool VSTPlugin::IsSynth()
{
    return pluginIsSynth;
}

bool VSTPlugin::OnCanDo(const char *ptr)
{
    // printf("Can do call: %s.\n", ptr);
    if ((!strcmp(ptr, "sendVstMidiEvent")) ||
        (!strcmp(ptr, "receiveVstMidiEvent")) ||
        (!strcmp(ptr, "sendVstEvents")) ||
        (!strcmp(ptr, "receiveVstEvents")) ||
        (!strcmp(ptr, "sendVstTimeInfo")) /*||
                                            (!strcmp(ptr, "sizeWindow"))*/)
      return true;
    return false;
}

bool VSTPlugin::OnInputConnected(AEffect *effect, long input)
{
    return true;
}

bool VSTPlugin::OnOutputConnected(AEffect *effect, long output)
{
    return true;
}

VstIntPtr VSTPlugin::Master(AEffect *effect,
                       VstInt32 opcode,
                       VstInt32 index,
                       VstIntPtr value,
                       void *ptr,
                       float opt)
{
    VSTPlugin *plugin = 0;
    CSOUND *csound = 0;
    if (effect) {
      plugin = (VSTPlugin *) effect->user;
      if (plugin) {
        csound = plugin->csound;
      }
    }
    std::string opcodeName;
    if (masterOpcodes().find(opcode) != masterOpcodes().end()) {
      opcodeName = masterOpcodes()[opcode];
    }
    // These messages are to tell Csound what the plugin wants it to do.
    if (plugin) {
      plugin->Debug("VSTPlugin::Master(AEffect 0x%p, opcode %ld %s, index %ld, "
                    "value %dl, ptr 0x%p, opt %f)\n",
                    effect, opcode, opcodeName.c_str(), index, value, ptr, opt);
    }
//   else {
//     fprintf(stdout, "VSTPlugin::Master(AEffect 0x%p, opcode %ld %s, index %ld, "
//          "value %ld, ptr 0x%p, opt %f)\n",
//          effect, opcode, opcodeName.c_str(), index, value, ptr, opt);
//   }
    switch (opcode) {
    case audioMasterAutomate:
      return true;
    case audioMasterVersion:
      if (effect)
        return OnGetVersion(effect);
    case audioMasterCurrentId:
      if (effect)
        return effect->uniqueID;
      else
        return -1;
    case audioMasterIdle:
      if (plugin) {
        plugin->Dispatch(effEditIdle, 0, 0, NULL, 0.0f);
      }
      return 0;
    case audioMasterPinConnected:
      if (effect)
        {
          return !((value) ?  OnOutputConnected(effect, index) :
                   OnInputConnected(effect, index));
        }
    case audioMasterWantMidi:
      return false;
    case audioMasterProcessEvents:
      return false;
    case audioMasterGetTime:
      if (plugin) {
        return (VstIntPtr) plugin->GetTime();
      }
      else {
        //static VstTimeInfo vstTimeInfo;
        //memset(&vstTimeInfo, 0, sizeof(VstTimeInfo));
        //return (long) &vstTimeInfo;
      }
      return 0;
    case audioMasterTempoAt:
      return 0;
    case audioMasterNeedIdle:
      if (plugin)
        return plugin->Dispatch(effIdle, 0, 0, NULL, 0.0f);
      return false;
    case audioMasterGetSampleRate:
      if (plugin)
        return plugin->framesPerSecond;
      else
        return 44100;
    case audioMasterGetVendorString:
      strcpy((char *)ptr, "vst4cs");
      return 0;
    case audioMasterGetVendorVersion:
      return 5000;
    case audioMasterGetProductString:
      strcpy((char*)ptr,"vst4cs");
      return 0;
    case audioMasterGetLanguage:
      return kVstLangEnglish;
    case audioMasterUpdateDisplay:
      if (plugin) {
        plugin->Dispatch(effEditIdle, 0, 0, NULL, 0.0f);
        //return 0;
      }
      else
        return 1;
    case audioMasterGetNextPlug:
      return 1;
    case audioMasterWillReplaceOrAccumulate:
      return 1;
    case audioMasterGetCurrentProcessLevel:
      return 0;
    case audioMasterCanDo:
      //if(plugin)
      return OnCanDo((char *)ptr);
      //else printf("No instance");
    default:
      break;
    }
    return 0;
}
