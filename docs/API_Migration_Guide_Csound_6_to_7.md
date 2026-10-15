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
A C-language interface for this class is provided as part of the
public headers.

## Numeric types

Use `cs_float` in place of `MYFLT`. Csound 7 keeps `MYFLT` as a deprecated
alias, so old host and opcode code remains valid. `cs_double` names values
that normally use double precision. The new `USE_FLOAT` build option makes
both types single precision. See [Numeric types](numeric-types.md) for the
build options, helper aliases, and binding changes.

The change is not limited to typedefs: the public headers now use `cs_float`
in every declaration that used `MYFLT` in Csound 6.x, and `MYFLT` otherwise
remains only as an alias in `csound_types.h`. For example, the function

```
PUBLIC MYFLT *csoundGetSpin(CSOUND *);
```

is now

```
PUBLIC cs_float *csoundGetSpin(CSOUND *);
```

The same applies to the buffers and data passed to the API, such as
`csoundGetSpout()`, `csoundTableCopyIn()` and `csoundTableCopyOut()`, and
the table data returned by `csoundGetTable()`. Throughout this guide, the
Csound 6.x declarations are shown with `MYFLT` and the Csound 7
replacements with `cs_float`.

Values that Csound 6.x declared as `double` now use `cs_double`, for
example

```
PUBLIC double csoundGetRealTime(RTCLOCK *);
PUBLIC double csoundGetCPUTime(RTCLOCK *);
PUBLIC double csoundGetScoreTime(CSOUND *);
```

are now

```
PUBLIC cs_double csoundGetRealTime(RTCLOCK *);
PUBLIC cs_double csoundGetCPUTime(RTCLOCK *);
PUBLIC cs_double csoundGetScoreTime(CSOUND *);
```

as are the `OPARMS` fields `cmdTempo` and `quality`. These are type-name
changes only; C's promotion rules, floating-point literals, and external
library interfaces are unchanged.

## Main Changes in `csound.h`

The following is an outline of the changes in the main
header file, `csound.h`.


```
PUBLIC int csoundGetAPIVersion(void);
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

where `opcodedir` is an opcode directory override.

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
PUBLIC int32_t csoundCompileTree(CSOUND *csound, TREE *root, int32_t async);
```

in `csound_compiler.h`.

The functions

```
PUBLIC int csoundCompileOrc(CSOUND *csound, const char *str);
PUBLIC int csoundCompileOrcAsync(CSOUND *csound, const char *str);
```

were replaced by

```
PUBLIC int32_t csoundCompileOrc(CSOUND *csound, const char *str, int32_t async);
```

This function was removed, as we have discontinued support for
Cscore in the API. 

```
PUBLIC int csoundInitializeCscore(CSOUND *, FILE *insco, FILE *outsco);
```

The callback version has been removed for the same reason:

```
PUBLIC void csoundSetCscoreCallback(CSOUND *,
                                    void (*cscoreCallback_)(CSOUND *));
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
PUBLIC int32_t csoundCompileCSD(CSOUND *csound, const char *csd, int32_t mode,
                                int32_t async);
```

Note the all uppercase letters in `CSD`. The mode parameter is `0` for a
filename or `1` for a CSD code string. Pass `0` for `async` to compile
synchronously, or `1` to compile asynchronously.

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
PUBLIC int32_t csoundUDPServerStart(CSOUND *csound, uint32_t port);
PUBLIC int32_t csoundUDPServerStatus(CSOUND *csound);
PUBLIC int32_t csoundUDPServerClose(CSOUND *csound);
PUBLIC int32_t csoundUDPConsole(CSOUND *csound, const char *addr,
                              int32_t port, int32_t mirror);
PUBLIC void csoundStopUDPConsole(CSOUND *csound);
```

These functions have been moved to `csound_server.h`

```
PUBLIC uint32_t csoundGetNchnls(CSOUND *);
PUBLIC uint32_t csoundGetNchnlsInput(CSOUND *csound);
```

These functions have been replaced by

```
PUBLIC uint32_t csoundGetChannels(CSOUND *, int32_t isInput);
```

The functions

```
PUBLIC void csoundSetParams(CSOUND *csound, CSOUND_PARAMS *p);
PUBLIC void csoundGetParams(CSOUND *csound, CSOUND_PARAMS *p);
```

have been removed. `csoundGetParams()` remains, with a changed
signature:

```
PUBLIC const OPARMS *csoundGetParams(CSOUND *csound);
```

Note that configuration parameters can be modified via
command-line options, CsOptions, or `csoundSetOption()`.

```
PUBLIC int32_t csoundSetOption(CSOUND *csound, const char *option);
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
                                        int32_t, int32_t, int32_t));
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
PUBLIC const cs_float *csoundGetSpout(CSOUND *csound)
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
PUBLIC void csoundClearSpin(CSOUND *);
PUBLIC MYFLT csoundGetSpoutSample(CSOUND *csound, int frame, int channel);
```

These functions have been removed as well. The engine clears the
spout buffer at the start of every k-cycle, but not the spin buffer;
if your host does not write the whole spin buffer each k-cycle, zero
it first with `memset()`, using the pointer returned by
`csoundGetSpin()`. Read individual spout samples through the pointer
returned by `csoundGetSpout()`.

```
PUBLIC void **csoundGetRtRecordUserData(CSOUND *);
PUBLIC void **csoundGetRtPlayUserData(CSOUND *);
```

These functions have been moved to the `csound_rtaudio.h` header,
where audio back ends continue to use them to store per-stream state.

```
PUBLIC void csoundSetHostImplementedAudioIO(CSOUND *,
int state, int bufSize);
```

This function was replaced by

```
PUBLIC void csoundSetHostAudioIO(CSOUND *);
```

These functions have been moved to the `csound_rtaudio.h` header:

```
PUBLIC void
  csoundSetPlayopenCallback(CSOUND *,
                            int32_t (*playopen__)(CSOUND *,
                                              const csRtAudioParams *parm));
PUBLIC void csoundSetRtplayCallback(CSOUND *,
                                      void (*rtplay__)(CSOUND *,
                                                       const cs_float *outBuf,
                                                       int32_t nbytes));
PUBLIC void csoundSetRecopenCallback(CSOUND *,
                                     int32_t (*recopen__)(CSOUND *,
                                                     const csRtAudioParams *parm));
PUBLIC void csoundSetRtrecordCallback(CSOUND *,
                                        int32_t (*rtrecord__)(CSOUND *,
                                                          cs_float *inBuf,
                                                          int32_t nbytes));
PUBLIC void csoundSetRtcloseCallback(CSOUND *, void (*rtclose__)(CSOUND *));
PUBLIC void csoundSetAudioDeviceListCallback(CSOUND *csound,
                                               int32_t (*audiodevlist__)(CSOUND *,
                                                   CS_AUDIODEVICE *list,
                                                   int32_t isOutput));
```

They remain available to hosts and audio back ends that supply
real-time audio callbacks. The audio module to use is selected by
name with `csoundSetRTAudioModule()`, also in `csound_rtaudio.h`.

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
PUBLIC void csoundEventString(CSOUND *csound, const char *str, int32_t async);
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
PUBLIC void csoundEvent(CSOUND *csound, int32_t type, const cs_float *pfields,
                       int32_t numFields, int32_t async);
```

where type is one of

```
  enum {
    CS_INSTR_EVENT = 0,
    CS_TABLE_EVENT,
    CS_END_EVENT,
    CS_ADV_EVENT
    };
```

During performance, both `csoundEvent()` and `csoundEventString()`
treat the start time as relative to the current time: the value is
added to the current sample position when the event is inserted. A
string such as `"i 1 4.0 1.0"` therefore starts the note 4.0 seconds
from now, not at score time 4.0. Before performance starts, the
current position is zero and the usual absolute score time applies.

There is no absolute-time event function in 7.0.
`csoundScoreEventAbsolute()` was removed and its implementation is not
exposed. Hosts that relied on it must convert the target time to a
delay, for example `targetTime - csoundGetScoreTime(csound)` when
submitting synchronously between k-periods, and decide how to treat a
target time that has already passed.

The function

```
PUBLIC int csoundGetChannelPtr(CSOUND *,
                                 MYFLT **p, const char *name, int type);
```

has changed signature to

```
PUBLIC int32_t csoundGetChannelPtr(CSOUND *,
                                 void **p, const char *name, int32_t type);
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
PUBLIC void csoundTableCopyOutAsync(CSOUND *csound, int table, MYFLT *dest);
PUBLIC void csoundTableCopyInAsync(CSOUND *csound, int table, MYFLT *src);
PUBLIC void csoundGetNamedGEN(CSOUND *csound, int num, char *name, int len);
PUBLIC int csoundIsNamedGEN(CSOUND *csound, int num);
PUBLIC void *csoundGetNamedGens(CSOUND *);
```

These functions have been removed. Read table data through the
pointer returned by `csoundGetTable()`. The asynchronous variants of
the copy functions have been folded into their synchronous
counterparts, which gained an `async` argument:

```
PUBLIC void csoundTableCopyOut(CSOUND *csound, int32_t table,
                               cs_float *ptable, int32_t async);
PUBLIC void csoundTableCopyIn(CSOUND *csound, int32_t table,
                              const cs_float *ptable, int32_t async);
```

Pass `0` to copy immediately under the API lock, or `1` to enqueue
the copy for the performance thread.

The list of named generators returned by `csoundGetNamedGens()`
remains available to modules as `csound->GetNamedGens()`.

```
PUBLIC int32_t csoundSetIsGraphable(CSOUND *, int32_t isGraphable);
PUBLIC void csoundSetMakeGraphCallback(CSOUND *,
                           void (*makeGraphCallback_)(CSOUND *,
                                                      WINDAT *windat,
                                                      const char *name));
PUBLIC void csoundSetDrawGraphCallback(CSOUND *,
      void (*drawGraphCallback_)(CSOUND *,WINDAT *windat));

PUBLIC void csoundSetKillGraphCallback(CSOUND *,
       void (*killGraphCallback_)(CSOUND *, WINDAT *windat));
PUBLIC void csoundSetExitGraphCallback(CSOUND *,
                                         int32_t (*exitGraphCallback_)(CSOUND *));
```

These functions have been moved to `csound_graph_display.h`.

```
PUBLIC int32_t csoundNewOpcodeList(CSOUND *, opcodeListEntry **opcodelist);
PUBLIC void csoundDisposeOpcodeList(CSOUND *, opcodeListEntry *opcodelist);
```

These functions have been moved to `csound_misc.h`.

```
PUBLIC int csoundOpenLibrary(void **library, const char *libraryPath);
PUBLIC int csoundCloseLibrary(void *library);
PUBLIC void *csoundGetLibrarySymbol(void *library, const char *symbolName);
```

These functions have been removed; loading shared libraries is no
longer part of the host API.


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
PUBLIC int32_t csoundAppendOpcode(CSOUND *, const char *opname,
                                  size_t dsblksiz, int32_t flags,
                                  const char *outypes, const char *intypes,
                                  int32_t (*init)(CSOUND *, void *),
                                  int32_t (*perf)(CSOUND *, void *),
                                  int32_t (*deinit)(CSOUND *, void *));
```

Note the removal of the `thread` argument. `dsblksiz` is now of type
`size_t`, and `flags` and the callback functions use `int32_t`.

```
PUBLIC void csoundSetYieldCallback(CSOUND *, int (*yieldCallback_)(CSOUND *));
```

This function was removed, with `csoundPerform()` removed, this callback
has lost its purpose.

All functions under the *threading and concurrency* group have been
moved to `csound_threads.h`.

All functions under the *miscellaneous* group have been
moved to `csound_misc.h`.

All circular buffer functions have been moved to `csound_circular_buffer.h`.

The header file `csound_threaded.hpp` has been removed as it 
duplicates (in a limited way) multithreading that is now built into
the Csound library. This is exposed in `csPerfThread.h` and `csPerfThread.hpp`.
Functions in `csound.h` are wrapped in the Csound class in
`csound.hpp` and so the changes listed above have been propagated
to that interface.

## Type-System Constructor Callbacks

The `createVariable` callback in `CS_TYPE` now receives the canonical
type and its optional setup data as separate arguments. A Csound6
callback such as

```c
struct csvariable *create_variable(void *csound, void *type_arg);
```

must use this Csound7 signature:

```c
struct csvariable *create_variable(void *csound,
                                   const struct cstype *type,
                                   const void *type_arg,
                                   struct insds *ctx);
```

Similarly, the `copyValue` callback changed from

```c
void copy_value(void *csound, void *dest, void *src);
```

to

```c
void copy_value(CSOUND *csound, const struct cstype *type,
                void *dest, const void *src, struct insds *ctx);
```

Call `csoundCreateVariableForType()` instead of invoking a type's
constructor directly. It checks the result and sets the returned
variable's canonical type.

## Explicit Local Variable Shadowing

An explicitly typed local variable may now shadow a global variable
with the same name and a different type. For example:

```csound
value@global:i init 1

instr 1
  value:k = 2
endin
```

Here, `value:k` belongs to instrument 1. It does not replace
`value@global:i`. Explicit global declarations still use the global
scope rules.
