# WebAssembly and browser applications

[Csound 7 release notes](../Version_7.00.md)

Csound 7 provides WebAssembly builds for browser applications and WASI
command-line runtimes. Native Csound can also load a supported WebAssembly
opcode plugin through Wasmtime. Each form has a different host interface.

| Form | Main use |
| --- | --- |
| `csound.wasm` | The wasm32 browser engine with float samples. |
| `csound-wasm64.wasm` | The wasm64 browser engine with double samples. |
| `csound-cli.wasm`, `csound-wasm64-cli.wasm` | Csound commands for a WASI Preview 3 runtime. |
| Utility commands such as `pvanal.wasm` | Separate analysis or conversion tasks. |
| An opcode `.wasm` file | Extra opcodes loaded by a compatible host. |

## Use the browser packages together

`@csound/browser` supplies the JavaScript interface.
`@csound/wasm-bin` supplies the compiled modules. Use the matching package
versions so the wrapper and engine agree on their interface.

Audio output uses AudioWorklet. ScriptProcessorNode support has ended, so
remove the old `useSPN` option from application setup. Microphone input
requires a secure browser context and the user's permission.

Call `enableAudioInput` before `start` when the application must handle
microphone errors before performance. A CSD with `-iadc` can also request
input during startup. See the
[browser guide](../../platform/wasm-wasi/browser/README.md) for input setup.

## Choose the browser host

Csound runs in an AudioWorklet by default. Pass `useWorker: true` to run the
engine in a worker. SharedArrayBuffer transport needs cross-origin isolation.
The browser package defaults to wasm32. Import from
`@csound/browser/wasm64.js` to use wasm64.

`Csound()` now rejects its promise when AudioWorklet support is missing.
Catch errors around `await Csound()` instead of checking for `undefined`.

## Initialize notes in the background

Background note initialization requires parallel threads. The Preview 3
builds initialize notes synchronously and report `supportsRealtime() === 0`.

## Load browser opcode plugins

Rebuild browser plugins with the Csound plugin SDK for the engine's pointer
width and sample precision.

Browser plugins can link static runtime libraries, including C++ libraries
with exception handling. They share the engine's memory and allocator.
Separate plugins cannot catch each other's C++ exceptions, and shared
dynamic-library dependencies are not supported. The
[plugin build guide](../../platform/wasm-wasi/PLUGIN_COMPILATION.md)
describes the required toolchain and linking rules.

## Run external score programs in the browser

The browser host can run an uploaded WASI command for a CSD score section.
For example, `<CsScore bin="csbeats">` can use `csbeats.wasm` from Csound's
virtual filesystem. Fetch `csbeats.core.wasm` from the matching binary package
and write its bytes to that filesystem as `csbeats.wasm`.

The application must put the command file in that filesystem before CSD
compilation. The host first looks for the exact program path, then tries
that path with `.wasm` appended. The utility must be a Preview 3 core module.

Csound passes the score input and output filenames to the command.
The command shares files with the browser host through the virtual filesystem.
This call is synchronous. A long command delays the engine thread.

The host does not provide shell pipes, redirection, or process creation.
This browser feature does not add external score programs to the standalone
WASI command.

## Run Csound and utilities as WASI commands

Use `csound-cli.wasm` with a runtime that supports WASI Preview 3 and
WebAssembly exceptions.

```sh
wasmtime run -Sp3 -Wexceptions=y --dir=. ./lib/csound-cli.wasm -nd example.csd
```

The `--dir=.` option gives the command access to files in the current
directory. Select directories that contain the CSD and its input files.

The build also supplies standalone analysis and conversion commands.
This command converts a sound file's sample rate.

```sh
wasmtime run -Sp3 -Wexceptions=y --dir=. ./lib/src_conv.wasm -r48000 -ooutput.wav input.wav
```

The [WASI guide](../../platform/wasm-wasi/README.md) lists the supplied tools and
build requirements.

## Load a WebAssembly opcode in native Csound

A native build with Wasmtime support accepts a supported plugin through
the usual library option:

```sh
csound --opcode-lib=./velvetlp.wasm piece.csd
```

The current native loader accepts the `OPCODE.WASM` plugin format with
64-bit `cs_float` values and fixed `i`, `k`, and `a` arguments.
It supports initialization, performance, and deinitialization callbacks.

Strings, arrays, function tables, optional arguments, and general Csound
host calls are outside this interface. An arbitrary WASI program is not
an opcode plugin. The host reports an error when a plugin needs an
unsupported feature.

Wasmtime compiles a raw plugin on its first load and can reuse its local
compile cache on later loads. Distribute the raw `.wasm` file.
The [native plugin guide](../../platform/wasm-wasi/NATIVE_OPCODE_PLUGINS.md)
gives the required build options and current interface limits.
