//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from the vst~ object, 
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

#include "AEffEditor.hpp"
#include "aeffectx.h"
#include "vsthost.h"
#include <cmath>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>

VSTPlugin::VSTPlugin(ENVIRON *csound_) : 
    window(0),
    windowHandle(0), 
    csound(csound_), 
    aeffect(0), 
    editor(0),
    hasEditor(false),
    libraryHandle(0), 
    overwrite(false),
    showParameters(false),
    midiChannel(0),
    edited(false),
    framesPerSecond(0),
    framesPerBlock(0)
{
    memset(&vstTimeInfo, 0, sizeof(VstTimeInfo));
}

VSTPlugin::~VSTPlugin()
{
	Free();			
}

bool VSTPlugin::AddMIDI(int data0, int data1, int data2)
{
	if(aeffect) {
	    vstMidiEvents.resize(vstMidiEvents.size() + 1);
		VstMidiEvent &vstMidiEvent = vstMidiEvents.back();
		vstMidiEvent.type = kVstMidiType;
		vstMidiEvent.byteSize = 24;
		vstMidiEvent.deltaFrames = 0;
		vstMidiEvent.flags = 0;
		vstMidiEvent.detune = 0;
		vstMidiEvent.noteLength = 0;
		vstMidiEvent.noteOffset = 0;
		vstMidiEvent.reserved1 = 0;
		vstMidiEvent.reserved2 = 0;
		vstMidiEvent.noteOffVelocity = 0;
		vstMidiEvent.midiData[0] = data0;
		vstMidiEvent.midiData[1] = data1;
		vstMidiEvent.midiData[2] = data2;
		vstMidiEvent.midiData[3] = 0;
		return true;
	}
	else 
        return false;
}

bool VSTPlugin::AddNoteOn(int midichannel, MYFLT note, MYFLT speed)
{
	if(aeffect) {
	    MYFLT rounded = round(note);
	    MYFLT cents = (note - rounded) * FL(100.0);
	    vstMidiEvents.resize(vstMidiEvents.size() + 1);
		VstMidiEvent &vstMidiEvent = vstMidiEvents.back();
		vstMidiEvent.type = kVstMidiType;
		vstMidiEvent.byteSize = 24;
		vstMidiEvent.deltaFrames = 0;
		vstMidiEvent.flags = 0;
		vstMidiEvent.detune = cents;
		vstMidiEvent.noteLength = 0;
		vstMidiEvent.noteOffset = 0;
		vstMidiEvent.reserved1 = 0;
		vstMidiEvent.reserved2 = 0;
		vstMidiEvent.noteOffVelocity = 0;
		vstMidiEvent.midiData[0] = 144 | midichannel;
		vstMidiEvent.midiData[1] = rounded;
		vstMidiEvent.midiData[2] = speed;
		vstMidiEvent.midiData[3] = 0;
		return true;
	}
	else 
        return false;
}

bool VSTPlugin::AddNoteOff(int midichannel, MYFLT note)
{
	if(aeffect) {
	    MYFLT rounded = round(note);
	    vstMidiEvents.resize(vstMidiEvents.size() + 1);
		VstMidiEvent &vstMidiEvent = vstMidiEvents.back();
		vstMidiEvent.type = kVstMidiType;
		vstMidiEvent.byteSize = 24;
		vstMidiEvent.deltaFrames = 0;
		vstMidiEvent.flags = 0;
		vstMidiEvent.detune = 0;
		vstMidiEvent.noteLength = 0;
		vstMidiEvent.noteOffset = 0;
		vstMidiEvent.reserved1 = 0;
		vstMidiEvent.reserved2 = 0;
		vstMidiEvent.noteOffVelocity = 0;
		vstMidiEvent.midiData[0] = 128 | midichannel;
		vstMidiEvent.midiData[1] = rounded;
		vstMidiEvent.midiData[2] = 0;
		vstMidiEvent.midiData[3] = 0;
		return true;
	}
	else 
        return false;
}

void VSTPlugin::SendMidi()
{
	if(aeffect && vstMidiEvents.size() > 0) {
	    vstEventsBuffer.resize(sizeof(VstEvents) + (sizeof(VstEvent *) * vstMidiEvents.size()));
	    VstEvents *vstEvents = (VstEvents *)&vstEventsBuffer.front();
	    vstEvents->numEvents = vstMidiEvents.size();
	    vstEvents->reserved = 0;
		for(size_t i = 0, n = vstEvents->numEvents; i < n; i++) {
            vstEvents->events[i] = (VstEvent *)&vstMidiEvents[i];	
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

bool VSTPlugin::DescribeValue(int parameter,char* value)
{
    Log("VSTPlugin::DescribeValue.\n");
	if(aeffect)
	{
		if(parameter<aeffect->numParams)
		{
			char par_display[64];
			char par_label[64];
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
    Log("VSTPlugin::Instance.\n");
 	libraryHandle = csound->OpenLibrary(libraryName_);
	if(!libraryHandle)	
	{
		Log("WARNING! '%s' was not found or is invalid.\n", libraryName_);
		return VSTINSTANCE_ERR_NO_VALID_FILE;
	}
	Log("Loaded plugin library '%s'.\n" , libraryName_);
	PVSTMAIN main = (PVSTMAIN)csound->GetLibrarySymbol(libraryHandle,"main");
	if(!main)
	{	
	    Log("Failed to find 'main' function.\n");
     	csound->CloseLibrary(libraryHandle);
		aeffect=NULL;
		return VSTINSTANCE_ERR_NO_VST_PLUGIN;
	}
	Log("Found 'main' function at 0x%x.\n" , main);
	aeffect = main((audioMasterCallback) VSTPlugin::Master);
	aeffect->user = this;
	if(!aeffect)
	{
		Log("VST plugin: unable to create effect.\n");
		csound->CloseLibrary(libraryHandle);
		return VSTINSTANCE_ERR_REJECTED;
	}
	Log("Created effect '%x'.\n" , aeffect);	
	if(  aeffect->magic!=kEffectMagic)
	{
		Log("VST plugin: Instance query rejected by 0x%x\n", aeffect);
		csound->CloseLibrary(libraryHandle);
		aeffect=NULL;
		return VSTINSTANCE_ERR_REJECTED;
	}
	return VSTINSTANCE_NO_ERROR;
}

void VSTPlugin::Info()
{
	int i =0;
	csound->Message(csound,"====================================================\n");
	if(!Dispatch( effGetProductString, 0, 0, &productName, 0.0f)) {
		strcpy(productName, libraryName);
	}
	csound->Message(csound,"Loaded plugin: %s\n", productName);
	if(!aeffect->dispatcher(aeffect, effGetVendorString, 0, 0, &vendorName, 0.0f)) {
		strcpy(vendorName, "Unknown vendor");
	}
	csound->Message(csound,"Vendor name: %s \n", vendorName);
	pluginVersion = aeffect->version;
	csound->Message(csound,"Version: %d \n", pluginVersion);
	pluginIsSynth = (aeffect->flags & effFlagsIsSynth)?true:false;
	csound->Message(csound,"Is synthesizer? %s\n",(pluginIsSynth==true?"Yes":"No"));
	overwrite = (aeffect->flags & effFlagsCanReplacing)?true:false;
	hasEditor = (aeffect->flags & effFlagsHasEditor)?true:false;
	csound->Message(csound,"Number of inputs: %i\n", getNumInputs());
	csound->Message(csound,"Number of outputs: %i\n", getNumOutputs());
	long nparams=NumParameters();
	csound->Message(csound,"Number of parameters: %d\n",nparams);
    char buffer[256];
	for(i=0; i<nparams;i++) {
	    strcpy(buffer, "No parameter");
		GetParamName(i, buffer);
		csound->Message(csound,"  Parameter%5i: %s\n",i, buffer);		
	}
	csound->Message(csound,"Number of programs: %d\n",aeffect->numPrograms);
	for(i = 0; i < aeffect->numPrograms; i++) {
        strcpy(buffer, "No program");
	    GetProgramName(0, i, buffer);
	    Log("  Program%7i: %s\n", i, buffer);
    }	
	csound->Message(csound,"----------------------------------------------------\n");
}

int VSTPlugin::getNumInputs( void )
{
    Log("VSTPlugin::getNumInputs.\n");
	return aeffect->numInputs;
}

int VSTPlugin::getNumOutputs( void )
{
    Log("VSTPlugin::getNumOutputs.\n");
	return aeffect->numOutputs;
}

void VSTPlugin::Free() // Called also in destruction
{
    Log("VSTPlugin::Free.\n");
	if(aeffect) {
		Dispatch(effMainsChanged, 0, 0, NULL, 0.0f);
		Dispatch(effClose,        0, 0, NULL, 0.0f);
		aeffect = 0;
		aeffect->user = 0;
		csound->CloseLibrary(libraryHandle);
		libraryHandle = 0;
	}
}

void VSTPlugin::Init()
{
    Log("VSTPlugin::Init.\n");
	framesPerSecond = csound->GetSr(csound);
	framesPerBlock = csound->GetKsmps(csound);
	channels = csound->GetNchnls(csound);
	inputs_.resize(channels);
	outputs_.resize(channels);
	for(size_t i = 0; i < channels; i++) {
        inputs_[i].resize(framesPerBlock);
        inputs.push_back(&inputs_[i].front());
        outputs_[i].resize(framesPerBlock);
        outputs.push_back(&outputs_[i].front());
    }      
	Dispatch(effOpen        ,  0, 0, NULL, 0.f);
	Dispatch(effSetProgram  ,  0, 0, NULL, 0.0f);
	Dispatch(effMainsChanged,  0, 1, NULL, 0.f);
	Dispatch(effSetSampleRate, 0, 0, 0, (float) framesPerSecond );
	Dispatch(effSetBlockSize,  0, framesPerBlock, NULL, 0.f );
}

bool VSTPlugin::SetParameter(int parameter, float value)
{
    Log("VSTPlugin::SetParameter(%d, %f).\n", parameter, value);
	if(aeffect) {
		if(( parameter >= 0 ) && (parameter<=aeffect->numParams)) {
			aeffect->setParameter(aeffect,parameter,value);
			return true;
		}
		else return false;
	}
	return false;
}

bool VSTPlugin::SetParameter(int parameter, int value)
{
    Log("VSTPlugin::SetParameter(%d, %d).\n", parameter, value);
	return SetParameter(parameter,value/65535.0f);
}

int VSTPlugin::GetCurrentProgram()
{
    Log("VSTPlugin::GetCurrentProgram.\n");
	if(aeffect)
		return Dispatch(effGetProgram,0,0,NULL,0.0f);
	else
		return 0;
}

void VSTPlugin::SetCurrentProgram(int prg)
{
    Log("VSTPlugin::SetCurrentProgram((%d).\n", prg);
	if(aeffect)
		Dispatch(effSetProgram,0,prg,NULL,0.0f);
}

bool VSTPlugin::replace()
{
	return overwrite;
}

void VSTPlugin::EditorIdle()
{
	Dispatch(effEditIdle,0,0, windowHandle,0.0f);			
}


ERect VSTPlugin::GetEditorRect()
{
    ERect *rect_ = 0;
	Dispatch(effEditGetRect,0,0, &rect_,0.0f);	
	rect = *rect_;
	return rect;
}

void VSTPlugin::OpenEditor()
{
    GetEditorRect();
    Debug("ERect top %d left %d right %d bottom %d.\n", rect.top, rect.left, 
        rect.right, rect.bottom);
    window = new Fl_Window(rect.right, rect.bottom, GetName());
    Debug("Window 0x%x.\n", window);
    window->show();						
	windowHandle = fl_xid(window);
    Debug("windowHandle 0x%x.\n", windowHandle);
	SetEditWindow(windowHandle);
}

void VSTPlugin::CloseEditor()
{
    OnEditorClose();
	window->hide();
	delete window;
	window = 0;
	windowHandle = 0;
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

void VSTPlugin::AddAftertouch(int value)
{
	if(value < 0) value = 0; else if(value > 127) value = 127;
 	AddMIDI((char)MIDI_NOTEOFF | midiChannel , value );
}

void VSTPlugin::AddPitchBend(int value)
{
    AddMIDI( MIDI_PITCHBEND + (midiChannel & 0xf) , ((value>>7) & 127), (value & 127));
}

void VSTPlugin::AddProgramChange(int value)
{
    if(value < 0) value = 0; else if(value > 127) value = 127;
    AddMIDI( MIDI_PROGRAMCHANGE + (midiChannel & 0xf), value, 0);
}

void VSTPlugin::AddControlChange(int control, int value)
{
    if(control < 0) control = 0;  else if(control > 127) control = 127;
    if(value < 0) value = 0;  else if(value > 127) value = 127;
    AddMIDI( MIDI_CONTROLCHANGE + (midiChannel & 0xf), control, value);
}

bool VSTPlugin::GetProgramName( int cat , int parameter, char *buf)
{
	if(aeffect) {
		if(parameter<NumPrograms()) {
			Dispatch(effGetProgramNameIndexed,parameter,cat,buf,0.0f);
			return true;
		}
	}
	return false;
}

int VSTPlugin::GetNumCategories()
{
	if(aeffect)
		return Dispatch(effGetNumProgramCategories,0,0,NULL,0.0f);
	else
		return 0;
}

void VSTPlugin::StopEditing()
{
	edited = false;
}

void VSTPlugin::Log(const char *format,...)
{
      va_list args;
      va_start(args, format);
      if(csound) {
            csound->MessageV(csound, format, args);
      }
      else {
            vfprintf(stdout, format, args);
      }
      va_end(args);
}

void VSTPlugin::Debug(const char *format,...)
{
      va_list args;
      va_start(args, format);
      if(csound) {
          if(csound->GetMessageLevel(csound) & WARNMSG ||
             csound->GetDebug(csound)) {
              csound->MessageV(csound, format, args);
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
	
void VSTPlugin::GetParamName(int numparam,char* name) 
{
    if(numparam < aeffect->numParams) 
        Dispatch(effGetParamName,numparam,0,name,0.0f);
    else 
        strcpy(name,"Parameter out of range.");
}

float VSTPlugin::GetParamValue(int numparam) 
{
    if(numparam < aeffect->numParams) 
        return(aeffect->getParameter(aeffect, numparam));
    else 
        return -1.0;
}

char* VSTPlugin::GetName(void) 
{ 
    return productName; 
}

unsigned long VSTPlugin::GetVersion() 
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

int VSTPlugin::NumPrograms() 
{ 
    return aeffect->numPrograms; 
}

bool VSTPlugin::IsSynth() 
{ 
    return pluginIsSynth; 
}

bool VSTPlugin::OnCanDo(const char *ptr) 
{
    if((!strcmp(ptr, "sendVstMidiEvent")) ||
       (!strcmp(ptr, "receiveVstMidiEvent")) /*||
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

long VSTPlugin::Master(AEffect *effect, long opcode, long index, 
    long value, void *ptr, float opt)
{
    // These messages are to tell Csound what the plugin wants it to do.
    fprintf(stdout, "VSTPlugin::Master(AEffect 0x%x, opcode %d, index %d, "
        "value %d, ptr 0x%x, opt %f)\n",
        effect, opcode, index, value, ptr, opt);
    VSTPlugin *plugin = 0;
    ENVIRON *csound = 0;
    if(effect) {
        plugin = (VSTPlugin *)effect->user;
        if(plugin) {
            csound = plugin->csound;
        }
    }
	switch(opcode)
	{
	case audioMasterAutomate:			
		return true;		
	case audioMasterVersion:			
		return OnGetVersion(effect);		
	case audioMasterCurrentId:			
		if(effect)
		    return effect->uniqueID; else return -1;	
	case audioMasterIdle:
		effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;		
	case audioMasterPinConnected:	
        return !((value) ?  OnOutputConnected(effect, index) :
                            OnInputConnected(effect, index));	
	case audioMasterWantMidi:			
		return false;
	case audioMasterProcessEvents:		
		return false; 	
	case audioMasterGetTime:
	    if(plugin) {
            plugin->vstTimeInfo.samplePos = csound->kcounter_ * csound->ksmps_;
	        plugin->vstTimeInfo.sampleRate = plugin->framesPerSecond;
	        return (long)&plugin->vstTimeInfo;
        }
        else
            return 0;
	case audioMasterTempoAt:			
		return 0;
	case audioMasterNeedIdle:	
		effect->dispatcher(effect, effIdle, 0, 0, NULL, 0.0f);  
		return false;
	case audioMasterGetSampleRate:
        if(plugin)		
            return plugin->framesPerSecond;	
        else 
            return 44100;
	case audioMasterGetVendorString:
		strcpy((char *)ptr, "vst4cs");
		return 0;
	case audioMasterGetVendorVersion:	
		return 5000;	
	case audioMasterGetProductString:	// Just fooling product string
		strcpy((char*)ptr,"Cubase 5.0");
		return 0;
	case audioMasterVendorSpecific:		
		return 0;
	case audioMasterGetLanguage:		
		return kVstLangEnglish;
	case audioMasterUpdateDisplay:
		if(plugin) plugin->Log("audioMasterUpdateDisplay\n");
		effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;
	case audioMasterSetTime:						
        if(plugin) plugin->Log("VST master dispatcher: Set Time\n");
        break;
	case audioMasterGetNumAutomatableParameters:	
        if(plugin) plugin->Log("VST master dispatcher: GetNumAutPar\n");
        break;
	case audioMasterGetParameterQuantization:
        if(plugin) plugin->Log("VST master dispatcher: ParamQuant\n");
        break;
	case audioMasterIOChanged:
        if(plugin) plugin->Log("VST master dispatcher: IOchanged\n");
        break;
	case audioMasterSizeWindow:
        if(plugin) ptr = &plugin->rect;//plugin->Log("VST master dispatcher: Size Window\n");
        break;
	case audioMasterGetBlockSize:				
        if(plugin) plugin->Log("VST master dispatcher: GetBlockSize\n");
        break;
	case audioMasterGetInputLatency:
        if(plugin) plugin->Log("VST master dispatcher: GetInLatency\n");
        break;
	case audioMasterGetOutputLatency:
	    if(plugin) plugin->Log("VST master dispatcher: GetOutLatency\n");
	    break;
	case audioMasterGetPreviousPlug:
	    if(plugin) plugin->Log("VST master dispatcher: PrevPlug\n");
	    break;
	case audioMasterGetNextPlug: 
        if(plugin) plugin->Log("VST master dispatcher: NextPlug\n");
        return 1;
	case audioMasterWillReplaceOrAccumulate:		
        if(plugin) plugin->Log("VST master dispatcher: WillReplace\n");
        return 1;
	case audioMasterGetCurrentProcessLevel:		
        return 0; 
 	case audioMasterGetAutomationState:			
        if(plugin) plugin->Log("VST master dispatcher: GetAutState\n");
		break;
	case audioMasterOfflineStart:			
        if(plugin) plugin->Log("VST master dispatcher: Offlinestart\n");
        break;
	case audioMasterOfflineRead:
        if(plugin) plugin->Log("VST master dispatcher: Offlineread\n");
        break;
	case audioMasterOfflineWrite:
        if(plugin) plugin->Log("VST master dispatcher: Offlinewrite\n");
        break;
	case audioMasterOfflineGetCurrentPass:
        if(plugin) plugin->Log("VST master dispatcher: OfflineGetcurrentpass\n");
        break;
	case audioMasterOfflineGetCurrentMetaPass:
        if(plugin) plugin->Log("VST master dispatcher: GetGetCurrentMetapass\n");
        break;
	case audioMasterSetOutputSampleRate:		
        if(plugin) plugin->Log("VST master dispatcher: Setsamplerate\n");
        break;
	case audioMasterGetSpeakerArrangement:
        if(plugin) plugin->Log("VST master dispatcher: Getspeaker\n");
        break;
	case audioMasterSetIcon:
        if(plugin) plugin->Log("VST master dispatcher: seticon\n");break;
	case audioMasterCanDo:
        if(plugin) 
            OnCanDo((char *)ptr);
        break;
	case audioMasterOpenWindow:
        if(plugin) plugin->Log("VST master dispatcher: OpenWindow\n");
        break;
	case audioMasterCloseWindow:
	    if(plugin) plugin->Log("VST master dispatcher: CloseWindow\n");
	    break;
	case audioMasterGetDirectory:
        if(plugin) plugin->Log("VST master dispatcher: GetDirectory\n");
        break;
	default: 
        if(plugin) plugin->Log("VST master dispatcher: "
            "undefined opcode: %d , %d\n",opcode , effKeysRequired);
        break;
	}	
	return 0;
}


