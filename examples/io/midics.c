// MIDI input module example
// V Lazzarini, Nov 2024
#include <CsoundLib64/csound.h>
#include <CsoundLib64/csdl.h>
#include <fcntl.h>
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

static void set_midi_callbacks(CSOUND *csound) {
    csound->SetExternalMidiInOpenCallback(csound, midi_open);
    csound->SetExternalMidiReadCallback(csound, midi_read);
    csound->SetExternalMidiInCloseCallback(csound, midi_close);
}

int main(int argc, const char *argv[]) {
  CSOUND *csound = csoundCreate(NULL, NULL);
  int32_t result = csoundCompile(csound, argc, argv);
  if(result == 0) {
    // set realtime MIDI input and audio to dac
    csoundSetOption(csound, "-M0 -odac");
    set_midi_callbacks(csound);
    csoundSetHostMIDIIO(csound);
    result = csoundStart(csound);
    while(result == 0) result = csoundPerformKsmps(csound);
  }
  csoundDestroy(csound);
  return OK;
}
