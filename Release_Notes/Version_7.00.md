# Csound 7.00 release notes

Csound 7 adds new ways to write instruments, define data types, and use
Csound from other programs. It retains the classic orchestra and score
syntax. The host API and plugin API have changes that require source updates
and new builds.

These notes cover the main features and changes from Csound 6.


## Major changes

- Explicit types let you name variables without a type prefix.
- User-defined opcodes (UDOs) have a new declaration form and can change
  caller variables through references.
- User-defined structs keep named members in one typed variable. UDOs can
  accept and return structs, and arrays can contain them.
- Opcode and instrument objects let orchestra code store references and
  control separate instances.
- UDOs can use a local sample rate through `oversample` and `undersample`.
- State-machine opcodes let orchestra code follow named states, choose
  transitions, and keep separate runners for the same definition.
- The host API combines related calls. The unit generator (UGen) API lets
  hosts run individual opcodes.
- WebAssembly builds add command-line tools and support external score
  programs in the browser. The browser can also initialize notes in the
  background during realtime playback. Native builds can load supported
  WebAssembly opcode plugins through Wasmtime.

The following articles explain the main features with examples:

1. [Explicit types and user-defined opcodes](Csound7/Language.md)
2. [User-defined structs](Csound7/Structs.md)
3. [Opcode and instrument objects](Csound7/Objects.md)
4. [Local sample rates](Csound7/Local_Sample_Rates.md)
5. [Host applications and unit generators](Csound7/Host_API.md)
6. [WebAssembly and browser applications](Csound7/WebAssembly.md)
7. [Module API changes](Csound7/Module_API.md)

Each Csound example contains orchestra code for the `<CsInstruments>` section
of a CSD. Use each example in a separate CSD.

## User-level changes

### Orchestra language

An explicit declaration puts the type after the name, as in `frequency:i`
or `signal:a`. Use `@global` to declare a global variable, as in
`gain@global:k`. Classic names such as `ifrequency`, `asignal`, and `gkgain`
remain valid.

The new UDO form declares inputs and outputs together:

```csound
opcode Sine(amplitude:k, frequency:k):a
  signal:a = oscili(amplitude, frequency)
  xout signal
endop
```

This form does not need `xin`. At the caller's rates, a UDO can change a
variable that the caller passes as an input. UDOs that contain `setksmps`,
`oversample`, or `undersample` use copies, even when the rate stays unchanged.
Review assignments to input arguments when you convert an existing
UDO to the new form.

Array literals use brackets, as in `notes:i[] = [60, 64, 67]`.
Array declarations fix the rank. Use `[][]` for a matrix and `[*]` in a UDO
interface that accepts any rank. To change rank, use `reshapearray` with a
separate output. These checks apply to both legacy declarations and typed
annotations. Arithmetic keeps its input rank. Native vector-producing opcodes
such as `sorta`, `slicearray`, and `rfft` also retain their output rank when
used in expressions. Array math functions that only support vectors reject
matrix inputs rather than processing part of the matrix.

The `for ... in ... do` loop reads array elements and can also provide
their indices. Audio variables support sample access with an index.
The `switch`, `case`, `default`, and `endsw` statements select a branch
from several choices.

The `struct` declaration defines a type with named members. Structs can
contain other structs and arrays. The `Complex` type supports complex
arithmetic, arrays, and conversion between rectangular and polar forms.

The `InstrDef`, `Instr`, `OpcodeDef`, and `Opcode` types represent definitions
and instances. Named instruments also provide an `InstrDef` value that code
can pass directly to `schedule`. See the [object article](Csound7/Objects.md)
for the difference between a definition and an instance.

### New opcodes and extended operations

These groups cover the main additions. The
[Csound 7 manual](https://csound.com/manual/) gives the full opcode reference.

| Area | Main additions or extensions |
| --- | --- |
| Objects | `create`, `init`, `perf`, `run`, and `delete` control opcode or instrument objects. |
| Local rates | `oversample` and `undersample` change the sample rate inside a UDO. |
| JSON conversion | [`jsonunmarshal`, `jsonunmarshalfile`, and `jsonmarshal`](../docs/json-opcodes.md) read and write JSON values. |
| Complex numbers | `complex`, `polar`, `real`, `imag`, and `arg` construct or inspect complex values. |
| Sample playback | `memplay` reads decoded audio from a shared memory cache. |
| Spatial audio | `dbap` and `dbapgains` provide distance-based amplitude panning. |
| State machines | `stmcreate`, `stmcompile`, `stminstance`, `stmadvance`, `stmnext`, `stmcall`, and `stmrecall` build and run stateful graphs. |
| Filtering | `savgol` applies Savitzky–Golay smoothing or differentiation to audio or control signals. `savgolmat` supplies filter coefficients. |
| Mapping | `remap` maps control values, control arrays, or audio through breakpoint arrays. |
| Filtering | `savgol` applies Savitzky–Golay smoothing or differentiation to audio or control signals. `savgolmat` supplies filter coefficients. |
| State machines | `stmcreate`, `stmcompile`, `stminstance`, and related opcodes build state graphs and control their runners. |


`memplay` uses the `diskin2` argument order. It supports variable speed,
reverse playback, bounded loops, and loop crossfades. The first use loads
the whole file. Later instances share its samples until the engine resets.
Use `diskin2` when a file must stream from disk. See the
[memory playback guide](../docs/memplay.md) for memory use and loop options.


The `stm` opcodes build a state machine in the orchestra. `stmcreate` makes a
mutable topology, `stmcompile` freezes it, and `stminstance` creates a runner
from the frozen definition. Several runners can share one definition and keep
their own state. The orchestra reads the current node with `stmcurrent`, runs
the matching code, and calls `stmadvance` to step the graph. Node code asks
for a transition with `stmnext`. `stmadvance` applies it only when the graph
allows it. `stmcall` and `stmrecall` checkpoint and resume a runner, and
`stmpause` and `stmresume` pause and resume it. The family also reports
transition events. The graph has its own clock, read with `stmtick`,
`stmtime`, and `stmnodetime`. That clock advances only when
`stmadvance` runs, so it does not follow `times()`. The graph does not execute
instrument code by itself. See the [state-machine examples](../tests/commandline/stm/).

`savgol` fits a polynomial of a chosen order over a sliding window and
outputs the fit or one of its derivatives. The window size must be odd and at
least three. The output lags the input by `(window size - 1) / 2` audio samples
or control cycles, depending on the signal rate. `savgolmat` returns the filter
coefficients as an array.

`remap` uses breakpoint arrays, with linear, nearest, previous, next, or
shape-preserving cubic interpolation. Out-of-range inputs can cause an error,
use the nearest endpoint, use a fill value, or extend the curve beyond its
endpoints.


### Score and application input

Scores remain available for event control. Orchestra code can also create
instrument definitions and schedule their instances through typed values.

Browser applications can use an uploaded WASI command with
`<CsScore bin="...">` to generate a score. This feature uses the browser
filesystem and does not provide a system shell.

Hosts can supply application arguments through `csoundSetCommandLineArgs`.
The `readline` host callback lets an application supply a line of text to
the orchestra. See the [host input guide](../docs/readline-host-input.md).

### Changes in behavior

String `print` adds a trailing newline. `prints` uses the line endings in its
format string. Add `\n` when a new line is needed.

The DSSI hosting opcodes, including `dssiinit` and `dssiaudio`, have moved to
the separate [Csound plugins repository](https://github.com/csound/plugins).
They are no longer part of the core build. Install the separate plugin if
an orchestra uses them. The core build no longer has `BUILD_DSSI_OPCODES`.

The `MYFLT` alias still works, but the deprecation warning for it is paused
for this release. New code should use `cs_float` and `cs_double`.

### General use and compatibility

Csound 7 reads `.csound7rc`. Plugin search paths use `OPCODE7DIR` and
`OPCODE7DIR64`. Update local setup files and plugin paths when you install
Csound 7 beside Csound 6.


Rebuild opcode plugins for Csound 7. The loader rejects native plugins with
missing, zero, or incompatible metadata before their entry points run.

Plugin loading waits until compilation, startup, or another operation needs
the modules. Commands such as `csound --version` no longer scan plugin
directories. Missing default directories stay quiet unless verbose output
is enabled. Invalid paths set by the user still produce a warning.

Default orchestra values remain `sr = 44100`, `ksmps = 10`, `nchnls = 1`,
and `0dbfs = 32768`. Set these values in the orchestra when a piece needs
other values. Sample-accurate score timing still requires `--sample-accurate`.

Deprecated opcodes remain available for older works. The
[replacement catalog](../docs/deprecated-opcodes.md) lists alternatives and
differences in behavior.

The DSSI hosting opcodes, including `dssiinit` and `dssiaudio`, have moved to
the separate [Csound plugins repository](https://github.com/csound/plugins).
They are no longer part of the core build. Install the separate plugin if
an orchestra uses them. The core build no longer has `BUILD_DSSI_OPCODES`.

`--error-deprecated` rejects every opcode marked as deprecated, including
older registrations that previously only gave a warning. With this option,
an orchestra that uses these opcodes can fail to compile.

Message-level bit 1024 (`CS_NOQQ`) suppresses deprecation warnings from the
catalog and hard-coded opcodes. `-m1024` replaces the entire message-level
mask, so it also disables ordinary warnings. Combine the bit with the default
mask (`-m1159`) to suppress deprecation warnings only. Alternatively,
`--no-deprecated-warnings` does the same while preserving other warnings.
Neither suppression option overrides `--error-deprecated`.

### Selected fixes

- Compiler diagnostics retain the CSD filename and report score-preprocessor
  errors before cleanup.
- Realtime messages preserve their line breaks. Browser output keeps message
  fragments together instead of displaying each fragment on a separate line.
- SoundFont playback preserves global-zone pan, attenuation, and scale tuning.
  Hard pan positions no longer produce invalid samples.
- `liveconv` clears and reloads its convolution state correctly.
  `hrtfreverb` creates its FFT setup before use during initialization.
- Opcode arrays accept array arguments correctly, and reused UDO opcode
  objects bind their arguments to the current caller.

## API changes

### Host API

Csound 7 changes public function signatures and removes duplicate calls.
Update host code and rebuild it against the Csound 7 headers.
The following table covers the main changes.

| Csound 6 interface | Csound 7 interface or action |
| --- | --- |
| `csoundCreate(hostData)` | Use `csoundCreate(hostData, opcodedir)`. Pass `NULL` for the default plugin path. |
| Separate synchronous and asynchronous orchestra compilation | Use `csoundCompileOrc(csound, code, async)`. |
| `csoundCompileCsd` and `csoundCompileCsdText` | Use `csoundCompileCSD(csound, input, mode, async)`. Mode `0` selects a filename. Mode `1` selects CSD text. |
| Automatic start through `csoundCompile` | Call `csoundStart` before performance. |
| `csoundPerform` and `csoundPerformBuffer` | Call `csoundPerformKsmps` in a loop or use the performance thread API. |
| `csoundStop` and `csoundCleanup` | Stop the host's performance loop. Use `csoundReset` for reuse or `csoundDestroy` for disposal. |
| Score and input-message calls | Use `csoundEvent` for numeric events or `csoundEventString` for text. Both return `void`. |
| `csoundGetNchnls` and `csoundGetNchnlsInput` | Use `csoundGetChannels(csound, isInput)`. |
| `csoundGetAPIVersion` | Use `csoundGetVersion` for the library version. |
| Host audio and MIDI enable calls | Use `csoundSetHostAudioIO` and `csoundSetHostMIDIIO`. |
| Dedicated input and output option setters | Use `csoundSetOption`. It accepts several command-line options in one string. |
| Parameter structures passed to get and set calls | Read `csoundGetParams`, which returns a read-only `OPARMS` pointer. Change settings through options. |
| `csoundGetChannelPtr` with `cs_float **` | Pass `void **`. Use `csoundLockChannel` and `csoundUnlockChannel`, or the typed get and set functions, for shared access. |
| Table get, set, and named generator helpers | Use `csoundGetTable` and the returned data pointer. The host must control concurrent access. `csoundTableCopyIn` and `csoundTableCopyOut` remain, with a new `async` argument. |
| `csoundAppendOpcode` with separate init, k-rate, and a-rate callbacks | Use `init`, `perf`, and `deinit` callbacks and remove the `thread` argument. Use `csoundAppendOpcodes` to append a list of `OENTRY` entries. |

Numeric events use `CS_INSTR_EVENT`, `CS_TABLE_EVENT`, `CS_END_EVENT`, or
`CS_ADV_EVENT`.
Call `csoundStart` before `csoundEvent`. For score text before the engine
starts, use `csoundEventString` with `async = 0`.

`csoundPerformKsmps` returns zero while the performance continues, a
positive value when the score has finished, and a negative value on error.
Call it until it returns a nonzero value. Use one performance thread per
Csound instance and do not call performance from a Csound callback. A call
during an active render interval returns `CSOUND_ERROR`. This check does not
serialize concurrent callers, so the host must prevent overlapping
performance calls.

Asynchronous event and table-copy calls return without waiting for queue
space. If the queue rejects a request, these `void` calls report at most one
warning per instance between resets. Asynchronous compilation returns an
error when it cannot queue the merge. In realtime mode, synchronous table
copies take no mutex, so the host must keep them apart from performance,
table changes, and other host table copies.

`csoundMessage` and `csoundMessageS` write the text exactly as formatted and
add no newline. A host that receives messages through the message callback
should preserve the supplied line endings and combine fragments when it
needs complete lines. Adding a newline after every callback splits columns
that belong on one line.

The channel API now includes array and spectral data interfaces.
Use `csoundInitArrayChannel` or `csoundInitPvsChannel` to create these channels.
The associated data functions provide access to their contents.

The main library now contains the performance thread interface. C hosts
use `csPerfThread.h`, and C++ hosts use `csPerfThread.hpp`. The separate
`libcsnd` library is no longer part of this interface.

Specialized functions have separate headers for compiler access, threads,
files, server functions, graph displays, and circular buffers. Use
[`include/csound.h`](../include/csound.h) for current signatures and the
[migration guide](../docs/API_Migration_Guide_Csound_6_to_7.md) for migration topics.

### Unit generator API

The new [`ugen.h`](../include/ugen.h) interface lets a host create and run
individual opcodes. A host can connect typed inputs and outputs, then
process the opcodes in a defined order. This supports signal processing
without an orchestra for those operations.

The API includes factories, typed variable handles, and ordered UGen graphs.
The Python binding exposes these objects too. See the
[host API article](Csound7/Host_API.md) for setup and ownership rules.

### Module API

The Module API serves opcode plugins, GENs, audio and MIDI backends, and
utilities. Modules include `csdl.h` to use the structures, inline functions,
macros, and public `CSOUND` function table from `csoundCore.h`.
This interface has its own changes, separate from the host API.

Rebuild native modules with the Csound 7 headers.
The `CSOUND` function table and the `OENTRY`, `OPDS`, and `INSDS` layouts
have changed. Opcode registrations now use `init`, `perf`, and `deinit`
callbacks. The `thread` field and `RegisterDeinitCallback` are no longer
part of this interface.

Opcode context queries now use inline functions such as `GetInputArgCnt`
and `GetLocalSr`. Rate macros such as `CS_ESR` use the owning instance's
local rate. Audio backends still register their callbacks through `CSOUND`.
Utilities access their functions through `csound->GetUtility(csound)`.

A native plugin must report its version and precision through `csoundModuleInfo`.
The standard `LINKAGE` and `FLINKAGE` macros supply the current value.
A hand-written function must return `CSOUND_MODULE_INFO`.

Plugins can register struct types and define `[]` access for their types.
The plugin registers
`##array_get`, `##array_set`, and `##array_init` entries, and Csound lowers
`v[...]` to them. Ordinary arrays, including arrays of plugin types, need none
of these entries because Csound indexes them itself. The
[plugin indexing guide](../docs/plugin-indexing.md)
gives the entry layouts and the rules for when an access runs.

The `INSDS` structure carries a `private_data` pointer for engine-owned
state. This lets the engine add private state without adding more public
fields to `INSDS`. Plugins must not read that pointer, change it, or free
its data.

The [Module API article](Csound7/Module_API.md) explains these changes, with
an opcode example and migration guidance for GENs, backends, and utilities.

### Numeric types

Use `cs_float` for sample data and API values. `MYFLT` remains a deprecated
alias, with compiler deprecation warnings currently disabled.
The `cs_double` type represents calculations that normally need
double precision. Hosts and plugins must match the library's precision.

See the [numeric type guide](../docs/numeric-types.md) for precision options
and compatibility aliases.

## System-level changes

### Builds and platforms

Windows now has a native ARM64 installer alongside the x64 installer.
Both include the native WASAPI audio backend and IP MIDI. Extra plugins must
match the installed architecture. See the [Windows build guide](../BUILD.md#windows-arm64).

CoreMIDI now supports MIDI output and maps multiple input ports to separate
channel banks. JACK MIDI can run with a different audio backend.

The default macOS deployment target is macOS 12. The iOS build configurations
target iOS 15, and generated XCFrameworks include a module map for Swift.

The `USE_FLOAT` build option selects single precision for both `cs_float`
and `cs_double`. It requires `USE_DOUBLE=OFF`. The default build keeps
double precision. A change in precision requires new host and plugin builds.

The browser package uses AudioWorklet for audio output. It no longer
supports ScriptProcessorNode or the `useSPN` option. Applications must use
the matching browser wrapper and WebAssembly binary package.

Browser applications can use background note initialization with
`useWorker: true` and `--realtime` before `start()`. This requires cross-origin
isolation and has limits on which APIs can run during playback. See the
[browser setup](Csound7/WebAssembly.md#choose-the-browser-host).

The WASI build supplies `csound-cli.wasm` and separate analysis and conversion
commands. Native Csound can load supported `.wasm` opcode plugins when
the build includes the Wasmtime C API. See the
[WebAssembly article](Csound7/WebAssembly.md) for the limits of each form.

Platform sources now live under `platform/`, language bindings under
`languages/`, and developer guides under `docs/`. See the
[build guide](../BUILD.md) for platform requirements.

Windows now has a native ARM64 installer alongside the x64 installer.
Both include the native WASAPI audio backend and IP MIDI. Extra plugins must
match the installed architecture. See the [Windows build guide](../BUILD.md#windows-arm64).

The default macOS deployment target is macOS 12. The release workflow can
sign macOS binaries and installers when signing credentials are configured.
The iOS build configurations target iOS 15, and generated XCFrameworks
include a module map so Swift code can call the Csound C API directly.

The CoreMIDI module on macOS supports MIDI output and multiport MIDI input.
Select it with `-+rtmidi=coremidi`. `-M m` maps each input source to its own
bank of 16 channels. The output device accepts an index, `a` for all
destinations, or `m` for all destinations with port mapping.
JACK MIDI can run with a different audio backend.

### Realtime mode

In `--realtime` mode, supported hosts initialize notes on a background thread.
This does not make every host API call safe during performance. This release
protects active instances during render calls, preserves MIDI event order
across initialization and shutdown, and bounds the queues that carry host
API calls and log messages. Concurrent asynchronous calls need a build with atomic operations.
Without one, the host must keep asynchronous calls and performance calls
apart.

Live compilation works while the engine runs. `csoundCompileOrc` with
`async = 1` compiles on the calling thread and merges the result into the
running instance, and the channel functions address the channels of a live
instance. Errors found before the merge leave the running orchestra in place.
Keep compilation off an audio callback.

When the toolchain supports WASI threads, the browser package ships an
engine that supports background initialization. Select the worker host with
`useWorker: true` and set `--realtime` before `start()` to enable it. Local
execution uses synchronous initialization and warns when given the flag.
The threaded host needs a secure context, AudioWorklet, shared memory, and the
`Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers.
Some operations still require a stopped performance. See the
[WebAssembly article](Csound7/WebAssembly.md) for live API limits and the
[WASI guide](../platform/wasm-wasi/README.md) for build settings.

### Internal interfaces

The compiler and type system now handle explicit types, structs, and
object references. Code that extends the type system must use the current
constructor callbacks and type descriptors.

The private part of the `CSOUND` structure is now grouped by area, and
unused fields have been removed. Code that reads engine fields directly must
use the current header.

These internal changes affect source extensions as well as binary plugins.
Use the Csound 7 headers for the library that your application loads.
