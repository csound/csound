#ifndef _VSTPLUGIN_HOST
#define _VSTPLUGIN_HOST

#include "cs.h"
#include "AEffectx.h"
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

class VSTPlugin
{
public:
    ENVIRON *csound;
	void *libraryHandle;
	AEffect *aeffect;
	char productName[64];
	char vendorName[64];
	char libraryName[0x100];
	unsigned long _version;
	bool _isSynth;
	float **inputs;
	float **outputs;
	static VstTimeInfo _timeInfo;
	VstMidiEvent vstMidiEvents[MAX_EVENTS];
	char buffer[0x1000];
	VstEvents *vstEvents;
	int	queue_size;
	bool overwrite;
	bool edited;
	bool show_params;
	static float sample_rate;
	float blockSize;
	char _midichannel;
	int _instance;		// Remove when Changing the FileFormat.
	void *w;
	VSTPlugin(ENVIRON *csound);
	virtual ~VSTPlugin();
	void StopEditing();
	int GetNumCategories();
	bool GetProgramName(int cat, int p , char* buf);
	void AddControlChange(int control , int value);
	void AddProgramChange(int value);
	void AddPitchBend(int value);
	void AddAftertouch(int value);
	bool editor;
	bool ShowParams();
	void SetShowParameters(bool s);
	void OnEditorCLose();
	void SetEditWindow(void *h);
	//CEditorThread* b;
	//RECT GetEditorRect();
	void EditorIdle();
	void edit(void);
	bool replace();
	void Free();
	int Instance(const char *dllname);
	void Info();
	void Init(float samplerate , float blocksize);
	virtual int GetNumParams(void) { return aeffect->numParams; }
	virtual void GetParamName(int numparam,char* name) {
		if(numparam < aeffect->numParams) Dispatch(effGetParamName,numparam,0,name,0.0f);
		else strcpy(name,"Parameter out of range.");
	}
	virtual float GetParamValue(int numparam) {
		if(numparam < aeffect->numParams) 
            return(aeffect->getParameter(aeffect, numparam));
		else 
            return -1.0;
	}
	int getNumInputs(void);
	int getNumOutputs(void);
	virtual char* GetName(void) { return productName; }
	unsigned long  GetVersion() { return _version; }
	char* GetVendorName(void) { return vendorName; }
	char* GetDllName(void) { return libraryName; }
	long NumParameters(void) { return aeffect->numParams; }
	float GetParameter(long parameter) { return aeffect->getParameter(aeffect, parameter); }
	bool DescribeValue(int parameter,char* psTxt);
	bool SetParameter(int parameter, float value); 
	bool SetParameter(int parameter, int value); //termina llamando  a la anterior
	void SetCurrentProgram(int prg);
	int GetCurrentProgram();
	int NumPrograms() { return aeffect->numPrograms; }
	bool IsSynth() { return _isSynth; }
	bool AddMIDI(int data0,int data1=0,int data2=0);
	void SendMidi();
	void processReplacing(float **inputs, float **outputs, long sampleframes);
	void process(float **inputs, float **outputs, long sampleframes);
	long Dispatch(long opCode, long index, long value, void *ptr, float opt) {
		return aeffect->dispatcher(aeffect, opCode, index, value, ptr, opt);
	}
	static long Master(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);
	bool AddNoteOn(int channel, MYFLT note, MYFLT speed);
	bool AddNoteOff(int channel,  MYFLT note);
	static bool OnCanDo(const char *ptr) {
	    if((!strcmp(ptr, "sendVstMidiEvent")) ||
	       (!strcmp(ptr, "receiveVstMidiEvent")) /*||
	       (!strcmp(ptr, "sizeWindow"))*/)
	          return true;
	    return false; 
	}
	static long OnGetVersion(AEffect *effect);
    static bool OnInputConnected(AEffect *effect, long input) { return true; }
    static bool OnOutputConnected(AEffect *effect, long output) { return true; }
};

#endif // _VSTPLUGIN_HOST
