# @csound/wasm — Local Development Guide

This guide explains how to build Csound WASM locally and link it into the
[Csound Web IDE](https://github.com/csound/web-ide).

---

## Overview

The WASM stack is split across two packages inside this directory:

| Package                | Path            | npm name           |
| ---------------------- | --------------- | ------------------ |
| Binary (`.wasm` files) | `platform/wasm-wasi/`         | `@csound/wasm-bin` |
| Browser wrapper        | `platform/wasm-wasi/browser/` | `@csound/browser`  |

The Web IDE depends on `@csound/browser`, which in turn depends on
`@csound/wasm-bin`. Linking works by replacing the installed npm packages with
symlinks that point at your local builds.

---

## 1. Build the WASM binary

The build uses [Nix](https://nixos.org/) to guarantee a reproducible
WASI cross compiler. Make sure you have Nix installed.

```bash
# From the repo root
cd platform/wasm-wasi
npm install           # install build-script dependencies
npm run build         # builds Csound, plugins and accessory commands with Nix
```

`scripts/compile.sh` produces the following artefacts in `platform/wasm-wasi/lib/`:

- `csound.wasm` / `csound.wasm.z` - baseline browser reactor and compressed copy
- `csound-browser-realtime.wasm` / `.wasm.z` - shared browser reactor, when supported
- `csound-cli.wasm` - standalone WASI command for runtimes such as Wasmtime
- `csound-plugin-sdk.tar.gz` - SDK matching the selected browser engine
- `csound-plugin-sdk-compat.tar.gz` - SDK for the baseline browser engine
- `plugin_example.wasm` / `plugin_example_cpp.wasm` - baseline example plugins
- `plugin_example_shared.wasm` / `plugin_example_cpp_shared.wasm` - shared examples, when supported

These modules are published in `@csound/wasm-bin`. The package `main`
entry remains the browser reactor, `csound.wasm`. Command-line runtimes should
load `csound-cli.wasm` explicitly.

The command build also places these tools in `lib`. Each file uses the tool
name followed by `.wasm`.

```text
atsa        csbeats     cvanal      dnoise      envext
extract     extractor   het_export  het_import  hetro
lpanal      lpc_export  lpc_import  mixer       mkir
pv_export   pv_import   pvanal      pvlook      scale
scot        scsort      sdif2ad     smf_conv    src_conv
```

`npm run build` builds everything through `scripts/compile.sh`. In command
mode, `src/csound.nix` compiles Csound's core once for the main command and
all tools. This mode also runs the Wasmtime checks in `postInstall`. The
libsamplerate build lives in `src/libsamplerate.nix`. Both the command and
browser builds link it, so `oversample` and `undersample` can use the sinc
converters. Configuration fails if CMake cannot find the library.

Each tool is a standalone WASI Preview 1 command with an exported `_start`
entry point and its own exported memory. It includes its utility code and
libraries, so it does not need a native Csound installation or a utility plugin.
For example, convert a sound file to 48 kHz with Wasmtime.

```bash
wasmtime run -Wexceptions=y --dir=. ./lib/src_conv.wasm -r48000 -ooutput.wav input.wav
```

A browser WASI host can load the same commands as its main module. The host
must provide WASI Preview 1 imports, arguments, files and standard streams,
then call `_start` once on a fresh instance. The browser must support standard
WebAssembly exception handling. These commands are separate from the
`@csound/browser` audio reactor and its plugin loader.

The build defaults to the local Nix system. To use a configured remote builder,
set `NIX_SYSTEM` to the system provided by that builder. For example, from
Darwin, `NIX_SYSTEM=x86_64-linux yarn build` selects an available
`x86_64-linux` builder.

The standalone module currently targets Wasmtime's standardized WebAssembly
exception handling. A direct invocation looks like:

```bash
wasmtime run -Wexceptions=y --dir=. ./lib/csound-cli.wasm -nd ./example.csd
```

Native Csound can also use Wasmtime to load supported opcode plugins passed as
`--opcode-lib=file.wasm`. See
[Native Csound WebAssembly plugins](NATIVE_OPCODE_PLUGINS.md) for the build
option, current ABI limits, and compile-cache rules.

To run the command-line CSD suite against an already-built
`lib/csound-cli.wasm`:

```bash
source ./scripts/nixpkgs-pin.sh
nix-build ./src/csound-tests.nix
```

The test derivation also defaults to the local Nix system. Pass
`--argstr system x86_64-linux` to select a configured Linux builder explicitly.
It uses Wasmtime from the pinned Nixpkgs, disables audio with `-nd`, and runs the
CSD files discovered under `tests/commandline`. Each file declares its own
expectations, including WebAssembly overrides. The OSC and asynchronous
`ftaudio` cases check the diagnostics for unavailable sockets and threads.

For releases, publish a new `@csound/wasm-bin` version before updating and
publishing `@csound/browser`; older binary packages do not contain the new
`csound-cli.wasm` command artifact.

---

## WASI realtime support

`npm run build` builds the compatible browser module and, when the toolchain
supports WASI threads, a shared-memory browser reactor. Both ship in
`@csound/wasm-bin`. The `csound.js` bundle includes one engine: the shared
reactor when available, otherwise the baseline module. All browser APIs use
that engine. No separate realtime build command or runtime module URL is needed.

CMake's `BUILD_WASI_REALTIME` setting defaults to `AUTO`. Its compile/link
check requires real pthread entry points, lock-free atomics, and shared
memory. `ON` requires that support; `OFF` disables it. The normal Nix build
checks a threaded C/C++ runtime built with the pinned LLVM 21. If the check
fails, it packages only the compatible module. Other build failures still
fail the build.

Support does not enable the mode. Csound starts its background init worker
only when `--realtime` is set before performance. Engines without support
retain synchronous behavior and warn when given the flag.

## WASI realtime browser

Build the binaries and browser wrapper through the usual commands:

```bash
# From platform/wasm-wasi
npm run build
cd browser
npm ci
npm run build:prod
```

The wrapper uses local `../lib` artifacts when present, or the installed
`@csound/wasm-bin` package. Serve `browser/dist/csound.js` from your app.
No second engine file is needed. Builds without a threaded reactor use the
baseline engine.

The threaded host needs a secure context (localhost works), AudioWorklet,
shared memory, Wasm atomics and exceptions, and these HTTP response headers:

```text
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

The default entry selects the shared host when these features are available.
Otherwise, it uses the same engine in one AudioWorklet with synchronous init.
Explicit `useWorker: true` requires the shared host unless `useSAB: false`
selects MessagePort mode; missing support causes an error before opening an
AudioContext. All modes still require the Wasm features of the bundled engine.
A failed worker startup closes the instance and any context it owns.

Explicit `useWorker: false` runs the engine in one AudioWorklet. `useSAB: false`
keeps the existing transport without shared audio buffers. The named
`libcsound()` constructor keeps its synchronous engine API. These modes use
the same shared Wasm binary locally, without passing its memory across workers,
so they do not require isolation headers. They still need shared Wasm memory,
atomics, and standard Wasm exceptions. They use synchronous init and ignore
`--realtime` with a warning; `supportsRealtime()` returns 0.

```js
import Csound from "/csound.js";

const cs = await Csound();
await cs.setOption("-odac -m0 --daemon");
await cs.setOption("--realtime");
await cs.compileOrc(`
  sr = 48000
  ksmps = 32
  nchnls = 2
  0dbfs = 1
  instr 1
    a1 oscils 0.1, p4, 0
    outs a1, a1
  endin
`);
// Start/resume from a user gesture if the browser requires one.
await cs.start();
await cs.event(0, [1, 0, 0.25, 440]);
// Later:
await cs.stop();
await cs.reset();
await cs.terminateInstance();
```

`supportsRealtime()` returns 1 when the selected engine supports the mode;
`isRealtime()` returns the native flag setting. Both return 0 otherwise.
`<CsOptions> --realtime </CsOptions>` also enables it. Reset clears the flag.
The wrapper reads native state, so rejected option changes do not change its
mode. Omitting the flag keeps synchronous init and the usual live engine APIs.

The shared host prepares four child Workers against fixed 256 MiB Wasm
memory. `src/shared-memory-max.txt` sets this limit for the engine, plugins,
and browser bundle. Change that file and rebuild all three together; CMake
rejects a conflicting `CSOUND_WASI_MAX_MEMORY` override. Csound uses its existing
init pthread when the flag is set. The UI owns all five workers, so it can end a failed engine even if the render worker
waits in a join. The AudioWorklet consumes audio from the shared ring. Wasm
constructors run once, before the child workers become ready.

With `--realtime`, compile and preload while stopped. During performance the
wrapper accepts numeric note events, MIDI, the sample clock, fixed audio
settings, mode queries, and pause/resume/stop. Compilation, filesystem calls,
table changes, and other engine APIs reject while realtime mode is active.
Resume before submitting engine calls while the shared host is paused.

Dynamic plugins work in all modes. Rebuild plugins with the matching
`csound-plugin-sdk.tar.gz`; old unshared binaries cannot use the shared engine
and fail with a clear error. The normal build produces the matching SDK and
shared C/C++ examples. Pass plugins through `withPlugins`, or write them to the
virtual filesystem and select them through Csound options before the first
`start()`. The shared worker host fixes its plugin set at that first start,
even without `--realtime`. Reset can reuse those plugins but cannot add new
ones. Create a new instance to change that set.

The SDK and example Nix builds check for `lib/csound-browser-realtime.wasm.z`
to choose the default memory ABI, just as the browser build does. When passing
an SDK archive from another package, also pass `--arg sharedMemory true` for
a shared engine or `--arg sharedMemory false` for an unshared engine.

The loader gives every worker the same function-table layout and each thread
its own plugin TLS storage. It initializes shared data and runs constructors
once. Plugins may use module-local TLS; cross-module TLS addresses and dependent
shared libraries are unsupported. Parent-only modes can still load plugins
synchronously through the filesystem. Child-thread file access, nested thread
creation, and parallel DSP remain unsupported. Preloading files does not make
child file descriptors available. Plugins must avoid those operations during
background init.

`event(0, pfields)` takes 3–64 finite numbers in active realtime mode and
requests asynchronous note admission. The shared host holds at most 256 API
calls and sends at most 32 per batch. A resolved call confirms dispatch, not
note init or audible output. Native queue overflow follows Csound's diagnostics.
A timeout removes pending host work but cannot undo a call already sent. In
realtime mode, stop rejects pending work; a later run never replays it. Without
the flag, completion keeps the existing API call behavior. MIDI uses a separate
1,024-packet ring and rejects full writes without replacing unread packets.

Normal stop/reset waits for init to finish. It cannot cancel an arbitrary
opcode. A transport timeout or fatal worker error rejects pending calls and
ends the instance. Create a new instance afterward. `terminateInstance()` also
discards all workers and shared engine state.

To run the Chrome checks, build a second wrapper with the test-only init gate:

```bash
# From platform/wasm-wasi, after the normal build
bash scripts/compile-test-fixtures.sh
cd browser
npm run test:fixtures
npm run test:realtime
```

`CSOUND_THREADED_WASM` is a build-time override for a raw or compressed reactor.
CI runs the WASI command pthread checks and these browser checks. Test fixtures
stay outside the published `lib/` directory; the fixture build leaves `dist/` on
the production engine. The gate is absent from normal artifacts. The checks
cover flag activation,
reset, shared C/C++ plugins, local modes without isolation headers, audio and
clock progress during held init, and shutdown. They do not establish a latency
bound or make every opcode safe for concurrent init. Set
`CSOUND_BROWSER_ENGINE=firefox` to run them in Firefox.

## WASI realtime command host

The published command module remains compatible with unthreaded WASI hosts.
For a WASI Preview 1 threads command host, build the internal Nix target:

```bash
# From platform/wasm-wasi
source scripts/nixpkgs-pin.sh
nix-build src/csound-realtime.nix -o result_realtime
wasmtime run -Sthreads=y -Wthreads=y -Wshared-memory=y -Wexceptions=y \
  -Ccache=n --dir=. result_realtime/lib/csound-realtime.wasm \
  --realtime -nd example.csd
```

This target keeps its static library and headers and runs the pthread and
Csound worker checks. It does not replace published command modules. It needs
a host with WASI P1 threads, shared memory, and Wasm exceptions.

An unpaced render can advance score time while background init waits. Compile
instruments before performance and use numeric events. Concurrent compilation
still needs an audit of shared compiler state. Dynamic plugins, external
processes, and parallel DSP remain outside this target.

Normal init errors keep Csound's note-error behavior. A fatal worker error
ends the command with failure: the worker cannot use the render thread's jump
buffer or safely clean up a half-finished opcode. The browser reactor uses the
same rule.

---

## External score programs in the browser

The browser build supports `<CsScore bin="csbeats">` through the usual
Csound score parser. Upload a WASI Preview 1 command to Csound's filesystem
before compiling the document:

```js
await csound.fs.writeFile("csbeats.wasm", bytes);
await csound.compileCSD(document);
```

The binding first looks for the exact `bin` program path, relative to
`csound.fs.getcwd()`, then tries that path with `.wasm` appended. It loads
and compiles the file only when Csound calls it. It caches up to four compiled
modules and checks the bytes again on each call, so replacing an uploaded
file takes effect. No command binaries or URLs are built into the binding.
An app such as the Web IDE can fetch its bundled `csbeats.wasm` on demand
and write it through the same filesystem API.

Other uploaded commands use the same path. They must export `memory` and
`_start` and use WASI Preview 1 imports. Each run has fresh memory and file
descriptors. The command sees Csound's current directory at `/`, like a
WASI runtime preopening one directory; files outside that directory are
not mounted. Csound passes the score input and output filenames as the last
two arguments. Commands use the binding's existing WASI code and share its
file storage directly, with no extra runtime dependency or file copies. File
changes take effect as the command writes, including when it later fails.
Each open handle keeps its own position. Standard input is empty, and the
last 16 KB from each output stream goes to Csound's message listeners.

The call is synchronous because Csound reads the generated score as soon as
`system()` returns. Long commands can hold up the engine thread. Arguments
can use quotes and backslash escapes, but there is no shell, pipe,
redirection, variable expansion or process spawning. A nonzero exit status,
missing command, invalid module or trap fails score generation. Unsupported
WASI calls return `ENOSYS`.

This is a Csound browser extension, not a WASI syscall. The pinned wasi-libc
declares `system()` but does not define it, and Preview 1 has no `exec` or
`system` import. The browser binary supplies `system()` through the explicit
`env.csoundWasiJsSystem(commandPointer)` import. The host reads a NUL-terminated
UTF-8 string from Csound memory and returns `exitCode << 8`, or `-1` on a host
error. `system(NULL)` returns zero because no shell is available. Use the
matching browser wrapper with this binary. The standalone WASI command has
no new host import and reports external score generation as unsupported.

---

## 2. Link `@csound/wasm-bin` locally


```bash
# Inside platform/wasm-wasi/
npm link            # registers this directory as the local @csound/wasm-bin
```

Then wire the browser wrapper to pick up the local binary:

The browser package requires Node.js 22.13 or later. Its npm settings enforce
the Node version and peer dependencies. Use `npm ci` for a clean install from
the lockfile; do not use `--legacy-peer-deps` or `--force`.

For nvm users, `platform/wasm-wasi/browser/.nvmrc` pins Node 22.23.2. Run `nvm install` and
`nvm use` from that directory before installing dependencies. CI reads the
same file. The file does not change your shell or PATH on its own; Nix users
can keep using `nix-shell`, which supplies Node 22 without nvm.

```bash
cd browser
npm ci              # install browser-wrapper dependencies
npm link @csound/wasm-bin   # replace the npm version with your local build
```

---

## 3. Build `@csound/browser`

```bash
# Still inside platform/wasm-wasi/browser/
npm run build       # development build
# or
npm run build:prod  # production build
```

The compiled output lands in `platform/wasm-wasi/browser/dist/`.

---

## 4. Link `@csound/browser` into the Web IDE

```bash
# Inside platform/wasm-wasi/browser/
npm link            # registers this directory as the local @csound/browser
```

```bash
# Inside web-ide/
npm link @csound/browser    # replace the npm version with your local build
```

The Web IDE dev server will now import your locally built `@csound/browser`
(and transitively your local `.wasm` binary) whenever you run:

```bash
# Inside web-ide/
npm start
```

---

## 5. Iterating after changes

Once links are in place (steps 1–4 are **one-time setup**), the rebuild cycle
is just:

```bash
# Inside platform/wasm-wasi/browser/
npm run build
```

Vite will detect the updated files and reload automatically.
There is no need to re-run `npm link` — the symlink persists.

> You may see a Babel note in the Vite output:
> `[BABEL] Note: The code generator has deoptimised the styling of .../csound.js as it exceeds the max of 500KB.`
> This is **informational only** — Babel skips pretty-printing large files for
> performance. It does not affect functionality.

---

## 6. Teardown — restore published versions

When you are done testing locally, remove the symlinks:

```bash
# Inside web-ide/
npm unlink @csound/browser
npm install         # re-install the published version

# Inside platform/wasm-wasi/browser/
npm unlink @csound/wasm-bin
npm install
```
