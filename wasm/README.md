# @csound/wasm — Local Development Guide

This guide explains how to build Csound WASM locally and link it into the
[Csound Web IDE](https://github.com/csound/web-ide).

---

## Overview

The WASM stack is split across two packages inside this directory:

| Package                | Path            | npm name           |
| ---------------------- | --------------- | ------------------ |
| Binary (`.wasm` files) | `wasm/`         | `@csound/wasm-bin` |
| Browser wrapper        | `wasm/browser/` | `@csound/browser`  |

The Web IDE depends on `@csound/browser`, which in turn depends on
`@csound/wasm-bin`. Linking works by replacing the installed npm packages with
symlinks that point at your local builds.

---

## 1. Build the WASM binary

The build uses [Nix](https://nixos.org/) to guarantee a reproducible
Emscripten toolchain. Make sure you have Nix installed.

```bash
# From the repo root
cd wasm
npm install           # install build-script dependencies
npm run build         # runs scripts/compile.sh via nix-build
```

`scripts/compile.sh` produces the following artefacts in `wasm/lib/`:

- `csound.wasm` — main binary
- `csound.wasm.z` — compressed variant
- `csound-plugin-sdk.tar.gz` — plugin SDK archive
- `plugin_example.wasm` / `plugin_example_cpp.wasm` — example plugins

---

## 2. Link `@csound/wasm-bin` locally

```bash
# Inside wasm/
npm link            # registers this directory as the local @csound/wasm-bin
```

Then wire the browser wrapper to pick up the local binary:

```bash
cd browser
npm install         # install browser-wrapper dependencies
npm link @csound/wasm-bin   # replace the npm version with your local build
```

---

## 3. Build `@csound/browser`

```bash
# Still inside wasm/browser/
npm run build       # development build
# or
npm run build:prod  # production build
```

The compiled output lands in `wasm/browser/dist/`.

---

## 4. Link `@csound/browser` into the Web IDE

```bash
# Inside wasm/browser/
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
# Inside wasm/browser/
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

# Inside wasm/browser/
npm unlink @csound/wasm-bin
npm install
```
