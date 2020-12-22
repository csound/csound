#include <stdio.h>
#include <stdlib.h>
/* #include <signal.h> */
#include <string.h>
#include <unistd.h>
/* #include <limits.h> */
#include "csound.h"
#include "csoundCore.h"
/* #include "fftlib.h" */
/* #include "lpred.h" */

#ifdef INIT_STATIC_MODULES
extern int init_static_modules(CSOUND *csound);
extern int scansyn_init_(CSOUND *csound);
extern int scansynx_init_(CSOUND *csound);
extern int emugens_init_(CSOUND *csound);
extern int pvsops_init_(CSOUND *csound);
extern int liveconv_init_(CSOUND *csound);
extern int unsupported_opdoces_init_(CSOUND *csound);
extern int dateops_init_(CSOUND *csound);
#endif

// returns the address of a string
// pointer which is writable from js
char* allocStringMem (int length) {
  char *ptr = NULL;
  ptr = malloc(((length + 1) * sizeof(char)));
  // NULL Terminate
  ptr[length] = 0;
  return ptr;
}

// free the allocated String Memory
// (this could be unneccecary, dont know)
void freeStringMem (char* ptr) {
  free(ptr);
}

CSOUND_PARAMS* allocCsoundParamsStruct() {
  CSOUND_PARAMS* ptr = NULL;
  ptr = malloc(sizeof(CSOUND_PARAMS));
  return ptr;
}

void freeCsoundParams(CSOUND_PARAMS* ptr) {
  free(ptr);
}


// START CS_MIDIDEVICE
int sizeOfMidiStruct() {
  // TODO: write comparison test of js/c sizeof
  return sizeof(CS_MIDIDEVICE);
}

CS_MIDIDEVICE* allocCsMidiDeviceStruct(int num) {
  CS_MIDIDEVICE* ptr = NULL;
  ptr = malloc(sizeof(CS_MIDIDEVICE) * num);
  return ptr;
}


void freeCsMidiDeviceStruct(CSOUND_PARAMS* ptr) {
  free(ptr);
}
// END CS_MIDIDEVICE

int csoundStartWasi(CSOUND *csound) {
  // got annoyed, fix this before adding more targets!
  csoundAppendEnv(csound, "SADIR", "/csound/");
  csoundAppendEnv(csound, "SSDIR", "/csound/");
  csoundAppendEnv(csound, "INCDIR", "/csound/");
  csoundAppendEnv(csound, "MFDIR", "/csound/");

  const char* outputDev = csoundGetOutputName(csound);
  // detect realtime mode automatically
  if (strncmp("dac", outputDev, 3) == 0) {
    csoundSetHostImplementedAudioIO(csound, 1, 0);
  }
  return csoundStart(csound);
}

// The built-in performKsmps has mutex and setjmp
// which we don't have in wasi based wasm
int csoundPerformKsmpsWasi(CSOUND *csound)
{
  int done;
  if (UNLIKELY(!(csound->engineStatus & CS_STATE_COMP))) {
    csound->Warning(csound,
                    Str("Csound not ready for performance: csoundStart() "
                        "has not been called\n"));
    return CSOUND_ERROR;
  }
  done = sensevents(csound);
  if (done || csound->performState == -1) {
    csoundMessage(csound, Str("Score finished in csoundPerformKsmps() with %d.\n"), done);
    return -1;
  } else {
    csound->kperf(csound);
    return 0;
  }
}

// copy/paste from upstream csound-emscripten
// https://github.com/csound/csound/blob/develop/Emscripten/src/CsoundObj.c

#define MIDI_QUEUE_SIZE 1024

struct MidiData {
  unsigned char status;
  unsigned char data1;
  unsigned char data2;
  unsigned char flag;
};

struct MidiCallbackData {
  struct MidiData *midiData;
  int p, q;
};

struct MidiData midiData[MIDI_QUEUE_SIZE];
struct MidiCallbackData midiCallbackData = { midiData, 0, 0 };

void pushMidiMessage(CSOUND *csound, unsigned char status, unsigned char data1, unsigned char data2){
  midiCallbackData.midiData[midiCallbackData.p].status = status;
  midiCallbackData.midiData[midiCallbackData.p].data1 = data1;
  midiCallbackData.midiData[midiCallbackData.p].data2= data2;
  midiCallbackData.midiData[midiCallbackData.p].flag = 1;
  midiCallbackData.p++;
  if (midiCallbackData.p == MIDI_QUEUE_SIZE) {
    midiCallbackData.p = 0;
  }
}

/* used to distinguish between 1 and 2-byte messages */
static const int datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0  };

/* csound MIDI read callback, called every k-cycle */
static int midiDataRead(CSOUND *csound, void *userData, unsigned char *mbuf, int nbytes) {
  struct MidiData *mdata = midiCallbackData.midiData;
  int *q = &midiCallbackData.q, st, d1, d2, n = 0;
  /* check if there is new data on circular queue */
  while (mdata[*q].flag) {
    st = (int) mdata[*q].status;
    d1 = (int) mdata[*q].data1;
    d2 = (int) mdata[*q].data2;
    if (st < 0x80)
      goto next;
    if (st >= 0xF0 &&
        !(st == 0xF8 || st == 0xFA || st == 0xFB ||
          st == 0xFC || st == 0xFF))
      goto next;
    nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
    if (nbytes < 0) break;
    /* write to csound midi buffer */
    n += (datbyts[(st - 0x80) >> 4] + 1);
    switch (datbyts[(st - 0x80) >> 4]) {
    case 0:
      *mbuf++ = (unsigned char) st;
      break;
    case 1:
      *mbuf++ = (unsigned char) st;
      *mbuf++ = (unsigned char) d1;
      break;
    case 2:
      *mbuf++ = (unsigned char) st;
      *mbuf++ = (unsigned char) d1;
      *mbuf++ = (unsigned char) d2;
      break;
    }
  next:
    mdata[*q].flag = 0;
    (*q)++;
    if(*q== MIDI_QUEUE_SIZE) *q = 0;
  }
  /* return the number of bytes read */
  return n;
}

static int midiInOpen(CSOUND *csound, void **userData, const char *dev) {
  return OK;
}

static int midiInClose(CSOUND *csound, void *userData) {
  return OK;
}

void csoundSetMidiCallbacks(CSOUND *csound) {
  csoundSetHostImplementedMIDIIO(csound, 1);
  csoundSetExternalMidiInOpenCallback(csound, midiInOpen);
  csoundSetExternalMidiReadCallback(csound, midiDataRead);
  csoundSetExternalMidiInCloseCallback(csound, midiInClose);
}

// END


// c

// same as csoundCreate but also loads
// opcodes which need initialization to
// be callable (aka static_modules)
CSOUND *csoundCreateWasi() {
  CSOUND *csound = csoundCreate(NULL);
  init_static_modules(csound);
  csoundSetMidiCallbacks(csound);
  scansyn_init_(csound);
  scansynx_init_(csound);
  emugens_init_(csound);
  pvsops_init_(csound);
  liveconv_init_(csound);
  dateops_init_(csound);
  unsupported_opdoces_init_(csound);
  return csound;
}

// same as csoundReset but also loads
// opcodes which need re-initialization to
// be callable (aka static_modules)
void csoundResetWasi(CSOUND *csound) {
  csoundReset(csound);
  init_static_modules(csound);
  csoundSetMidiCallbacks(csound);
  scansyn_init_(csound);
  scansynx_init_(csound);
  emugens_init_(csound);
  pvsops_init_(csound);
  liveconv_init_(csound);
  dateops_init_(csound);
  unsupported_opdoces_init_(csound);
}

int isRequestingRtMidiInput(CSOUND *csound) {
  if (csound->oparms->Midiin || csound->oparms->FMidiin || csound->oparms->RMidiin) {
    return 1;
  } else {
    return 0;
  }
}

char* getRtMidiName(CSOUND *csound) {
  return csound->QueryGlobalVariable(csound, "_RTMIDI");
}

char* getMidiOutFileName(CSOUND *csound) {
  if (csound->oparms->FMidiname == NULL) {
    printf("NIX\n");
    return "\0";
  } else {
    /* char* str = (char*) malloc((100)*sizeof(char)); */
    printf("FMIDINAME %s \n", csound->oparms->FMidiname);
    /* sprintf(str, "%s\n", csound->oparms->FMidiname); */
    /* printf("STR %s \n", str); */
    return csound->oparms->FMidiname;
  }
}

double csoundGetControlChannelWasi(CSOUND* csound, char* channelName) { 
  int *error = NULL;
  double returnValue = csoundGetControlChannel(csound, channelName, error);

//  printf("csoundGetControlChannel: Channel Name %s\n", channelName);
  if (error != NULL) {
    printf("csoundGetControlChannel: Error %d\n", *error);
    return 0;
  } else {
    return returnValue;
  }
}

char* csoundGetStringChannelWasi(CSOUND* csound, const char *channelName) {
  int len = csoundGetChannelDatasize(csound, channelName);
  char *data = calloc(1, sizeof(char) * (len + 1));

  csoundGetStringChannel(csound, channelName, data);
  return data;
}


/* char* csoundGetMidiOptions(CSOUND *csound) { */
/*   csoundGetParams(); */
/* } */

/* #if defined(CSOUND_EXE_WASM) */
/* #else */
// DUMMY MAIN (never called, but is needed)
int main (int argc, char *argv[] ) {}
/* #endif */

// HACK FIX
int __multi3(int a, double b, double c, double d, double e) {
  return 0;
}

int __lttf2(long double a, long double b) {
  return 0;
}
