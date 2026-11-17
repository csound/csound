#include <amidi/AMidi.h>
#include "csoundCore.h"
#include <stdlib.h>

/* What setMidiIO set up, until Csound's reset deletes it */
typedef struct {
    AMidiDevice *in, *out;
    AMidiOutputPort *in_port;   /* the ports Csound has open on them */
    AMidiInputPort *out_port;
    int32_t host_midi;  /* enableHostImplementedMIDIIO before setMidiIO */
    int32_t active;
} ANDROID_MIDI;

static ANDROID_MIDI *get_midi(CSOUND *csound) {
    return (ANDROID_MIDI *) csound->QueryGlobalVariable(csound, "ANDROID_MIDI");
}

/* With no device for a direction, opening it gives no port, not an error:
   setMidiIO always asks for both. */
static int32_t open_midi_in(CSOUND *csound, void **userData, const char *dev) {
    AMidiOutputPort* port = NULL;
    ANDROID_MIDI *midi = get_midi(csound);
    int32_t result = midi == NULL || midi->in == NULL ? AMEDIA_OK :
      AMidiOutputPort_open(midi->in, atoi(dev), &port);
    if (port != NULL)
      midi->in_port = port;
    *userData = port;
    return result == AMEDIA_OK ? 0  : CSOUND_ERROR;
}

static int32_t open_midi_out(CSOUND *csound, void **userData, const char *dev) {
    AMidiInputPort* port = NULL;
    ANDROID_MIDI *midi = get_midi(csound);
    int32_t result = midi == NULL || midi->out == NULL ? AMEDIA_OK :
      AMidiInputPort_open(midi->out, atoi(dev), &port);
    if (port != NULL)
      midi->out_port = port;
    *userData = port;
    return result == AMEDIA_OK ? 0  : CSOUND_ERROR;
}

/* One message per call: Csound asks again once it has parsed it, and a
   message taken from the port cannot be put back. */
static int32_t read_midi(CSOUND *csound, void *userData,
                         unsigned char *mbuf, int32_t nbytes) {
  AMidiOutputPort* port = (AMidiOutputPort*) userData;
  int32_t opc;
  int64_t timestamp;
  size_t recv;
  ssize_t n;

  if (port == NULL)
    return 0;
  do {
    n = AMidiOutputPort_receive(port, &opc, mbuf, nbytes, &recv, &timestamp);
    if (n > 0 && opc == AMIDI_OPCODE_DATA)
      return (int32_t) recv;
  } while (n > 0);
  return (int32_t) n;
}

static int32_t write_midi(CSOUND *csound, void *userData,
                          const unsigned char *mbuf, int32_t nbytes) {
   AMidiInputPort* port = (AMidiInputPort*) userData;
   return (int32_t) AMidiInputPort_send(port, mbuf, nbytes);
}


static int32_t close_midi_in(CSOUND *csound, void *userData) {
    AMidiOutputPort* port = (AMidiOutputPort*) userData;
    ANDROID_MIDI *midi = get_midi(csound);
    if (midi != NULL && midi->in_port == port)
      midi->in_port = NULL;
    AMidiOutputPort_close(port);
    return CSOUND_SUCCESS;
}


static int32_t close_midi_out(CSOUND *csound, void *userData) {
    AMidiInputPort* port = (AMidiInputPort*) userData;
    ANDROID_MIDI *midi = get_midi(csound);
    if (midi != NULL && midi->out_port == port)
      midi->out_port = NULL;
    AMidiInputPort_close(port);
    return CSOUND_SUCCESS;
}

static int32_t list_devs(CSOUND *csound, CS_MIDIDEVICE *list,
                         int32_t isOutput){
  return 0;
}

static AMidiDevice *device_from_java(CSOUND *csound, JNIEnv* env, jobject obj) {
  AMidiDevice *device = NULL;
  media_status_t result;
  if (obj == NULL)
    return NULL;
  result = AMidiDevice_fromJava(env, obj, &device);
  if (result != AMEDIA_OK) {
    csound->Warning(csound, Str("Android MIDI: cannot open the MidiDevice (%d)\n"),
                    (int) result);
    return NULL;
  }
  return device;
}

static int32_t release_devices(CSOUND *csound) {
  ANDROID_MIDI *midi = get_midi(csound);
  if (midi == NULL || !midi->active)
    return CSOUND_SUCCESS;
  if (midi->in_port != NULL || midi->out_port != NULL) {
    csound->Warning(csound, Str("Android MIDI: the devices are in use until the run ends\n"));
    return CSOUND_ERROR;
  }
  if (midi->out != NULL && midi->out != midi->in)
    AMidiDevice_release(midi->out);
  if (midi->in != NULL)
    AMidiDevice_release(midi->in);
  midi->in = midi->out = NULL;
  midi->active = 0;
  csound->enableHostImplementedMIDIIO = midi->host_midi;
  return CSOUND_SUCCESS;
}

/* csoundReset closes Csound's ports in midi_close, then calls this, then
   deletes ANDROID_MIDI: a port still open here would lose the devices */
static int32_t release_on_reset(CSOUND *csound, void *userData) {
  if (release_devices(csound) != CSOUND_SUCCESS)
    csound->ErrorMsg(csound, Str("Android MIDI: a port is still open at reset, "
                                 "and its devices are lost\n"));
  return 0;
}

void android_midi_init(CSOUND *csound, JNIEnv* env, jobject obj_in, jobject obj_out){
  ANDROID_MIDI *midi;
  if (release_devices(csound) != CSOUND_SUCCESS)
    return;
  /* created once until a reset */
  if (csound->CreateGlobalVariable(csound, "ANDROID_MIDI", sizeof(ANDROID_MIDI)) == CSOUND_SUCCESS)
    csound->RegisterResetCallback(csound, NULL, release_on_reset);
  if ((midi = get_midi(csound)) == NULL)
    return;
  midi->in = device_from_java(csound, env, obj_in);
  /* AMidiDevice_fromJava refuses a device already open, so one MidiDevice
     for both directions is opened once */
  if (obj_in != NULL && obj_out != NULL && (*env)->IsSameObject(env, obj_in, obj_out))
    midi->out = midi->in;
  else
    midi->out = device_from_java(csound, env, obj_out);
  midi->host_midi = csound->enableHostImplementedMIDIIO;
  midi->active = 1;

  csound->SetExternalMidiInOpenCallback(csound, open_midi_in);
  csound->SetExternalMidiReadCallback(csound, read_midi);
  csound->SetExternalMidiInCloseCallback(csound, close_midi_in);
  csound->SetExternalMidiOutOpenCallback(csound, open_midi_out);
  csound->SetExternalMidiWriteCallback(csound, write_midi);
  csound->SetExternalMidiOutCloseCallback(csound, close_midi_out);
  csound->SetMIDIDeviceListCallback(csound,list_devs);
  /* host MIDI, or -+rtmidi=null in a CSD puts Csound's dummy callbacks back */
  csound->enableHostImplementedMIDIIO = 1;
}


void android_midi_deinit(CSOUND *csound) {
  release_devices(csound);
}
