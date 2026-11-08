# Csound UGen AudioWorklet Demo

A minimal example that runs Csound's **UGen API** entirely inside an
[AudioWorklet](https://developer.mozilla.org/en-US/docs/Web/API/AudioWorklet),
with no audio rendering on the main thread.

An `oscili` unit generator is created via the UGen API, and its `process()`
callback calls `csoundUgenGraphPerform()` directly in the worklet thread —
one ksmps block (128 samples) per Web Audio rendering quantum.

## Project structure

```
src/
  main.js               – Main-thread UI: AudioContext, sliders, start/stop
  processor.js           – AudioWorkletProcessor (runs in worklet thread)
  worklet-polyfills.js   – Global shims so csound.js runs in AudioWorkletGlobalScope
index.html               – Demo page
vite.config.js           – Vite + custom esbuild plugin to bundle the processor
package.json
```

## How it works

AudioWorkletGlobalScope does not support ES module `import` or dynamic
`import()`.  The Vite plugin in `vite.config.js` uses **esbuild** to bundle
`src/processor.js` (which imports `libcsound` from `@csound/browser`) into a
single self-contained IIFE served at `/processor.js`.

The `@csound/browser` package's `dist/csound.js` is a Closure-compiled
main-thread build that probes globals like `self`, `window`, `navigator`,
`TextDecoder`, and `TextEncoder` at the top level.  These may not exist (or
may be stripped by browser extensions) inside an AudioWorklet, so
`src/worklet-polyfills.js` provides lightweight shims that are prepended as
an esbuild banner before the bundled code.

## Quick start

```bash
npm install
npm run dev
```

Then open http://localhost:5173, click **Start**, and adjust the sliders.

## Production build

```bash
npm run build
npm run preview
```

## Note on the `@csound/browser` dependency

This demo currently resolves `@csound/browser` to a **local hardcoded path**
(`../../dist/csound.js`) via an esbuild alias in `vite.config.js`.  This is
for development and testing within the csound source tree.

In a standalone project you would instead install the published package:

```bash
npm install @csound/browser
```

and update the esbuild alias in `vite.config.js` to point to the installed
package, e.g.:

```js
alias: { "@csound/browser": require.resolve("@csound/browser") }
```

or restructure the import to use the package's public entry point directly.
