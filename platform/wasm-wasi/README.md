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

- `csound.wasm` - browser-hosted WASI reactor (`_initialize`, no `_start`)
- `csound.wasm.z` - compressed browser-hosted module
- `csound-cli.wasm` - standalone WASI command for runtimes such as Wasmtime
- `csound-plugin-sdk.tar.gz` — plugin SDK archive
- `plugin_example.wasm` / `plugin_example_cpp.wasm` — example plugins

Both Csound modules are published in `@csound/wasm-bin`. The package `main`
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
