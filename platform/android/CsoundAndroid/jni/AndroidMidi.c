#include <amidi/AMidi.h>
#include <csdl.h>
#include <stdlib.h>

static int32_t open_midi_in(CSOUND *csound, void **userData, const char *dev) {
    AMidiOutputPort* port;
    AMidiDevice *mididevice = *((AMidiDevice **)
                                csound->QueryGlobalVariable(csound, "ANDROID_MIDI_IN"));       
    int32_t result =
      AMidiOutputPort_open(mididevice, atoi(dev), &port);
    *userData = port;
    return result == AMEDIA_OK ? 0  : CSOUND_ERROR;
}

static int32_t open_midi_out(CSOUND *csound, void **userData, const char *dev) {
    AMidiInputPort* port;
    AMidiDevice *mididevice = *((AMidiDevice **)
                                csound->QueryGlobalVariable(csound, "ANDROID_MIDI_OUT"));                   
    int32_t result =
      AMidiInputPort_open(mididevice, atoi(dev), &port);
    *userData = port;
    return result == AMEDIA_OK ? 0  : CSOUND_ERROR;
}

static int32_t read_midi(CSOUND *csound, void *userData,
                         unsigned char *mbuf, int32_t nbytes) {
  AMidiOutputPort* port = (AMidiOutputPort*) userData;
  uint8_t buffer[MBUFSIZ];
  size_t n;
  int32_t opc;
  int64_t timestamp;
  size_t recv;
  int32_t rbytes = 0;

  do {
    n = AMidiOutputPort_receive(port, &opc, buffer, MBUFSIZ, &recv, &timestamp);
    if(opc == AMIDI_OPCODE_DATA) {
      for(int i = 0; i < recv; i++) {
        if(rbytes > nbytes) {
          *mbuf++ = buffer[i];
          rbytes++;
        }
      }
    }
  } while (n > 0);
  return rbytes;
}

static int32_t write_midi(CSOUND *csound, void *userData,
                          const unsigned char *mbuf, int32_t nbytes) {
   AMidiInputPort* port = (AMidiInputPort*) userData;
   return (int32_t) AMidiInputPort_send(port, mbuf, nbytes);
}


static int32_t close_midi_in(CSOUND *csound, void *userData) {
    AMidiOutputPort* port = (AMidiOutputPort*) userData;
    AMidiOutputPort_close(port);
    return CSOUND_SUCCESS;
}


static int32_t close_midi_out(CSOUND *csound, void *userData) {
    AMidiInputPort* port = (AMidiInputPort*) userData;
    AMidiInputPort_close(port);
    return CSOUND_SUCCESS;
}

static int32_t list_devs(CSOUND *csound, CS_MIDIDEVICE *list,
                         int32_t isOutput){
  return 0;
}

void android_midi_init(CSOUND *csound, JNIEnv* env, jobject obj_in, jobject obj_out){
  AMidiDevice *mididevice, **mididevin, **mididevout;
  csound->CreateGlobalVariable(csound, "ANDROID_MIDI_IN", sizeof(AMidiDevice*));
  mididevin = (AMidiDevice **)
    csound->QueryGlobalVariable(csound, "ANDROID_MIDI_IN");
  if(obj_in != NULL) {
  AMidiDevice_fromJava(env, obj_in, &mididevice);
  *mididevin = mididevice;
  } else *mididevin = NULL;
  
  csound->CreateGlobalVariable(csound, "ANDROID_MIDI_OUT", sizeof(AMidiDevice*));
  mididevout = (AMidiDevice **)
    (csound->QueryGlobalVariable(csound, "ANDROID_MIDI_OUT"));
  *mididevin = mididevice;
  if(obj_out != NULL) {
    AMidiDevice_fromJava(env, obj_out, &mididevice);
    *mididevout = mididevice;
  } else *mididevout = NULL;
  
  csound->SetExternalMidiInOpenCallback(csound, open_midi_in);
  csound->SetExternalMidiReadCallback(csound, read_midi);
  csound->SetExternalMidiInCloseCallback(csound, close_midi_in);
  csound->SetExternalMidiOutOpenCallback(csound, open_midi_out);
  csound->SetExternalMidiWriteCallback(csound, write_midi);
  csound->SetExternalMidiOutCloseCallback(csound, close_midi_out);
  csound->SetMIDIDeviceListCallback(csound,list_devs);
}


void android_midi_deinit(CSOUND *csound) {
  AMidiDevice *mididevice = *((AMidiDevice **)
                                csound->QueryGlobalVariable(csound, "ANDROID_MIDI_OUT"));
  if(mididevice != NULL)
    AMidiDevice_release(mididevice);

  csound->DestroyGlobalVariable(csound, "ANDROID_MIDI_OUT");

  mididevice = *((AMidiDevice **)
                 csound->QueryGlobalVariable(csound, "ANDROID_MIDI_IN"));
  if(mididevice != NULL)
    AMidiDevice_release(mididevice);

  csound->DestroyGlobalVariable(csound, "ANDROID_MIDI_IN");
}
