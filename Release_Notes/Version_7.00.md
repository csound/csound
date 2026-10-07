# Csound 7.00 release notes

Csound 7 adds new ways to write instruments, define data types, and use
Csound from other programs. It retains the classic orchestra and score
syntax. The host API and plugin API have changes that require source updates
and new builds.

These notes cover the main features and changes from Csound 6. They describe
the Csound 7 beta on the `develop` branch.

## Major changes

- Explicit types let you name variables without a type prefix.
- User-defined opcodes (UDOs) have a new declaration form and can change
  caller variables through references.
- User-defined structs keep named members in one typed variable. UDOs can
  accept and return structs, and arrays can contain them.
- Opcode and instrument objects let orchestra code store references and
  control separate instances.
- UDOs can use a local sample rate through `oversample` and `undersample`.
- The host API combines related calls. The unit generator (UGen) API lets
  hosts run individual opcodes.
- WebAssembly builds add command-line tools and support external score
  programs in the browser. Native builds can load supported WebAssembly
  opcode plugins through Wasmtime.

The following articles explain these features with examples:

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

`memplay` uses the `diskin2` argument order. It supports variable speed,
reverse playback, bounded loops, and loop crossfades. The first use loads
the whole file. Later instances share its samples until the engine resets.
Use `diskin2` when a file must stream from disk. See the
[memory playback guide](../docs/memplay.md) for memory use and loop options.

### Score and application input

Scores remain available for event control. Orchestra code can also create
instrument definitions and schedule their instances through typed values.

Browser applications can use an uploaded WASI command with
`<CsScore bin="...">` to generate a score. This feature uses the browser
filesystem and does not provide a system shell.

Hosts can supply application arguments through `csoundSetCommandLineArgs`.
The `readline` host callback lets an application supply a line of text to
the orchestra. See the [host input guide](../docs/readline-host-input.md).

### General use and compatibility

Csound 7 reads `.csound7rc`. Plugin search paths use `OPCODE7DIR` and
`OPCODE7DIR64`. Update local setup files and plugin paths when you install
Csound 7 beside Csound 6.

Default orchestra values remain `sr = 44100`, `ksmps = 10`, `nchnls = 1`,
and `0dbfs = 32768`. Set these values in the orchestra when a piece needs
other values. Sample-accurate score timing still requires `--sample-accurate`.

Deprecated opcodes remain available for older works. The
[replacement catalog](../docs/deprecated-opcodes.md) lists alternatives and
differences in behavior.

`--error-deprecated` rejects every opcode marked as deprecated, including
older registrations that previously only gave a warning. With this option,
an orchestra that uses these opcodes can fail to compile.

Message-level bit 1024 (`CS_NOQQ`, as in `-m1024`) suppresses ordinary
deprecation warnings for both registration styles. It does not override
`--error-deprecated`.

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
| `csoundGetChannelPtr` with `cs_float **` | Pass `void **`. Use channel locks or the typed get and set functions for shared access. |
| Table get, set, and copy helpers | Use `csoundGetTable` and the returned data pointer. The host must control concurrent access. |

Numeric events use `CS_INSTR_EVENT`, `CS_TABLE_EVENT`, or `CS_END_EVENT`.
Call `csoundStart` before `csoundEvent`. For score text before the engine
starts, use `csoundEventString` with `async = 0`.

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

The [Module API article](Csound7/Module_API.md) explains these changes, with
an opcode example and migration guidance for GENs, backends, and utilities.

### Numeric types

Use `cs_float` for sample data and API values. `MYFLT` remains a deprecated
alias. The `cs_double` type represents calculations that normally need
double precision. Hosts and plugins must match the library's precision.

See the [numeric type guide](../docs/numeric-types.md) for precision options
and compatibility aliases.

## System-level changes

### Builds and platforms

The `USE_FLOAT` build option selects single precision for both `cs_float`
and `cs_double`. It requires `USE_DOUBLE=OFF`. The default build keeps
double precision. A change in precision requires new host and plugin builds.

The browser package uses AudioWorklet for audio output. It no longer
supports ScriptProcessorNode or the `useSPN` option. Applications must use
the matching browser wrapper and WebAssembly binary package.

The WASI build supplies `csound-cli.wasm` and separate analysis and conversion
commands. Native Csound can load supported `.wasm` opcode plugins when
the build includes the Wasmtime C API. See the
[WebAssembly article](Csound7/WebAssembly.md) for the limits of each form.

Platform sources now live under `platform/`, language bindings under
`languages/`, and developer guides under `docs/`. See the
[build guide](../BUILD.md) for platform requirements.

### Internal interfaces

The compiler and type system now handle explicit types, structs, and
object references. Code that extends the type system must use the current
constructor callbacks and type descriptors.

These internal changes affect source extensions as well as binary plugins.
Use the Csound 7 headers for the library that your application loads.
