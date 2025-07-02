// MIDI input module example
// V Lazzarini, Nov 2024
#include <csdl.h>
#include <poll.h>

static int32_t midi_open(CSOUND *csound,void **userData,
                         const char *devName){
  // set stdin to get data 
  int mode = fcntl(0, F_GETFL, 0);
  fcntl(0, F_SETFL, mode | O_NDELAY);
  return OK;
}

static int32_t midi_read(CSOUND *csound, void *userData,
                         unsigned char *buf, int32_t nBytes) {
  
  struct pollfd fd = {0, POLLIN, 0};
  int32_t cnt = 0, data;
  // poll for stdin data
  if(poll(&fd, 1, 0)) {
    // read stdin MIDI data into buffer
    while(scanf("%d", &data) > 0 && cnt < nBytes)
      buf[cnt++] = data;
  }
  return cnt;
}

static int32_t midi_close(CSOUND *csound,void *userData){
  // nothing to do
  return OK;
}


int32_t csoundModuleCreate(CSOUND *csound)
{
  // announce module
  csound->Message(csound, "stdin MIDI module\n");
  return 0;
}

int32_t csoundModuleInit(CSOUND *csound)
{
  // get requested module name
  char *module = (char*)
    (csound->QueryGlobalVariable(csound, "_RTMIDI"));
  if (module == NULL)
    return 0;
  // check for requested module
  if (strcmp(module, "stdin") != 0)
    return 0;
  // print message - module selected
  csound->Message(csound, "stdin MIDI module enabled\n");
  csound->SetExternalMidiInOpenCallback(csound, midi_open);
  csound->SetExternalMidiReadCallback(csound, midi_read);
  csound->SetExternalMidiInCloseCallback(csound, midi_close);
  return 0;
}

// versioning
int32_t csoundModuleInfo(void)
{
  return ((CS_VERSION << 16) +
          (CS_SUBVER << 8) +
          (int32_t) sizeof(MYFLT));
}
