//vst4cs --- VST host opcodes for Csound5
//By: Andres Cabrera
//Modifications: Michael Gogins
//June 2004
//Using code by Hermann Seib and from the vst~ object in pd 
//(which in turn borrows from the psycle tracker).
#include "vsthost.h"

/*typedef struct _dsp_args
{
	float num_samples;
	float sample_rate;
	int num_in;
	int num_out;
	t_float** inbufs;
	t_float** outbufs;
	VSTPlugin *plug;
} t_dsp_args;
*/

VstTimeInfo VSTPlugin::_timeInfo;

float VSTPlugin::sample_rate = 44100;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//VSTPlugin class implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////

// Constructor
VSTPlugin::VSTPlugin() : 
    h_dll(0),
    _sDllName(0),
    instantiated(false),
    overwrite(false),
    show_params(false),
    _midichannel(0),
    edited(false),
    w(0)
{
	 //w = GetForegroundWindow();
}

VSTPlugin::~VSTPlugin()
{
	Free(0);				// Call free
	delete _sDllName;	// if _sDllName = NULL , the operation does nothing -> it's safe.
}

bool VSTPlugin::AddMIDI(int data0, int data1, int data2)
{
	if (instantiated) {
	    VstMidiEvent vstMidiEvent;	    
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
		vstMidiEventQueue.push_back(vstMidiEvent);
		SendMidi();
		return true;
	}
	else 
        return false;
}

bool VSTPlugin::AddNoteOn(MYFLT note, MYFLT speed, MYFLT midichannel)
{
	if(instantiated)
	{
		VstMidiEvent vstMidiEvent;
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
		vstMidiEvent.midiData[0] = (char) (MIDI_NOTEON | ((int) midichannel)); 
		vstMidiEvent.midiData[1] = (char) note;
		vstMidiEvent.midiData[2] = (char) speed;
		vstMidiEvent.midiData[3] = 0;
		vstMidiEventQueue.push_back(vstMidiEvent);
		SendMidi();
		return true;
	}
	else	
        return false;
}

bool VSTPlugin::AddNoteOff(MYFLT note, MYFLT midichannel)
{
	if (instantiated)
	{
		VstMidiEvent vstMidiEvent;
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
		vstMidiEvent.midiData[0] = (char) (MIDI_NOTEOFF | ((int) midichannel)); 
		vstMidiEvent.midiData[1] = (char) note;
		vstMidiEvent.midiData[2] = 0;
		vstMidiEvent.midiData[3] = 0;
		vstMidiEventQueue.push_back(vstMidiEvent);
		SendMidi();
		return true;
	}
	else
    	return false;
}

bool VSTPlugin::DescribeValue(int p,char* value)
{
	int parameter = p;
	if(instantiated)
	{
		if(parameter<pEffect->numParams)
		{
//			char par_name[64];
			char par_display[64];
			char par_label[64];
//			Dispatch(effGetParamName,parameter,0,par_name,0.0f);
			Dispatch(effGetParamDisplay,parameter,0,par_display,0.0f);
			Dispatch(effGetParamLabel,parameter,0,par_label,0.0f);
//			sprintf(psTxt,"%s:%s%s",par_name,par_display,par_label);
			//printf("%s:%s",par_display,par_label);
			*value = *par_display;
			return true;
		}
		//else	sprintf(psTxt,"NumParams Exeeded");
	}
	//else		sprintf(psTxt,"Not loaded");
	return false;
}

int VSTPlugin::Instance(ENVIRON *csound, const char *dllname)
{
	//printf ("Instance \n");
 	h_dll = csound->OpenLibrary(dllname);
	//strcpy(_sDllName,dllname);
	if(!h_dll)	
	{
		csound->Message(csound, "WARNING! Library \"%s\" was not found or is invalid.\n", dllname);
		return VSTINSTANCE_ERR_NO_VALID_FILE;
	}
	csound->Message(csound, "Loaded library %s\n" , dllname);
	PVSTMAIN main = (PVSTMAIN)csound->GetLibrarySymbol(h_dll, "main");
	if(!main)
	{	
		csound->CloseLibrary(h_dll);
		pEffect = 0;
		instantiated = false;
		return VSTINSTANCE_ERR_NO_VST_PLUGIN;
	}
	//printf("Found main function - about to call it\n");
	//This calls the "main" function and receives the pointer to the AEffect structure.
	pEffect = main((audioMasterCallback)&(this->Master));
	//printf ("Main function called\n");
	if(!pEffect)
	{
		csound->Message(csound, "VST plugin: Unable to create effect.\n");
		csound->CloseLibrary(h_dll);
		pEffect = 0;
		instantiated = false;
		return VSTINSTANCE_ERR_REJECTED;
	}
	if(  pEffect->magic!=kEffectMagic)
	{
		csound->Message(csound, "VST plugin : Instance query rejected by 0x%.8X\n",(int)pEffect);
		csound->CloseLibrary(h_dll);
		pEffect = 0;
		instantiated = false;
		return VSTINSTANCE_ERR_REJECTED;
	}
	pEffect->user = this;
	//  Dispatch( effOpen        ,  0, 0, NULL, 0.0f);
	//  Dispatch( effSetProgram  ,  0, 0, NULL, 0.0f);
	//	Dispatch( effMainsChanged,  0, 1, NULL, 0.0f);
	//  ************************************set samplerate and stream size here
    //  we get it when we init our DSP
    //	Dispatch( effSetSampleRate, 0, 0, NULL, (float)Global::pConfig->_pOutputDriver->_samplesPerSec);
    //	Dispatch( effSetBlockSize,  0, STREAM_SIZE, NULL, 0.0f);
	//printf("all well...\n");
	if ( _sDllName != NULL ) 
        delete _sDllName;
	//_sDllName = new char[strlen(dllname)+1];
	//printf(_sDllName,dllname);
	//keep plugin name
	instantiated = true;
	//Info();
	return VSTINSTANCE_NO_ERROR;
}


void VSTPlugin::Info (ENVIRON *csound)
{
	int i =0;
	csound->Message(csound, "--------------------------------------------\n");
	//printf("VST plugin : Instanced at (Effect*): %.8X\n",(int)pEffect);
	if (!Dispatch( effGetProductString, 0, 0, &_sProductName, 0.0f)) {
		char* str1=_sDllName;
		strcpy(_sProductName,str1);
	}
	csound->Message(csound, "Plugin loaded: %s \n", _sProductName);
	if (!pEffect->dispatcher(pEffect, effGetVendorString, 
        0, 0, &_sVendorName, 0.0f)) {
		strcpy(_sVendorName, "Unknown vendor");
	}
	csound->Message(csound, "Vendor name: %s \n", _sVendorName);
	_version = pEffect->version;
	csound->Message(csound, "Version: %lu \n",_version);
	_isSynth = (pEffect->flags & effFlagsIsSynth)?true:false;
	csound->Message(csound, "Is synth? %s\n",(_isSynth==true?"Yes":"No"));
	overwrite = (pEffect->flags & effFlagsCanReplacing)?true:false;
	editor = (pEffect->flags & effFlagsHasEditor)?true:false;
	csound->Message(csound, "Number of inputs: %i\n", getNumInputs());
	csound->Message(csound, "Number of outputs: %i\n", getNumOutputs());
	long nparams=NumParameters();
	csound->Message(csound, "Number of parameters: %lu \n",nparams);
	for (i=0; i<nparams;i++) {
		char parname[64] ="Empty";
		GetParamName (i, parname);
		csound->Message(csound, "   Parameter%4i: %s\n",i,parname);		
	}
	/*do
	{
		CString sProg;
      char szPgName[256] = "";
      if (!pEffect->EffGetProgramNameIndexed(nClass, j, szPgName))
        {
        bProgSet = true;
        pEffect->EffSetProgram(j);
        pEffect->EffGetProgramName(szPgName);
        if (!*szPgName)
		printf(szPgName, "Program %d", j);
        }
      sProg.Format("%d. %s", j, szPgName);
      popup.AppendMenu(MF_STRING, IDM_EFF_PROGRAM_0 + j, sProg);
      
    } while (j < pEffect->numPrograms)*/
}

int VSTPlugin::getNumInputs( void )
{
	return pEffect->numInputs;
}

int VSTPlugin::getNumOutputs( void )
{
	return pEffect->numOutputs;
}

void VSTPlugin::Create(VSTPlugin *plug)
{
	h_dll=plug->h_dll;
	pEffect=plug->pEffect;
	pEffect->user=this;
	Dispatch( effMainsChanged,  0, 1, NULL, 0.0f);
//	strcpy(_editName,plug->_editName); On current implementation, this replaces the right one. 
	strcpy(_sProductName,plug->_sProductName);
	strcpy(_sVendorName,plug->_sVendorName);
	_sDllName = new char[strlen(plug->_sDllName)+1];
	strcpy(_sDllName,plug->_sDllName);
	_isSynth=plug->_isSynth;
	_version=plug->_version;
	plug->instantiated=false;	// We are "stoling" the plugin from the "plug" object so this
								// is just a "trick" so that when destructing the "plug", it
								// doesn't unload the Dll.
	instantiated=true;
}

void VSTPlugin::Free(ENVIRON *csound)
{
	if(instantiated) {
		instantiated = false;
		pEffect->user = NULL;
		Dispatch( effMainsChanged, 0, 0, NULL, 0.0f);
		Dispatch( effClose,        0, 0, NULL, 0.0f);
//		delete pEffect; // <-  Should check for the necessity of this command.
		pEffect=0;
		if(csound)
            csound->CloseLibrary(h_dll);
	}
}

void VSTPlugin::Init( float samplerate , float blocksize )
{
	sample_rate = samplerate;
	//printf ("init ok\n");
	Dispatch(effOpen        ,  0, 0, NULL, 0.f);
	Dispatch(effSetProgram  ,  0, 0, NULL, 0.0f);
	Dispatch(effMainsChanged,  0, 1, NULL, 0.f);
	Dispatch(effSetSampleRate, 0, 0, 0, (float) sample_rate );
	Dispatch(effSetBlockSize,  0, blocksize, NULL, 0.f );	
}

bool VSTPlugin::SetParameter(int parameter, float value)
{
	if(instantiated) {
		if (( parameter >= 0 ) && (parameter<=pEffect->numParams)) {
			pEffect->setParameter(pEffect,parameter,value);
			return true;
		}
		else return false;
	}
	return false;
}

bool VSTPlugin::SetParameter(int parameter, int value)
{
	return SetParameter(parameter,value/65535.0f);
}

int VSTPlugin::GetCurrentProgram()
{
	if(instantiated)
		return Dispatch(effGetProgram,0,0,NULL,0.0f);
	else
		return 0;
}

void VSTPlugin::SetCurrentProgram(int prg)
{
	if(instantiated)
		Dispatch(effSetProgram,0,prg,NULL,0.0f);
}

void VSTPlugin::SendMidi()
{
    vstEvents.numEvents = vstMidiEventQueue.size();
	if(instantiated && vstEvents.numEvents) {
		vstEvents.reserved  = 0;
		for(size_t i = 0; i < vstEvents.numEvents; i++) {
		  vstEvents.events[i] = (VstEvent *)&vstMidiEventQueue[i];
		}
		Dispatch(effProcessEvents, 0, 0, &vstEvents, 0.0f);
		vstMidiEventQueue.clear();
	}
}

void VSTPlugin::processReplacing( float **inputs, float **outputs, long sampleframes )
{
	pEffect->processReplacing( pEffect , inputs , outputs , sampleframes );
}

void VSTPlugin::process( float **inputs, float **outputs, long sampleframes )
{
	pEffect->process( pEffect , inputs , outputs , sampleframes );
}

long VSTPlugin::Master(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
	//printf("VST plugin call to host dispatcher: Eff: 0x%.8X, Opcode = %d, Index = %d, Value = %d, PTR = %.8X, OPT = %.3f\n",(int)effect, opcode,index,value,(int)ptr,opt);
	//st( "audioMasterWantMidi %d " , audioMasterWantMidi);
	// Support opcodes
	switch(opcode)
	{
	case audioMasterAutomate:			
		return true;		// index, value, returns 0	
	case audioMasterVersion:			
		return OnGetVersion(effect);		// vst version, currently 7 (0 for older)
	case audioMasterCurrentId:			
		return vsthandle;	// returns the unique id of a plug that's currently loading
	case audioMasterIdle:
		effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;		// call application idle routine (this will call effEditIdle for all open editors too) 
	case audioMasterPinConnected:	
		    return !((value) ? 
        OnOutputConnected(effect, index) :
        OnInputConnected(effect, index));	// inquire if an input or output is beeing connected;
	case audioMasterWantMidi:			
		return false;
	case audioMasterProcessEvents:		
		return false; 	// Support of vst events to host is not available
	case audioMasterGetTime:
		memset(&_timeInfo, 0, sizeof(_timeInfo));
		_timeInfo.samplePos = 0;
		_timeInfo.sampleRate = sample_rate;
		return (long)&_timeInfo;
		/* TODO (#1#): Sera este el problema? */
	case audioMasterTempoAt:			
		return 0;
	case audioMasterNeedIdle:	
		//effect->dispatcher(effect, effIdle, 0, 0, NULL, 0.0f);  //linea del problema
		return false;
	case audioMasterGetSampleRate:		
		return sample_rate;	
	case audioMasterGetVendorString:	// Just fooling version string
		//strcpy((char*)ptr,"Steinberg");
		return 0;
	case audioMasterGetVendorVersion:	
		return 5000;	// HOST version 5000
	case audioMasterGetProductString:	// Just fooling product string
		//strcpy((char*)ptr,"Cubase 5.0");
		return 0;
	case audioMasterVendorSpecific:	{
			return 0;
		}
	case audioMasterGetLanguage:		
		return kVstLangEnglish;
	case audioMasterUpdateDisplay:
		//printf("audioMasterUpdateDisplay\n");
		//effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;
	case 	audioMasterSetTime:						printf("VST master dispatcher: Set Time\n");break;
	case 	audioMasterGetNumAutomatableParameters:	printf("VST master dispatcher: GetNumAutPar\n");break;
	case 	audioMasterGetParameterQuantization:	printf("VST master dispatcher: ParamQuant\n");break;
	case 	audioMasterIOChanged:					printf("VST master dispatcher: IOchanged\n");break;
	case 	audioMasterSizeWindow:					printf("VST master dispatcher: Size Window\n");break;
	case 	audioMasterGetBlockSize:				printf("VST master dispatcher: GetBlockSize\n");break;
	case 	audioMasterGetInputLatency:				printf("VST master dispatcher: GetInLatency\n");break;
	case 	audioMasterGetOutputLatency:			printf("VST master dispatcher: GetOutLatency\n");break;
	case 	audioMasterGetPreviousPlug:				printf("VST master dispatcher: PrevPlug\n");break;
	case 	audioMasterGetNextPlug:					printf("VST master dispatcher: NextPlug\n"); return 1;break;
	case 	audioMasterWillReplaceOrAccumulate:		printf("VST master dispatcher: WillReplace\n");return 1; break;
	case 	audioMasterGetCurrentProcessLevel:		return 0; break;
	case 	audioMasterGetAutomationState:			printf("VST master dispatcher: GetAutState\n");
 			break;
	case 	audioMasterOfflineStart:				printf("VST master dispatcher: Offlinestart\n");break;
	case 	audioMasterOfflineRead:					printf("VST master dispatcher: Offlineread\n");break;
	case 	audioMasterOfflineWrite:				printf("VST master dispatcher: Offlinewrite\n");break;
	case 	audioMasterOfflineGetCurrentPass:		printf("VST master dispatcher: OfflineGetcurrentpass\n");break;
	case 	audioMasterOfflineGetCurrentMetaPass:	printf("VST master dispatcher: GetGetCurrentMetapass\n");break;
	case 	audioMasterSetOutputSampleRate:			printf("VST master dispatcher: Setsamplerate\n");break;
	case 	audioMasterGetSpeakerArrangement:		printf("VST master dispatcher: Getspeaker\n");break;
	case 	audioMasterSetIcon:						printf("VST master dispatcher: seticon\n");break;
	case 	audioMasterCanDo:						//printf("VST master dispatcher: Can Do\n");
													OnCanDo((char *)ptr);
													//(char*)ptr = hostCanDos; /*printf ("%s",(char*)ptr[1])*/; 									
         											break;
	case 	audioMasterOpenWindow:					printf("VST master dispatcher: OpenWindow\n");break;
	case 	audioMasterCloseWindow:					printf("VST master dispatcher: CloseWindow\n");break;
	case 	audioMasterGetDirectory:				printf("VST master dispatcher: GetDirectory\n");break;
	//case	audioMasterUpdateDisplay:				printf("VST master dispatcher: audioMasterUpdateDisplay");break;
	default: printf("VST master dispatcher: Undefined opcode: %d , %d\n",opcode , effKeysRequired )	;break;
	}	
	return 0;
}


bool VSTPlugin::replace()
{
	return overwrite;
}

void VSTPlugin::edit()
{	
	if(instantiated)
	{ 		
		if ( ( editor ) && (!edited))
		{			
			edited = true;
			//b = (CEditorThread*) AfxBeginThread(RUNTIME_CLASS( CEditorThread ) );
			/*b =  new CEditorThread();			
			b->SetPlugin( this );			
			b->CreateThread();	*/		
		}
	}
}

void VSTPlugin::EditorIdle()
{
	Dispatch(effEditIdle,0,0, w,0.0f);			
}

/*RECT VSTPlugin::GetEditorRect()
{
	RECT ret;
	ERect *r;
	Dispatch(effEditGetRect,0,0, &r,0.0f);				
	ret.top = r->top;
	ret.bottom = r->bottom;
	ret.left = r->left;
	ret.right = r->right;
	return ret;
}*/

void VSTPlugin::SetEditWindow(void *h)
{
	w = h;	
	Dispatch(effEditOpen,0,0, w,0.0f);							
}

void VSTPlugin::OnEditorCLose()
{
	Dispatch(effEditClose,0,0, w,0.0f);					
}

void VSTPlugin::SetShowParameters(bool s)
{
	show_params = s;
}

bool VSTPlugin::ShowParams()
{
	return show_params;
}

void VSTPlugin::AddAftertouch(int value)
{
    if (value < 0) 
        value = 0; 
    else if (value > 127) 
        value = 127;
 	AddMIDI( (char)MIDI_NOTEOFF | _midichannel , value );
}

void VSTPlugin::AddPitchBend(int value)
{
    AddMIDI( MIDI_PITCHBEND + (_midichannel & 0xf) , ((value>>7) & 127), (value & 127));
}

void VSTPlugin::AddProgramChange(int value)
{
    if (value < 0) 
        value = 0; 
    else if (value > 127) 
        value = 127;
    AddMIDI( MIDI_PROGRAMCHANGE + (_midichannel & 0xf), value, 0);
}

void VSTPlugin::AddControlChange(int control, int value)
{
  if (control < 0) 
    control = 0;  
  else if (control > 127) 
    control = 127;
  if (value < 0) 
    value = 0;  
  else if (value > 127) 
    value = 127;
  AddMIDI( MIDI_CONTROLCHANGE + (_midichannel & 0xf), control, value);
}

bool VSTPlugin::GetProgramName( int cat , int p, char *buf)
{
	int parameter = p;
	if(instantiated) {
		if(parameter<NumPrograms()) {
			Dispatch(effGetProgramNameIndexed,parameter,cat,buf,0.0f);
			return true;
		}
	}
	return false;
}

int VSTPlugin::GetNumCategories()
{
	if(instantiated)
		return Dispatch(effGetNumProgramCategories,0,0,NULL,0.0f);
	else
		return 0;
}

void VSTPlugin::StopEditing()
{
	edited = false;
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

