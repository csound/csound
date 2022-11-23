#include "csound.h"
#include "csoundCore.h"
#include "emugens_common.h"
#include "csound_orc.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
CSOUND_PARAMS* allocCsoundParamsStruct() {
  CSOUND_PARAMS* ptr = NULL;
  ptr = malloc(sizeof(CSOUND_PARAMS));
  return ptr;
}

void freeCsoundParams(CSOUND_PARAMS* ptr) {
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


void freeCsMidiDeviceStruct(CSOUND_PARAMS* ptr) {
  free(ptr);
}

// internal function to determine
// if csound has started in daemon mode
// or without orcCompile*/evalCode* called
__attribute__((used))
int csoundShouldDaemonize(CSOUND *csound) {
  if (csound->oparms->daemon == 1 ||
      (csound->instr0 == NULL && *csound->csdname == '\0')) {
    return 1;
  } else {
    return 0;
  }
}


// END CS_MIDIDEVICE
__attribute__((used))
int csoundStartWasi(CSOUND *csound) {

  csoundMessage(csound, Str("SIZEOF int %d.\n"), sizeof(int));
  csoundMessage(csound, Str("SIZEOF uint64_t %d.\n"), sizeof(uint64_t));
  csoundMessage(csound, Str("SIZEOF TREE %d.\n"), sizeof(TREE));
  csoundMessage(csound, Str("SIZEOF pointer %d.\n"), sizeof(char *));


  const char* outputDev = csoundGetOutputName(csound);

  // detect realtime mode automatically
  if ((strncmp("dac", outputDev, 3) == 0) ||
      csoundShouldDaemonize(csound)) {
    csoundSetHostImplementedAudioIO(csound, 1, 0);
  }
  return csoundStart(csound);
}

extern int sensevents(CSOUND *);

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
    return csound->kperf(csound);
  }
}
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
  csoundSetHostImplementedMIDIIO(csound, 1);
  csoundSetExternalMidiInOpenCallback(csound, midiInOpen);
  csoundSetExternalMidiReadCallback(csound, midiDataRead);
  csoundSetExternalMidiInCloseCallback(csound, midiInClose);
}


// same as csoundCreate but also loads
// opcodes which need initialization to
// be callable (aka static_modules)
__attribute__((used))
CSOUND *csoundCreateWasi() {
  CSOUND *csound = csoundCreate(NULL);
  csoundSetMidiCallbacks(csound);
  return csound;
}

// same as csoundReset but also loads
// opcodes which need re-initialization to
// be callable (aka static_modules)
int csoundResetWasi(CSOUND *csound) {
  int cleanResult = csoundCleanup(csound);
  csoundReset(csound);
  csoundSetMidiCallbacks(csound);
  return cleanResult;
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
    return "\0";
  } else {
    /* char* str = (char*) malloc((100)*sizeof(char)); */
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

/* TOKEN GETTRS */

__attribute__((used))
int valueOf_NEWLINE() {
  return NEWLINE;
}

__attribute__((used))
int valueOf_S_NEQ() {
  return S_NEQ;
}

__attribute__((used))
int valueOf_S_AND() {
  return S_AND;
}

__attribute__((used))
int valueOf_S_OR() {
  return S_OR;
}

__attribute__((used))
int valueOf_S_LT() {
  return S_LT;
}

__attribute__((used))
int valueOf_S_LE() {
  return S_LE;
}

__attribute__((used))
int valueOf_S_EQ() {
  return S_EQ;
}

__attribute__((used))
int valueOf_S_ADDIN() {
  return S_ADDIN;
}

__attribute__((used))
int valueOf_S_SUBIN() {
  return S_SUBIN;
}

__attribute__((used))
int valueOf_S_MULIN() {
  return S_MULIN;
}

__attribute__((used))
int valueOf_S_DIVIN() {
  return S_DIVIN;
}

__attribute__((used))
int valueOf_S_GT() {
  return S_GT;
}

__attribute__((used))
int valueOf_S_GE() {
  return S_GE;
}

__attribute__((used))
int valueOf_S_BITSHIFT_LEFT() {
  return S_BITSHIFT_LEFT;
}

__attribute__((used))
int valueOf_S_BITSHIFT_RRIGHT() {
  return S_BITSHIFT_RRIGHT;
}

__attribute__((used))
int valueOf_LABEL_TOKEN() {
  return LABEL_TOKEN;
}

__attribute__((used))
int valueOf_IF_TOKEN() {
  return IF_TOKEN;
}

__attribute__((used))
int valueOf_DECLARE_TOKEN() {
  return DECLARE_TOKEN;
}

__attribute__((used))
int valueOf_UDO_TOKEN() {
  return UDO_TOKEN;
}

__attribute__((used))
int valueOf_UDOSTART_DEFINITION() {
  return UDOSTART_DEFINITION;
}

__attribute__((used))
int valueOf_UDOEND_TOKEN() {
  return UDOEND_TOKEN;
}

__attribute__((used))
int valueOf_UDO_ANS_TOKEN() {
  return UDO_ANS_TOKEN;
}

__attribute__((used))
int valueOf_UDO_ARGS_TOKEN() {
  return UDO_ARGS_TOKEN;
}

__attribute__((used))
int valueOf_UDO_IDENT() {
  return UDO_IDENT;
}

__attribute__((used))
int valueOf_VOID_TOKEN() {
  return VOID_TOKEN;
}

__attribute__((used))
int valueOf_ERROR_TOKEN() {
  return ERROR_TOKEN;
}

__attribute__((used))
int valueOf_T_OPCALL() {
  return T_OPCALL;
}

__attribute__((used))
int valueOf_T_FUNCTION() {
  return T_FUNCTION;
}

__attribute__((used))
int valueOf_T_ASSIGNMENT() {
  return T_ASSIGNMENT;
}

__attribute__((used))
int valueOf_STRUCT_TOKEN() {
  return STRUCT_TOKEN;
}

__attribute__((used))
int valueOf_INSTR_TOKEN() {
  return INSTR_TOKEN;
}

__attribute__((used))
int valueOf_ENDIN_TOKEN() {
  return ENDIN_TOKEN;
}

__attribute__((used))
int valueOf_GOTO_TOKEN() {
  return GOTO_TOKEN;
}

__attribute__((used))
int valueOf_KGOTO_TOKEN() {
  return KGOTO_TOKEN;
}

__attribute__((used))
int valueOf_IGOTO_TOKEN() {
  return IGOTO_TOKEN;
}

__attribute__((used))
int valueOf_STRING_TOKEN() {
  return STRING_TOKEN;
}

__attribute__((used))
int valueOf_T_IDENT() {
  return T_IDENT;
}

__attribute__((used))
int valueOf_T_TYPED_IDENT() {
  return T_TYPED_IDENT;
}

__attribute__((used))
int valueOf_T_PLUS_IDENT() {
  return T_PLUS_IDENT;
}

__attribute__((used))
int valueOf_INTEGER_TOKEN() {
  return INTEGER_TOKEN;
}

__attribute__((used))
int valueOf_NUMBER_TOKEN() {
  return NUMBER_TOKEN;
}

__attribute__((used))
int valueOf_THEN_TOKEN() {
  return THEN_TOKEN;
}

__attribute__((used))
int valueOf_ITHEN_TOKEN() {
  return ITHEN_TOKEN;
}

__attribute__((used))
int valueOf_KTHEN_TOKEN() {
  return KTHEN_TOKEN;
}

__attribute__((used))
int valueOf_ELSEIF_TOKEN() {
  return ELSEIF_TOKEN;
}

__attribute__((used))
int valueOf_ELSE_TOKEN() {
  return ELSE_TOKEN;
}

__attribute__((used))
int valueOf_ENDIF_TOKEN() {
  return ENDIF_TOKEN;
}

__attribute__((used))
int valueOf_UNTIL_TOKEN() {
  return UNTIL_TOKEN;
}

__attribute__((used))
int valueOf_WHILE_TOKEN() {
  return WHILE_TOKEN;
}

__attribute__((used))
int valueOf_DO_TOKEN() {
  return DO_TOKEN;
}

__attribute__((used))
int valueOf_OD_TOKEN() {
  return OD_TOKEN;
}

__attribute__((used))
int valueOf_S_ELIPSIS() {
  return S_ELIPSIS;
}

__attribute__((used))
int valueOf_T_ARRAY() {
  return T_ARRAY;
}

__attribute__((used))
int valueOf_T_ARRAY_IDENT() {
  return T_ARRAY_IDENT;
}

__attribute__((used))
int valueOf_T_DECLARE() {
  return T_DECLARE;
}

__attribute__((used))
int valueOf_STRUCT_EXPR() {
  return STRUCT_EXPR;
}

__attribute__((used))
int valueOf_T_MAPI() {
  return T_MAPI;
}

__attribute__((used))
int valueOf_T_MAPK() {
  return T_MAPK;
}


/* STRUCT GETTRS */

__attribute__((used))
int treeStructGetType(TREE* tree) {
  return tree->type;
}

__attribute__((used))
ORCTOKEN* treeStructGetValuePtr(TREE* tree) {
  return tree->value;
}

__attribute__((used))
int treeStructGetRate(TREE* tree) {
  return tree->rate;
}

__attribute__((used))
int treeStructGetLen(TREE* tree) {
  return tree->len;
}

__attribute__((used))
int treeStructGetLine(TREE* tree) {
  return tree->line;
}

__attribute__((used))
uint64_t treeStructGetLocn(TREE* tree) {
  return tree->locn;
}

__attribute__((used))
TREE* treeStructGetLeftPtr(TREE* tree) {
  return tree->left;
}

__attribute__((used))
TREE* treeStructGetRightPtr(TREE* tree) {
  return tree->right;
}

__attribute__((used))
TREE* treeStructGetNextPtr(TREE* tree) {
  return tree->next;
}

__attribute__((used))
int orctokenGetType(ORCTOKEN* t) {
  return t->type;
}

__attribute__((used))
char* orctokenGetLexeme(ORCTOKEN* t) {
  return t->lexeme;
}

__attribute__((used))
int orctokenGetValue(ORCTOKEN* t) {
  return t->value;
}

__attribute__((used))
double orctokenGetFvalue(ORCTOKEN* t) {
  return t->fvalue;
}

__attribute__((used))
char* orctokenGetOptype(ORCTOKEN* t) {
  return t->optype;
}

__attribute__((used))
ORCTOKEN* orctokenGetNext(ORCTOKEN* t) {
  return t->next;
}

extern size_t __heap_base;

// DUMMY MAIN (never called, but is needed)
int main (int argc, char *argv[] ) {}


// Compilation fix for unsupported functions defined
// wasi-libc/expected/wasm32-wasi/undefined-symbols.txt

int vsiprintf(char *restrict s, const char *restrict fmt, va_list ap) {
	return vsnprintf(s, INT_MAX, fmt, ap);
}

int __small_vsprintf(char *restrict s, const char *restrict fmt, va_list ap) {
  	return vsnprintf(s, INT_MAX, fmt, ap);
}

int siprintf(char *restrict s, const char *restrict fmt, ...) {
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = vsiprintf(s, fmt, ap);
	va_end(ap);
	return ret;
}

int __small_sprintf(char *restrict s, const char *restrict fmt, ...) {
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = __small_vsprintf(s, fmt, ap);
	va_end(ap);
	return ret;
}


int32_t fiprintf(int32_t x, int32_t y, int32_t z) {
  printf("ERROR: call to unsupported function fiprintf");
  return 0;
}

int32_t __small_fprintf(int32_t x, int32_t y, int32_t z) {
  printf("ERROR: call to unsupported function __small_fprintf");
  return 0;
}

int32_t __getf2(int64_t x, int64_t y, int64_t z, int64_t zz) {
  if (x > y) {
    return 1;
  } else if (x == y) {
    return 0;
  } else {
    return -1;
  }
}

int32_t __netf2(int64_t b, int64_t c, int64_t d, int64_t e) {
  return __getf2(b, c, d, e);
}

int32_t __gttf2(int64_t b, int64_t c, int64_t d, int64_t e) {
  return __getf2(b, c, d, e);
}

void __extenddftf2(int32_t x, double y) {}

void __multi3(int32_t a, int64_t b, int64_t c, int64_t d, int64_t e) {}

void __muloti4(int32_t a, int64_t b, int64_t c, int64_t d, int64_t d_, int32_t e) {}

int __lttf2(long double a, long double b) {
  if (a > b) {
    return 1;
  } else if (a == b) {
    return 0;
  } else {
    return -1;
  }
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
