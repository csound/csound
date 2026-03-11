#include "csound.h"
#include "csound_misc.h"
#include "ugen.h"
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Define constants that were previously from internal headers
#define MAX_MESSAGE_STR 4096
#define OK 0

// Forward declaration for printDebug function
void printDebug(const char *log);

// returns the address of a string
// pointer which is writable from js
__attribute__((used))
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


__attribute__((used))
double* allocFloatArray(int length) {
  double *ptr = NULL;
  ptr = malloc(length * sizeof(double));
  return ptr;
}

void freeFloatArrayMem(double* ptr) {
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

void freeCsMidiDeviceStruct(CS_MIDIDEVICE* ptr) {
  free(ptr);
}

// internal function to determine
// if csound has started in daemon mode
// or without orcCompile*/evalCode* called
__attribute__((used))
int csoundShouldDaemonize(CSOUND *csound) {
  // For WASI, we'll use a simplified check
  // Since some API functions might not be available, we'll assume daemon mode is not enabled
  return 0;
}


// END CS_MIDIDEVICE
__attribute__((used))
int csoundStartWasi(CSOUND *csound) {

  // For WASI, we'll use a simplified approach
  // Since csoundGetOutputName might not be available, we'll skip the device check
  return csoundStart(csound);
}

extern int sense_events(CSOUND *);


// for message callbacks, probably we don't want this for non-js
// wasm interpretors

void csoundWasiJsMessageCallback(
    CSOUND *csound,
    int attr,
    int len,
    const char *str
) __attribute__((
     used,
    __import_module__("env"),
    __import_name__("csoundWasiJsMessageCallback")
));


void csoundWasiCMessageCallback(CSOUND *csound, int attr, const char *format, va_list args) {
  char buffer[MAX_MESSAGE_STR];
  int len = vsnprintf(buffer, MAX_MESSAGE_STR, format, args);
  (* csoundWasiJsMessageCallback)(csound, attr, len, buffer);
}

__attribute__((used))
void __wasi_js_csoundSetMessageStringCallback() {
  return csoundSetDefaultMessageCallback(&csoundWasiCMessageCallback);
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

__attribute__((used))
void csoundSetMidiCallbacks(CSOUND *csound) {
  csoundSetHostMIDIIO(csound);
  csoundSetExternalMidiInOpenCallback(csound, midiInOpen);
  csoundSetExternalMidiReadCallback(csound, midiDataRead);
  csoundSetExternalMidiInCloseCallback(csound, midiInClose);
}


// same as csoundCreate but also loads
// opcodes which need initialization to
// be callable (aka static_modules)
__attribute__((used))
CSOUND *csoundCreateWasi() {
  CSOUND *csound = csoundCreate(NULL, NULL);
  // printDebug("DEBUG: csoundCreateWasi called, setting message callback");
  csoundSetMessageCallback(csound, &csoundWasiCMessageCallback);
  csoundSetMidiCallbacks(csound);
  return csound;
}

// same as csoundReset but also loads
// opcodes which need re-initialization to
// be callable (aka static_modules)
// NB: csoundCleanup has been removed from API for 7.0
int csoundResetWasi(CSOUND *csound) {
  csoundReset(csound);
  csoundSetMidiCallbacks(csound);
  return CSOUND_SUCCESS;
}

// Keep a stable non-null pointer for JS string reads.
static char emptyString[1] = { 0 };

__attribute__((used))
int isRequestingRtMidiInput(CSOUND *csound) {
  const OPARMS *params = csoundGetParams(csound);
  if (params == NULL) {
    return 0;
  }

  if (params->Midiin || params->FMidiin || params->RMidiin) {
    return 1;
  } else {
    return 0;
  }
}

__attribute__((used))
char* getRtMidiName(CSOUND *csound) {
  char *name = (char *) csoundQueryGlobalVariable(csound, "_RTMIDI");
  return name == NULL ? emptyString : name;
}

__attribute__((used))
char* getMidiOutFileName(CSOUND *csound) {
  const OPARMS *params = csoundGetParams(csound);
  if (params == NULL || params->FMidiname == NULL) {
    return emptyString;
  } else {
    return params->FMidiname;
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

// extern size_t __heap_base;

// DUMMY MAIN (never called, but is needed)
int main (int argc, char *argv[] ) {}


// Compilation fix for unsupported functions defined
// wasi-libc/expected/wasm32-wasi/undefined-symbols.txt

// int vsiprintf(char *restrict s, const char *restrict fmt, va_list ap) {
// 	return vsnprintf(s, INT_MAX, fmt, ap);
// }

// int __small_vsprintf(char *restrict s, const char *restrict fmt, va_list ap) {
//   	return vsnprintf(s, INT_MAX, fmt, ap);
// }

// int siprintf(char *restrict s, const char *restrict fmt, ...) {
// 	int ret;
// 	va_list ap;
// 	va_start(ap, fmt);
// 	ret = vsiprintf(s, fmt, ap);
// 	va_end(ap);
// 	return ret;
// }

// int __small_sprintf(char *restrict s, const char *restrict fmt, ...) {
// 	int ret;
// 	va_list ap;
// 	va_start(ap, fmt);
// 	ret = __small_vsprintf(s, fmt, ap);
// 	va_end(ap);
// 	return ret;
// }


// int32_t fiprintf(int32_t x, int32_t y, int32_t z) {
//   printf("ERROR: call to unsupported function fiprintf");
//   return 0;
// }

// int32_t __small_fprintf(int32_t x, int32_t y, int32_t z) {
//   printf("ERROR: call to unsupported function __small_fprintf");
//   return 0;
// }

// int32_t __getf2(int64_t x, int64_t y, int64_t z, int64_t zz) {
//   if (x > y) {
//     return 1;
//   } else if (x == y) {
//     return 0;
//   } else {
//     return -1;
//   }
// }

// int32_t __netf2(int64_t b, int64_t c, int64_t d, int64_t e) {
//   return __getf2(b, c, d, e);
// }

// int32_t __gttf2(int64_t b, int64_t c, int64_t d, int64_t e) {
//   return __getf2(b, c, d, e);
// }

// void __extenddftf2(int32_t x, double y) {}

// void __multi3(int32_t a, int64_t b, int64_t c, int64_t d, int64_t e) {}

// void __muloti4(int32_t a, int64_t b, int64_t c, int64_t d, int64_t d_, int32_t e) {}

// int __lttf2(long double a, long double b) {
//   if (a > b) {
//     return 1;
//   } else if (a == b) {
//     return 0;
//   } else {
//     return -1;
//   }
// }

// ==== UGEN helpers for WASM/JS ====

/**
 * Returns the raw data pointer for a UGEN_VAR so JS can create
 * a Float64Array view over wasm memory.
 * JS usage: new Float64Array(memory.buffer, ptr, size / 8)
 */
__attribute__((used))
void* csoundUgenVarGetDataAsFloat64Array(UGEN_VAR* var) {
  return csoundUgenVarGetData(var);
}

/**
 * Returns the ksmps value used by this UGEN_VAR.
 * For a-rate vars, the data buffer contains ksmps MYFLTs.
 * For i/k vars this still returns the factory ksmps (useful for
 * knowing the audio block size).
 */
__attribute__((used))
int32_t csoundUgenVarGetKsmps(UGEN_VAR* var) {
  if (var == NULL) return 0;
  // Access the ksmps field from the UGEN_VAR struct.
  // We need the internal header for this, but since we include ugen.h
  // (opaque types only), we use csoundUgenVarGetSize and type to infer:
  UGEN_ARG_TYPE type = csoundUgenVarGetType(var);
  size_t size = csoundUgenVarGetSize(var);
  if (type == UGEN_ARG_TYPE_A) {
    // a-rate: size = ksmps * sizeof(MYFLT)
    return (int32_t)(size / sizeof(MYFLT));
  }
  // For i/k/S/F types, return 1 (scalar)
  return 1;
}

void printDebugCallback(
    const char *str,
    int len
) __attribute__((
     used,
    __import_module__("env"),
    __import_name__("printDebugCallback")
));

__attribute__((used))
void printDebug(const char *log) {
  if (strlen(log) > 0) {
    (* printDebugCallback)(log, strlen(log));
  }
}


// WASI setjmp/longjmp stubs for the linker
// These are weak symbols to allow the linker to resolve them
// Function signatures match what's expected by the linker
__attribute__((weak))
void __wasm_setjmp(int a, int b, void *env) {
  // WASI doesn't support setjmp, return failure
  (void)a; (void)b; // Suppress unused parameter warnings
}

__attribute__((weak))
int __wasm_setjmp_test(int a, void *env) {
  // WASI doesn't support setjmp, return failure
  (void)a; // Suppress unused parameter warning
  return -1;
}

__attribute__((weak))
void __wasm_longjmp(void *env, int val) {
  // Report through JS host so callers can catch a normal exception instead
  // of aborting the entire WASI instance via proc_exit.
  char message[64];
  (void) env;
  snprintf(message, sizeof(message), "CSOUND_WASI_LONGJMP:%d", val);
  printDebug(message);
  __builtin_trap();
}
