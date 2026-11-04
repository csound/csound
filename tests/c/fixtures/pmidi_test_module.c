/*
  pmidi_test_module.c:

  InOut/pmidi.c compiled against stand-in PortMidi and PortTime functions,
  so the PortMIDI module opens devices without MIDI hardware and without
  the PortMidi library: only its headers are used. The stand-ins offer two
  inputs and two outputs, one of each without an interface name.
*/

#define Pm_Initialize csound_test_Pm_Initialize
#define Pm_Terminate csound_test_Pm_Terminate
#define Pm_CountDevices csound_test_Pm_CountDevices
#define Pm_GetDeviceInfo csound_test_Pm_GetDeviceInfo
#define Pm_GetDefaultInputDeviceID csound_test_Pm_GetDefaultInputDeviceID
#define Pm_GetDefaultOutputDeviceID csound_test_Pm_GetDefaultOutputDeviceID
#define Pm_OpenInput csound_test_Pm_OpenInput
#define Pm_OpenOutput csound_test_Pm_OpenOutput
#define Pm_SetFilter csound_test_Pm_SetFilter
#define Pm_Poll csound_test_Pm_Poll
#define Pm_Read csound_test_Pm_Read
#define Pm_Write csound_test_Pm_Write
#define Pm_Close csound_test_Pm_Close
#define Pm_GetErrorText csound_test_Pm_GetErrorText
#define Pt_Start csound_test_Pt_Start
#define Pt_Stop csound_test_Pt_Stop
/* libcsound defines these too */
#define csoundLock csound_test_pmidi_lock
#define csoundUnLock csound_test_pmidi_unlock
#define csoundModuleCreate csound_test_pmidi_module_create
#define csoundModuleInit csound_test_pmidi_module_init
#define csoundModuleDestroy csound_test_pmidi_module_destroy
#define csoundModuleInfo csound_test_pmidi_module_info
#include "../../../InOut/pmidi.c"

static const PmDeviceInfo devices[] = {
  { .interf = "Stand-in", .name = "Keys", .input = 1 },
  { .interf = NULL, .name = "Pads", .input = 1 },
  { .interf = "Stand-in", .name = "Synth", .output = 1 },
  { .interf = NULL, .name = "Box", .output = 1 },
};
static int stream;

PmError Pm_Initialize(void) { return pmNoError; }
PmError Pm_Terminate(void) { return pmNoError; }
int Pm_CountDevices(void) { return (int) (sizeof(devices) / sizeof(devices[0])); }
const PmDeviceInfo *Pm_GetDeviceInfo(PmDeviceID id)
{
    return id >= 0 && id < Pm_CountDevices() ? &devices[id] : NULL;
}
PmDeviceID Pm_GetDefaultInputDeviceID(void) { return 0; }
PmDeviceID Pm_GetDefaultOutputDeviceID(void) { return 2; }
PmError Pm_OpenInput(PortMidiStream **s, PmDeviceID id, void *info,
                     int32_t size, PmTimeProcPtr time, void *time_info)
{
    (void) id; (void) info; (void) size; (void) time; (void) time_info;
    *s = &stream;
    return pmNoError;
}
PmError Pm_OpenOutput(PortMidiStream **s, PmDeviceID id, void *info,
                      int32_t size, PmTimeProcPtr time, void *time_info,
                      int32_t latency)
{
    (void) id; (void) info; (void) size; (void) time; (void) time_info;
    (void) latency;
    *s = &stream;
    return pmNoError;
}
PmError Pm_SetFilter(PortMidiStream *s, int32_t filters)
{
    (void) s; (void) filters;
    return pmNoError;
}
PmError Pm_Poll(PortMidiStream *s) { (void) s; return pmNoData; }
int Pm_Read(PortMidiStream *s, PmEvent *buffer, int32_t length)
{
    (void) s; (void) buffer; (void) length;
    return 0;
}
PmError Pm_Write(PortMidiStream *s, PmEvent *buffer, int32_t length)
{
    (void) s; (void) buffer; (void) length;
    return pmNoError;
}
PmError Pm_Close(PortMidiStream *s) { (void) s; return pmNoError; }
const char *Pm_GetErrorText(PmError error) { (void) error; return ""; }
PtError Pt_Start(int resolution, PtCallback *callback, void *data)
{
    (void) resolution; (void) callback; (void) data;
    return ptNoError;
}
PtError Pt_Stop(void) { return ptNoError; }

int32_t csound_test_pmidi_open_input(CSOUND *csound, void **data,
                                     const char *dev)
{
    return OpenMidiInDevice_(csound, data, dev);
}

int32_t csound_test_pmidi_close_input(CSOUND *csound, void *data)
{
    return CloseMidiInDevice_(csound, data);
}

int32_t csound_test_pmidi_open_output(CSOUND *csound, void **data,
                                      const char *dev)
{
    return OpenMidiOutDevice_(csound, data, dev);
}

int32_t csound_test_pmidi_close_output(CSOUND *csound, void *data)
{
    return CloseMidiOutDevice_(csound, data);
}
