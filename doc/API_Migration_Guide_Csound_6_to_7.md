# Csound Host API Migration Guide

This document outlines the most salient issues
regarding the migration of Csound API code
from 6.x to 7.0. It does not cover new functionality
or interfaces in 7.0, except where they relate to or
replace the ones in 6.x.

## General Remarks

The changes in the API have been motivated by two
goals:

-  Simplification
-  Consolidation

As part of the work to achieve simplification, unused or
little-used parts of the API have been removed. Other less
commonly used components were placed in separate header
files. In terms of consolidation, we have mostly avoided
duplication of functionality and similar-but-not-quite
interfaces. In Csound 6.x, a separate library, *libcsnd*,
existed, mainly to support the wrapping of the C API in
other languages, plus some little used extra functionality.
This has been removed, and the C++ `CsoundPerformanceThread`
class was incorporated into the main Csound library.
A C-languageinterface for this class is provided as part of the
public headers.

## Main Changes in `csound.h`

The following is an outline of the changes in the main
header file, `csound.h`.


```
PUBLIC void csoundGetAPIVersion(csound);
```

This function has been removed. Semantic versioning only available now
through `csoundGetVersion()`.

```
PUBLIC void csoundSetOpcodedir(const char *s);
```

This function has been removed. The functionality has
been consolidated into `csoundCreate()`.

```
PUBLIC CSOUND *csoundCreate(void *hostData);
```

This function now takes two parameters:

```
PUBLIC CSOUND *csoundCreate(void *hostData, const char *opcodedir);
```

where `opcodedir`is an opcode directory override.

```
PUBLIC TREE *csoundParseOrc(CSOUND *csound, const char *str);
PUBLIC void csoundDeleteTree(CSOUND *csound, TREE *tree);  
```

These functions were moved to `csound_compiler.h`

```
PUBLIC int csoundCompileTree(CSOUND *csound, TREE *root);
PUBLIC int csoundCompileTreeAsync(CSOUND *csound, TREE *root)
```

These functions were replaced by the function 

```
PUBLIC int csoundCompileTree(CSOUND *csound, TREE *root, int async);
```

in `csound_compiler.h`.

The functions

```
PUBLIC int csoundCompileOrc(CSOUND *csound, const char *str);
PUBLIC int csoundCompileOrcAsync(CSOUND *csound, const char *str);
```

were replaced by

```
PUBLIC int csoundCompileOrc(CSOUND *csound, const char *str, int async);
```

This function was removed, as we have discontinued support for
Cscore in the API. 

```
PUBLIC int csoundInitializeCscore(CSOUND *, FILE *insco, FILE *outsco);
```

The function 

```
PUBLIC int csoundCompileArgs(CSOUND *, int argc, const char **argv);
```
was removed as it duplicates the functionality of `csoundCompile()`.

The function

```
PUBLIC int csoundStart(CSOUND *csound);
```

is now required to be called in all cases to start the Csound engine.
In Csound 6.x, `csoundCompile()` called this internally, but that
behaviour is discontinued.

```
PUBLIC int csoundCompileCsd(CSOUND *csound, const char *csd_filename);
PUBLIC int csoundCompileCsdText(CSOUND *csound, const char *csd_text);
```

These functions have been replaced by

```
PUBLIC int csoundCompileCSD(CSOUND *csound, const char *csd, int mode);
```

Note the all uppercase letters in `CSD`. The mode parameter indicates whether
the `csd` parameter is a filename or a code string.

```
PUBLIC int csoundPerform(CSOUND *);
```

This function has been removed, it duplicates the functionality of `csoundPerformKsmps()`
called inside a loop.

```
PUBLIC int csoundPerformBuffer(CSOUND *);
```

This function has been removed since buffer-level processing granularity was never found to be useful.

```
csoundStop(CSOUND *)
```

This function has been removed as it was only used to stop the processing inside `csoundPerform()`.

```
PUBLIC int csoundCleanup(CSOUND *);
```

This function has been removed as cleanup can be achieved with `csoundReset()` or `csoundDestroy()`.


```
PUBLIC int csoundUDPServerStart(CSOUND *csound, unsigned int port);
PUBLIC int csoundUDPServerStatus(CSOUND *csound);
PUBLIC int csoundUDPServerClose(CSOUND *csound);
PUBLIC int csoundUDPConsole(CSOUND *csound, const char *addr,
                              int port, int mirror);
PUBLIC void csoundStopUDPConsole(CSOUND *csound);
```

These functions have been moved to `csound_server.h`

```
PUBLIC uint32_t csoundGetNchnls(CSOUND *);
PUBLIC uint32_t csoundGetNchnlsInput(CSOUND *csound);
```

These functions have been replaced by

```
PUBLIC uint32_t csoundGetChannels(CSOUND *, int isInput);
```

The functions

```
PUBLIC void csoundSetParameters(CSOUND *, CSOUND_PARAMETERS *p);
PUBLIC void csoundGetParameters(CSOUND *, CSOUND_PARAMETERS *p);
```

have been removed and replaced by`

```
PUBLIC const OPARMS *csoundGetParams(CSOUND *csound);
```

Note that configuration parameters can be modified via
command-line options, CsOptions, or `csoundSetOption()`.

```
PUBLIC int csoundSetOption(CSOUND *csound, const char *option);
```

This function can now take any number of options, in the command-line
arguments format, and can include spaces.

```
PUBLIC void csoundSetOutput(CSOUND *csound, const char *name,
                              const char *type, const char *format);
PUBLIC void csoundGetOutputFormat(CSOUND *csound,char *type,
                                    char *format);
PUBLIC void csoundSetInput(CSOUND *csound, const char *name);
PUBLIC void csoundSetMIDIInput(CSOUND *csound, const char *name);
PUBLIC void csoundSetMIDIFileInput(CSOUND *csound, const char *name);
PUBLIC void csoundSetMIDIOutput(CSOUND *csound, const char *name);
PUBLIC void csoundSetMIDIFileOutput(CSOUND *csound, const char *name);
```

These functions have been removed as they duplicate functionality
provided by csound options.

```
PUBLIC void csoundSetFileOpenCallback(CSOUND *p,
                                        void (*func)(CSOUND*, const char*,
                                        int, int, int));
```

This function has been placed in the `csound_files.h` header.


```
PUBLIC long csoundGetInputBufferSize(CSOUND *);
PUBLIC long csoundGetOutputBufferSize(CSOUND *);
PUBLIC MYFLT *csoundGetInputBuffer(CSOUND *);
PUBLIC MYFLT *csoundGetOutputBuffer(CSOUND *);
```

The function

```
PUBLIC MYFLT *csoundGetSpout(CSOUND *csound)
```

has changed signature to

```
PUBLIC const MYFLT *csoundGetSpout(CSOUND *csound)
```

These functions have been removed as there is no buffer-level
processing in the API anymore. Access is now limited to spin
and spout buffers.

```
PUBLIC void csoundAddSpinSample(CSOUND *csound,
                                  int frame, int channel, MYFLT sample);
PUBLIC void csoundSetSpinSample(CSOUND *csound,
                                  int frame, int channel, MYFLT sample);
```
These functions duplicate functionality and have been removed.
Access to spin/spout data is done by acquiring the pointers
and access the vectors directly.

```
PUBLIC void **csoundGetRtRecordUserData(CSOUND *);
PUBLIC void **csoundGetRtPlayUserData(CSOUND *);
```

Audio IO module registration has been removed from the
API and so these functions have also been removed as they
have no use.

```
PUBLIC void csoundSetHostImplementedAudioIO(CSOUND *,
int state, int bufSize);
```

This function was replaced by

```
PUBLIC void csoundSetHostAudioIO(CSOUND *);
```

These functions have been removed:

```
PUBLIC void
  csoundSetPlayopenCallback(CSOUND *,
                            int (*playopen__)(CSOUND *,
                                              const csRtAudioParams *parm));
PUBLIC void csoundSetRtplayCallback(CSOUND *,
                                      void (*rtplay__)(CSOUND *,
                                                       const MYFLT *outBuf,
                                                       int nbytes));
PUBLIC void csoundSetRecopenCallback(CSOUND *,
                                     int (*recopen_)(CSOUND *,
                                                     const csRtAudioParams *parm));
PUBLIC void csoundSetRtrecordCallback(CSOUND *,
                                        int (*rtrecord__)(CSOUND *,
                                                          MYFLT *inBuf,
                                                          int nbytes));
PUBLIC void csoundSetRtcloseCallback(CSOUND *, void (*rtclose__)(CSOUND *));
PUBLIC void csoundSetAudioDeviceListCallback(CSOUND *csound,
                                               int (*audiodevlist__)(CSOUND *,
                                                   CS_AUDIODEVICE *list,
                                                   int isOutput));
```

Registration of audio modules via the API has been
discontinued. Audio modules
are now only added using the module API.

```
PUBLIC void csoundSetHostImplementedMIDIIO(CSOUND *csound,
  int state);
```
This function has been replaced by

```
PUBLIC void csoundSetHostMIDIIO(CSOUND *csound);
```

```
PUBLIC int csoundReadScore(CSOUND *csound, const char *str);
PUBLIC void csoundReadScoreAsync(CSOUND *csound, const char *str);
PUBLIC void csoundInputMessage(CSOUND *, const char *message);
PUBLIC void csoundInputMessageAsync(CSOUND *, const char *message);
```

These functions have been replaced by

```
PUBLIC int csoundEventString(CSOUND *csound, const char *str, int async);
```

These functions:

```
PUBLIC int csoundScoreEvent(CSOUND *,
                              char type, const MYFLT *pFields, long numFields);
PUBLIC void csoundScoreEventAsync(CSOUND *,
                              char type, const MYFLT *pFields, long numFields);
PUBLIC int csoundScoreEventAbsolute(CSOUND *,
                 char type, const MYFLT *pfields, long numFields, double time_ofs);
PUBLIC void csoundScoreEventAbsoluteAsync(CSOUND *,
                 char type, const MYFLT *pfields, long numFields, double time_ofs);
```

have been replaced by

```
PUBLIC int csoundEvent(CSOUND *csound, int type, const MYFLT *pfields,
long numFields, int async);
```

where type is one of

```
  enum {
    CS_INSTR_EVENT = 0,
    CS_TABLE_EVENT,
    CS_END_EVENT
    };
```

The function

```
PUBLIC int csoundGetChannelPtr(CSOUND *,
                                 MYFLT **p, const char *name, int type);
```

has changed signature to

```
PUBLIC int csoundGetChannelPtr(CSOUND *,
                                 void **p, const char *name, int type);
```

The function

```
PUBLIC int *csoundGetChannelLock(CSOUND *, const char *name);
```

has been removed and was replaced by

```
PUBLIC void csoundLockChannel(CSOUND *csound, const char *channel);
PUBLIC void csoundUnlockChannel(CSOUND *csound, const char *channel);
```

The function

```
PUBLIC int csoundKillInstance(CSOUND *csound, MYFLT instr,
                                char *instrName, int mode, int allow_release);
```

was removed. An event or csound code can be used for this.

```
PUBLIC int csoundRegisterSenseEventCallback(CSOUND *,
                                              void (*func)(CSOUND *, void *),
                                              void *userData);
```

This function was removed, with `csoundPerform()` removed, this callback
has lost its purpose.


```
PUBLIC MYFLT csoundTableGet(CSOUND *, int table, int index);
PUBLIC void csoundTableSet(CSOUND *, int table, int index, MYFLT value);
PUBLIC void csoundTableCopyOut(CSOUND *csound, int table, MYFLT *dest);
PUBLIC void csoundTableCopyOutAsync(CSOUND *csound, int table, MYFLT *dest);
PUBLIC void csoundTableCopyIn(CSOUND *csound, int table, MYFLT *src);
PUBLIC void csoundTableCopyInAsync(CSOUND *csound, int table, MYFLT *src);
PUBLIC void csoundGetNamedGEN(CSOUND *csound, int num, char *name, int len);
PUBLIC int csoundIsNamedGEN(CSOUND *csound, int num);
```

These functions duplicated `csoundGetTable()` and so have been removed.

```
PUBLIC int csoundSetIsGraphable(CSOUND *, int isGraphable);
PUBLIC void csoundSetMakeGraphCallback(CSOUND *,
                           void (*makeGraphCallback_)(CSOUND *,
                                                      WINDAT *windat,
                                                      const char *name));
PUBLIC void csoundSetDrawGraphCallback(CSOUND *,
      void (*drawGraphCallback_)(CSOUND *,WINDAT *windat));

PUBLIC void csoundSetKillGraphCallback(CSOUND *,
       void (*killGraphCallback_)(CSOUND *, WINDAT *windat));
PUBLIC void csoundSetExitGraphCallback(CSOUND *,
                                         int (*exitGraphCallback_)(CSOUND *));
```

These functions have been moved to `csound_graph_displays.h`.

```
PUBLIC int csoundNewOpcodeList(CSOUND *, opcodeListEntry **opcodelist);
PUBLIC void csoundDisposeOpcodeList(CSOUND *, opcodeListEntry *opcodelist);
```

These functions have been removed.


```
PUBLIC int csoundAppendOpcode(CSOUND *, const char *opname,
                                int dsblksiz, int flags, int thread,
                                const char *outypes, const char *intypes,
                                int (*iopadr)(CSOUND *, void *),
                                int (*kopadr)(CSOUND *, void *),
                                int (*aopadr)(CSOUND *, void *));
```

The signature of this function has changed to:

```
PUBLIC int csoundAppendOpcode (CSOUND *, const char *opname,
                                 int dsblksiz, int flags,
                                 const char *outypes, const char *intypes,
                                 int(*init)(CSOUND *, void *),
                                 int(*perf)(CSOUND *, void *),
                                 int(*deinit)(CSOUND *, void *));
```

Note the removal of the `thread` argument.

```
PUBLIC void csoundSetYieldCallback(CSOUND *, int (*yieldCallback_)(CSOUND *));
```

This function was removed, with `csoundPerform()` removed, this callback
has lost its purpose.

All functions under the *threading and concurrency* group have been
moved to `csound_threads.h`.

All functions under the *miscellaneous* group have been
moved to `csound_misc.h`.

All cicular buffer functions have been moved to `csound_circular_buffer.h`.

The header file `csound_threaded.hpp` has been removed as it 
duplicates (in a limited way) multithreading that is now built into
the Csound library. This is exposed in `csPerfThread.h` and `csPerfThread.hpp`.
Functions in `csound.h` are wrapped in the Csound class in
`csound.hpp` and so the changes listed above have been propagated
to that interface.



  

