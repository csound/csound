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

`scripts/compile.sh` builds both targets from source and writes them to `lib/`:

| Output | Target | Pointers | Audio and score values |
| --- | --- | --- | --- |
| `csound.wasm`, `csound.wasm.z` | wasm32-wasip2 | 32-bit | float (`USE_DOUBLE=0`, `USE_FLOAT=1`) |
| `csound_x64.wasm`, `csound_x64.wasm.z` | wasm64-wasip2 | 64-bit | double (`USE_DOUBLE=1`, `USE_FLOAT=0`) |
| `csound-cli.wasm`, `csound_x64-cli.wasm` | Preview 2 components | as above | as above |
| `csound-plugin-sdk.tar.gz`, `csound_x64-plugin-sdk.tar.gz` | matching plugin headers and static library | as above | as above |

The package entry remains `csound.wasm`. Browser modules explicitly bundle
`csound_x64.wasm.z`, with one engine per module. They return `Float64Array`
table copies. `csoundUgenVarGetFloat64Array` gives a live view until memory
grows; `csoundUgenVarGetFloat32Array` gives a converted copy. Web Audio transport
buffers remain float. The build also supplies matching C and C++ example
plugins, with `_x64` in the names of the 64-bit versions.

Both targets use WASI Preview 2. Nix builds LLVM runtimes and wasi-libc from
source; it does not download a WASI SDK bundle. The shared nixpkgs pin is
`d3b13ad75cc492690728f9307e443dec9ef9a20e` (wasi-libc 32, LLVM 21).
The wasm64 port patches libc's pointer atomics, stack declaration and WIT
metadata object, plus compiler-rt's floating comparison return types.

The Component Model's direct memory64 ABI remains unfinished. The x64 build
therefore includes a small core adapter with its own memory32 heap. It copies
WASI strings, lists and records between that heap and the engine's memory64
heap. Resource handles and fixed-width scalars keep their WIT types. The
engine, libc, plugins and audio buffers use real 64-bit pointers throughout;
audio buffers do not pass through the adapter. A single WASI transfer must
fit the adapter's 2 GiB limit. This does not limit the engine heap to 32 bits.

Commands wrap both cores in a standard Preview 2 component and run under
unmodified Wasmtime. Browser artifacts embed the adapter in a custom section
so loading them needs no second engine download. The JS loader reads the ABI
metadata and converts native i64 pointers to exact Numbers for the public
API. It rejects values outside JavaScript's exact integer range.
Browsers must support memory64, table64, multiple memories and Wasm exceptions.
The browser loader caps engine memory at 8 GiB.

The [Component Model memory64 work](https://github.com/WebAssembly/component-model/issues/22)
and [WASI SDK target policy](https://github.com/WebAssembly/wasi-sdk#notable-limitations)
explain why the adapter is needed. Run the source-built host probes with:

```bash
source scripts/nixpkgs-pin.sh
nix-build ./src/wasi-preview2-support.nix -o result_preview2_support
nix-build ./src/wasi64-support.nix -o result_wasi64_support
```

The wasm64 probe runs real C code in Wasmtime and JavaScript, including file
paths and buffers above 4 GiB, arguments, directories, clocks, random data and
long-double comparisons. The build fails if either host check fails.

Rebuild plugins with the SDK for their target. Its generated `float-version.h`
sets precision. Do not pass `-DUSE_DOUBLE=0` as a C compiler flag: the headers
test whether that macro is defined. See [plugin compilation](PLUGIN_COMPILATION.md).

The command build also places these tools in `lib`. Each file uses the tool
name followed by `.wasm` for float, or `_x64.wasm` for double. Each also has
a `.core.wasm` derivative for the browser command host.

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

Each tool is a WASI Preview 2 component exporting `wasi:cli/run@0.2.0`. It includes its utility code and
libraries, so it does not need a native Csound installation or a utility plugin.
For example, convert a sound file to 48 kHz with Wasmtime.

```bash
wasmtime run -Wexceptions=y --dir=. ./lib/src_conv.wasm -r48000 -ooutput.wav input.wav
```

The browser host loads each command's `.core.wasm` derivative. It provides
Preview 2 imports, arguments, files and streams, then calls
`wasi:cli/run@0.2.0#run` once on a fresh instance. The browser must support standard
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

To run the command-line CSD suite against an already-built Preview 2
`lib/csound_x64-cli.wasm`:

```bash
source ./scripts/nixpkgs-pin.sh
nix-build ./src/csound-tests.nix
```

The test derivation also defaults to the local Nix system. Pass
`--argstr system x86_64-linux` to select a configured Linux builder explicitly.
It uses Wasmtime from the pinned Nixpkgs and runs the CSD files discovered
under `tests/commandline`. Tests
default to `-nd`; real-time tests can select the dummy audio driver and their
own options. Each file declares its own expectations, including WebAssembly
overrides for unavailable features such as sockets and PARCS. Preview 2 lacks
native thread support. The `wasip2` test profile records this limit and maps
expected error statuses to Preview 2's boolean exit result.

When the toolchain supports native threads, CI instead reuses
`test-artifacts/csound-realtime.wasm` from `scripts/compile-test-fixtures.sh`
and passes `--arg threaded true` to the test derivation. This keeps the
threaded tests ready for a future Preview 2 toolchain. The published
`csound-cli.wasm` remains the non-threaded command.

For releases, publish a new `@csound/wasm-bin` version before updating and
publishing `@csound/browser`; older binary packages do not contain the new
`csound_x64.wasm` engine and its SDK.

---

## Browser playback and threads

`@csound/browser` supports AudioWorklet playback and worker playback, with
SharedArrayBuffer or MessagePort transport. These modes use the same x64
engine. `useWorker` defaults to `false`; set it to `true` to run Csound in a
worker. SharedArrayBuffer transport needs cross-origin isolation headers.

Preview 2 does not supply native thread spawning in this toolchain.
`supportsRealtime()` returns 0, and `--realtime` cannot start Csound's native
background init worker. Browser playback still works. The build does not
substitute a Preview 1 engine or a 32-bit browser engine.

The internal thread probes remain available through
`scripts/compile-test-fixtures.sh`. They skip thread fixtures when the target
lacks support. They do not change the published engine.

---

## External score programs in the browser

The browser build supports `<CsScore bin="csbeats">` through the usual
Csound score parser. Upload a WASI Preview 2 core command to Csound's filesystem
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
An app such as the Web IDE can fetch `csbeats.core.wasm` on demand
and write it through the same filesystem API.

Other uploaded commands use the same path. They must export `memory` and
`cabi_realloc` and `wasi:cli/run@0.2.0#run`, and use Preview 2 imports. Each run has fresh memory and file
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
imports fail at instantiation. The host returns typed Preview 2 errors for
failed file and stream operations.
Timed clock waits require a worker that supports `Atomics.wait`; UI threads and
AudioWorklet cannot run commands that wait on a clock.

This is a Csound browser extension, not a WASI syscall. The pinned wasi-libc
declares `system()` but does not define it, and Preview 2 has no `exec` or
`system` import. The browser binary supplies `system()` through the explicit
`env.csoundWasiJsSystem(commandPointer)` import. The host reads a NUL-terminated
UTF-8 string from Csound memory and returns `exitCode << 8`, or `-1` on a host
error. Preview 2.0 commands report status 0 for success or 1 for failure.
`system(NULL)` returns zero because no shell is available. Use the
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
