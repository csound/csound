//  vst4cs --- VST host opcodes for Csound5
//  By: Andres Cabrera
//  Modifications: Michael Gogins
//  June 2004
//  Using code by Hermann Seib and from the vst~ object in pd 
//  (which in turn borrows from the psycle tracker).
#ifndef _VSTPLUGIN_HOST
#define _VSTPLUGIN_HOST

#include "cs.h"
#include "aeffectx.h"
#include <vector>

#define MAX_EVENTS		64
#define MAX_INOUTS		8
#define VSTINSTANCE_ERR_NO_VALID_FILE -1
#define VSTINSTANCE_ERR_NO_VST_PLUGIN -2
#define VSTINSTANCE_ERR_REJECTED -3
#define VSTINSTANCE_NO_ERROR 0
#define MIDI_NOTEON 144
#define MIDI_NOTEOFF 128
#define MIDI_POLYAFTERTOUCH 160
#define MIDI_CONTROLCHANGE 176
#define MIDI_PROGRAMCHANGE 192
#define MIDI_AFTERTOUCH 208
#define MIDI_PITCHBEND 224

typedef AEffect* (*PVSTMAIN)(audioMasterCallback audioMaster);
static int vsthandle = -1;

typedef union VstEventBlock_
{
    VstEvents vstEvents;
    void *block[0x1000];
} VstEventBlock;

class VSTPlugin
{
public:
	VSTPlugin();
	virtual ~VSTPlugin();
	void StopEditing();
	int GetNumCategories();
	bool GetProgramName( int cat, int p , char* buf);
	void AddControlChange( int control , int value );
	void AddProgramChange( int value );
	void AddPitchBend( int value );
	void AddAftertouch( int value );
	bool editor;
	bool ShowParams();
	void SetShowParameters( bool s);
	void OnEditorCLose();
	void SetEditWindow( void * h );
	//CEditorThread* b;
	//RECT GetEditorRect();
	void EditorIdle();
	void edit(void);
	bool replace(  );
	void Free(ENVIRON *csound);
	int Instance(ENVIRON *csound, const char *sharedLibraryPath);
	void Info(ENVIRON *csound);
	void Create(VSTPlugin *plug);
	void Init(ENVIRON *csound);
	virtual int GetNumParams(void) { return pEffect->numParams; }
	virtual void GetParamName(int numparam,char* name)
	{
		if ( numparam < pEffect->numParams ) 
            Dispatch(effGetParamName,numparam,0,name,0.0f);
		else 
            strcpy(name,"Out of Range");
	}
	/*virtual void GetParamValue(int numparam,char* parval)
	{
		if ( numparam < pEffect->numParams ) DescribeValue(numparam,parval);
		else strcpy(parval,"Out of Range");
	}*/
	virtual float GetParamValue(int numparam) {
		if ( numparam < pEffect->numParams ) 
            return (pEffect->getParameter(pEffect, numparam));
		else 
            return -1.0;
	}
	int getNumInputs( void );
	int getNumOutputs( void );
	virtual char* GetName(void) { return _sProductName; }
	unsigned long GetHostVersion() { return _version; }
	char* GetVendorName(void) { return _sVendorName; }
	char* GetDllName(void) { return _sDllName; }
	long NumParameters(void) { return pEffect->numParams; }
	float GetParameter(long parameter) { return pEffect->getParameter(pEffect, parameter); }
	bool DescribeValue(int parameter,char* psTxt);
	bool SetParameter(int parameter, float value); 
	bool SetParameter(int parameter, int value); //termina llamando  a la anterior
	void SetCurrentProgram(int prg);
	int GetCurrentProgram();
	int NumPrograms() { return pEffect->numPrograms; }
	bool IsSynth() { return _isSynth; }
	bool AddMIDI(int data0, int data1=0, int data2=0);
	void SendMidi();
	void processReplacing( float **inputs, float **outputs, long sampleframes );
	void process( float **inputs, float **outputs, long sampleframes );
	/* TODO (#6#): Aqui puede haber problema */
	AEffect *pEffect;
	long Dispatch(long opCode, long index, long value, void *ptr, float opt) {
		return pEffect->dispatcher(pEffect, opCode, index, value, ptr, opt);
	}
	static long Master(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);
	bool AddNoteOn( MYFLT note,MYFLT speed,MYFLT midichannel=1);
	bool AddNoteOff( MYFLT note,MYFLT midichannel=1);
	char _midichannel;
	bool instantiated;
	int _instance;		// Remove when Changing the FileFormat.
	void *w;
	static bool OnCanDo(const char *ptr) {
	//printf ("%c\n",ptr);
	if ((!strcmp(ptr, "sendVstMidiEvent")) ||
	    (!strcmp(ptr, "receiveVstMidiEvent")) /*||
	    (!strcmp(ptr, "sizeWindow"))*/ )
	  return true;
	  //printf("Can't Do\n");
	  return false; 
	}
	static long OnGetVersion(AEffect *effect);
    static bool OnInputConnected(AEffect *effect, long input) { return true; }
    static bool OnOutputConnected(AEffect *effect, long output) { return true; }
	void *h_dll;
	void *h_winddll;
	char _sProductName[64];
	char _sVendorName[64];
	char *_sDllName;	// Contains dll name
	unsigned long _version;
	bool _isSynth;
	float *inputs[MAX_INOUTS];
	float *outputs[MAX_INOUTS];
	size_t vstMidiEventIndex;
	VstMidiEvent vstMidiEventQueue[0x1000];
	VstEventBlock vstEventBlock;
	bool overwrite;
	bool edited;
	bool show_params;
	static VstTimeInfo vstTimeInfo;
	static float sampleRate;
	static long blockSize;
};

#endif // _VSTPLUGIN_HOST
