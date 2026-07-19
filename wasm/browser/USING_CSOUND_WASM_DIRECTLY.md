# Using Csound WASM Directly

The high-level `Csound()` API from `@csound/browser` creates and manages the
Web Audio nodes, workers, filesystem, and Csound instance for an application.
Applications that need a different architecture can instead use the
low-level `libcsound()` API or instantiate the browser-hosted WebAssembly
binary themselves.

This document demonstrates how to run several `CSOUND*` instances inside one
WebAssembly instance and one `AudioWorkletNode`. Each Csound instance is
exposed through a separate worklet output, so routing remains the
responsibility of the Web Audio graph. Csound output device names such as
`dac1`, `dac2`, and `dac3` are not used for this purpose.

> [!NOTE]
> Running one Csound instance per note is an unusually expensive form of
> polyphony. A single Csound orchestra containing multiple notes will usually
> be more efficient. The examples below demonstrate the lower-level hosting
> model for applications that specifically require isolated Csound instances.

## Choose the correct WASM artifact

`@csound/wasm-bin` contains two different WASI modules:

- `lib/csound.wasm` is a standalone WASI command. It has a command-style
  `_start` entry point and is intended for runtimes such as Wasmtime.
- `lib/csound-no-entry.wasm` is the browser-hosted WASI reactor. It has an
  `_initialize` entry point, imports its memory and host callbacks, and is the
  module used by `@csound/browser`.
- `lib/csound-no-entry.wasm.z` is the compressed form of the browser reactor
  used when building the browser package.

Code that supplies its own browser host must load `csound-no-entry.wasm`
explicitly. The package's default entry points to the standalone
`csound.wasm`, which is not interchangeable with the browser reactor.

## Architecture

Both examples in this document use the following arrangement:

```text
AudioContext
  |
  `-- AudioWorkletNode
        |
        `-- one WebAssembly.Instance
              |-- CSOUND* 1 --> AudioWorklet output 0
              |-- CSOUND* 2 --> AudioWorklet output 1
              `-- CSOUND* 3 --> AudioWorklet output 2
```

The main thread connects each worklet output to ordinary Web Audio nodes such
as `GainNode`, `StereoPannerNode`, and `AudioDestinationNode`.

The two loading approaches are:

1. Use `libcsound()` from `@csound/browser`. This reuses Csound's WASI,
   filesystem, callback, memory, and API-binding code while avoiding the
   high-level Web Audio wrapper.
2. Load `@csound/wasm-bin/lib/csound-no-entry.wasm` directly. The application
   must then provide WASI, memory, callbacks, string marshalling, filesystem
   behavior, and any desired plugin loading.

## Shared multi-instance processor

The following processor implementation is used by both loading approaches.
The loader passed to `makeMultiCsoundProcessor()` returns a small API matching
the relevant part of the Csound C API.

Each `CSOUND*` has an independent orchestra and spout buffer, but all of the
pointers belong to the same WebAssembly instance and linear memory.

```js
// multi-csound-processor.js

export function makeMultiCsoundProcessor(loadApi) {
  return class MultiCsoundProcessor extends AudioWorkletProcessor {
    constructor(options) {
      super();

      const config = options.processorOptions ?? {};

      this.api = null;
      this.voices = [];
      this.ready = false;
      this.running = true;

      this.port.onmessage = ({ data }) => {
        if (data?.type === "stop") {
          this.running = false;
          this.destroy();
        }
      };

      void this.initialize(config);
    }

    async initialize(config) {
      try {
        const api = await loadApi(config, this.port);

        if (!this.running) {
          return;
        }

        this.api = api;

        const frequencies = config.frequencies ?? [220, 330, 440];
        const sr = config.sampleRate ?? sampleRate;
        const ksmps = config.ksmps ?? 64;

        for (const frequency of frequencies) {
          this.voices.push(
            this.createVoice({
              frequency,
              sr,
              ksmps,
            }),
          );
        }

        this.ready = true;
        this.port.postMessage({
          type: "ready",
          voiceCount: this.voices.length,
        });
      } catch (error) {
        this.destroy();
        this.port.postMessage({
          type: "error",
          message: error instanceof Error ? error.message : String(error),
        });
      }
    }

    createVoice({ frequency, sr, ksmps }) {
      const api = this.api;
      const csound = api.csoundCreate();

      if (!csound) {
        throw new Error("csoundCreate() returned a null pointer");
      }

      for (const option of ["-d", "-n"]) {
        const result = api.csoundSetOption(csound, option);
        if (result !== 0) {
          api.csoundDestroy(csound);
          throw new Error(`csoundSetOption(${option}) failed: ${result}`);
        }
      }

      const orchestra = `
sr = ${sr}
ksmps = ${ksmps}
nchnls = 2
0dbfs = 1

instr 1
  aout poscil 0.15, p4
  outs aout, aout
endin

schedule(1, 0, -1, ${frequency})
`;

      const compileResult = api.csoundCompileOrc(csound, orchestra);
      if (compileResult !== 0) {
        api.csoundDestroy(csound);
        throw new Error(`csoundCompileOrc() failed: ${compileResult}`);
      }

      const startResult = api.csoundStart(csound);
      if (startResult !== 0) {
        api.csoundDestroy(csound);
        throw new Error(`csoundStart() failed: ${startResult}`);
      }

      const actualKsmps = api.csoundGetKsmps(csound);
      const channels = api.csoundGetNchnls(csound);
      const zeroDbfs = api.csoundGet0dBFS(csound) || 1;

      return {
        csound,
        ksmps: actualKsmps,
        channels,
        zeroDbfs,

        // Force csoundPerformKsmps() before reading the first sample.
        cursor: actualKsmps,
        active: true,

        spout: null,
        spoutPointer: 0,
        memoryBuffer: null,
      };
    }

    ensureCurrentBlock(voice) {
      if (!voice.active) {
        return false;
      }

      if (voice.cursor < voice.ksmps) {
        return true;
      }

      const result = this.api.csoundPerformKsmps(voice.csound);
      if (result !== 0) {
        voice.active = false;
        return false;
      }

      const memoryBuffer = this.api.getMemory().buffer;
      const spoutPointer = this.api.csoundGetSpout(voice.csound);

      // Recreate the view after WebAssembly.Memory grows or if Csound moves
      // the spout buffer.
      if (
        !voice.spout ||
        voice.spout.length === 0 ||
        voice.memoryBuffer !== memoryBuffer ||
        voice.spoutPointer !== spoutPointer
      ) {
        voice.spout = new Float64Array(memoryBuffer, spoutPointer, voice.ksmps * voice.channels);

        voice.memoryBuffer = memoryBuffer;
        voice.spoutPointer = spoutPointer;
      }

      voice.cursor = 0;
      return true;
    }

    process(_inputs, outputs) {
      // Each CSOUND* is routed to a separate AudioWorklet output.
      for (const output of outputs) {
        for (const channel of output) {
          channel.fill(0);
        }
      }

      if (!this.running) {
        return false;
      }

      if (!this.ready) {
        return true;
      }

      for (let voiceIndex = 0; voiceIndex < this.voices.length; voiceIndex++) {
        const voice = this.voices[voiceIndex];
        const output = outputs[voiceIndex];

        if (!output) {
          continue;
        }

        const frameCount = output[0]?.length ?? 0;

        for (let frame = 0; frame < frameCount; frame++) {
          if (!this.ensureCurrentBlock(voice)) {
            break;
          }

          for (let channel = 0; channel < output.length; channel++) {
            const csoundChannel = Math.min(channel, voice.channels - 1);
            const index = voice.cursor * voice.channels + csoundChannel;

            output[channel][frame] = voice.spout[index] / voice.zeroDbfs;
          }

          voice.cursor++;
        }
      }

      return true;
    }

    destroy() {
      if (!this.api) {
        return;
      }

      for (const voice of this.voices) {
        if (voice.csound) {
          this.api.csoundDestroy(voice.csound);
          voice.csound = 0;
        }
      }

      this.voices = [];
      this.ready = false;
    }
  };
}
```

The render loop does not assume that Csound's `ksmps` is equal to the Web
Audio render quantum. It requests another Csound block whenever the current
spout block has been consumed.

## Approach 1: use `libcsound()`

Install the browser package:

```bash
npm install @csound/browser
```

The low-level `libcsound()` constructor creates the WASM/WASI runtime without
creating an `AudioContext` or `AudioWorkletNode`. Calling `csoundCreate()`
several times on the returned API creates several Csound pointers in that one
runtime.

### Worklet processor entry point

```js
// libcsound.processor.js

import { libcsound } from "@csound/browser";
import { makeMultiCsoundProcessor } from "./multi-csound-processor.js";

let sharedApiPromise = null;

async function loadApi() {
  // Shared by every processor created in this AudioWorkletGlobalScope.
  sharedApiPromise ??= libcsound();
  return sharedApiPromise;
}

registerProcessor("multi-csound-libcsound", makeMultiCsoundProcessor(loadApi));
```

Bundle this file and its imports into an AudioWorklet-compatible module. The
worklet example in `examples/worklet-demo/` demonstrates the required build
setup and global polyfills for `@csound/browser`.

### Main-thread AudioWorklet setup

```js
// main-libcsound.js

export async function startLibcsoundExample() {
  const frequencies = [220, 330, 440];
  const context = new AudioContext();

  await context.audioWorklet.addModule(new URL("./libcsound.processor.bundle.js", import.meta.url));

  const node = new AudioWorkletNode(context, "multi-csound-libcsound", {
    numberOfInputs: 0,
    numberOfOutputs: frequencies.length,
    outputChannelCount: frequencies.map(() => 2),

    processorOptions: {
      sampleRate: context.sampleRate,
      ksmps: 64,
      frequencies,
    },
  });

  const routes = frequencies.map((frequency, outputIndex) => {
    const gain = context.createGain();
    const pan = context.createStereoPanner();

    gain.gain.value = 0.25;
    pan.pan.value = (outputIndex - 1) * 0.6;

    // Route CSOUND* number outputIndex through the Web Audio graph.
    node.connect(gain, outputIndex, 0);
    gain.connect(pan);
    pan.connect(context.destination);

    return { frequency, gain, pan };
  });

  node.port.onmessage = ({ data }) => {
    if (data.type === "ready") {
      console.log(`${data.voiceCount} Csound instances running`);
    } else if (data.type === "error") {
      console.error("Csound worklet failed:", data.message);
    }
  };

  await context.resume();

  return {
    context,
    node,
    routes,

    async stop() {
      node.port.postMessage({ type: "stop" });
      node.disconnect();

      for (const { gain, pan } of routes) {
        gain.disconnect();
        pan.disconnect();
      }

      await context.close();
    },
  };
}
```

The worklet outputs are independent. Connecting all three routes to
`context.destination` causes Web Audio to mix them. An application may instead
connect each output to a different processing chain or destination.

## Approach 2: instantiate `@csound/wasm-bin` directly

Install the binary package and a browser WASI Preview 1 implementation:

```bash
npm install @csound/wasm-bin @bjorn3/browser_wasi_shim
```

The example below uses
[`@bjorn3/browser_wasi_shim`](https://www.npmjs.com/package/@bjorn3/browser_wasi_shim)
as an independent WASI host. Other WASI implementations can be used instead.

### Load and compile the browser reactor

The `?url` import below is Vite syntax. Other build tools should copy
`lib/csound-no-entry.wasm` to the application's assets and provide its public
URL by the mechanism appropriate to that tool.

```js
// main-wasm-bin.js

import reactorUrl from "@csound/wasm-bin/lib/csound-no-entry.wasm?url";

export async function startRawWasmExample() {
  const frequencies = [220, 330, 440];
  const context = new AudioContext();

  const response = await fetch(reactorUrl);
  if (!response.ok) {
    throw new Error(`Unable to load Csound WASM: ${response.status}`);
  }

  // Compilation happens once, outside the realtime AudioWorklet thread.
  const wasmBytes = await response.arrayBuffer();
  const wasmModule = await WebAssembly.compile(wasmBytes);

  await context.audioWorklet.addModule(new URL("./wasm-bin.processor.bundle.js", import.meta.url));

  const node = new AudioWorkletNode(context, "multi-csound-wasm-bin", {
    numberOfInputs: 0,
    numberOfOutputs: frequencies.length,
    outputChannelCount: frequencies.map(() => 2),

    processorOptions: {
      // WebAssembly.Module supports structured cloning into the worklet.
      wasmModule,
      sampleRate: context.sampleRate,
      ksmps: 64,
      frequencies,
    },
  });

  const routes = frequencies.map((_frequency, outputIndex) => {
    const gain = context.createGain();
    const pan = context.createStereoPanner();

    gain.gain.value = 0.25;
    pan.pan.value = (outputIndex - 1) * 0.6;

    node.connect(gain, outputIndex, 0);
    gain.connect(pan);
    pan.connect(context.destination);

    return { gain, pan };
  });

  node.port.onmessage = ({ data }) => {
    if (data.type === "ready") {
      console.log(`${data.voiceCount} raw Csound instances running`);
    } else if (data.type === "log") {
      console.log("[Csound]", data.message);
    } else if (data.type === "error") {
      console.error("Csound worklet failed:", data.message);
    }
  };

  await context.resume();

  return {
    context,
    node,
    routes,

    async stop() {
      node.port.postMessage({ type: "stop" });
      node.disconnect();

      for (const { gain, pan } of routes) {
        gain.disconnect();
        pan.disconnect();
      }

      await context.close();
    },
  };
}
```

The compiled `WebAssembly.Module` is reusable. Passing the module into one
worklet and instantiating it once produces one live WASM instance. Calling
`csoundCreateWasi()` repeatedly inside that instance produces the separate
Csound engines.

### Supply WASI and the Csound host imports

The raw module does not accept JavaScript strings and cannot be instantiated
with an empty imports object. The following loader provides:

- imported WebAssembly memory;
- WASI Preview 1 functions and a small in-memory filesystem;
- Csound message and debug callbacks;
- no-op dynamic module callbacks for a no-plugin setup;
- C-string allocation and marshalling; and
- a small JavaScript adapter around the raw C exports.

```js
// wasm-bin.processor.js

import { WASI, File, OpenFile, ConsoleStdout, PreopenDirectory } from "@bjorn3/browser_wasi_shim";

import { makeMultiCsoundProcessor } from "./multi-csound-processor.js";

const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder();

async function loadRawCsound(config, port) {
  if (!(config.wasmModule instanceof WebAssembly.Module)) {
    throw new TypeError("processorOptions.wasmModule is required");
  }

  const fds = [
    new OpenFile(new File([])),

    ConsoleStdout.lineBuffered((message) => {
      port.postMessage({ type: "log", message });
    }),

    ConsoleStdout.lineBuffered((message) => {
      port.postMessage({ type: "log", message });
    }),

    // Empty in-memory filesystem rooted at "/".
    new PreopenDirectory("/", []),
  ];

  const wasi = new WASI(["csound-no-entry.wasm"], [], fds, { debug: false });

  const pagesPerMiB = 16;

  // This mirrors the current no-plugin reservation in Csound's browser
  // loader. A direct host becomes responsible for tracking future changes
  // to memory sizing.
  const memory = new WebAssembly.Memory({
    initial: 128 * pagesPerMiB,
    maximum: 1024 * pagesPerMiB,
  });

  const env = {
    memory,

    __memory_base: new WebAssembly.Global({ value: "i32", mutable: false }, 128 * pagesPerMiB),

    __table_base: new WebAssembly.Global({ value: "i32", mutable: false }, 1),

    csoundLoadModules() {
      // This minimal example does not load dynamic WASM plugins.
      return 0;
    },

    csoundLoadExternals() {
      return 0;
    },

    csoundWasiJsMessageCallback(_csound, _attribute, length, pointer) {
      const bytes = new Uint8Array(memory.buffer, pointer, length);
      port.postMessage({
        type: "log",
        message: textDecoder.decode(bytes),
      });
    },

    csoundWasiJsDebugCallback() {},

    printDebugCallback(pointer, length) {
      const bytes = new Uint8Array(memory.buffer, pointer, length);
      port.postMessage({
        type: "log",
        message: textDecoder.decode(bytes),
      });
    },
  };

  const instance = await WebAssembly.instantiate(config.wasmModule, {
    wasi_snapshot_preview1: wasi.wasiImport,
    env,
  });

  // The Csound module imports memory instead of exporting it. Supply a
  // facade containing that memory to the WASI implementation.
  const exports = Object.assign({}, instance.exports, { memory });
  const runtime = { exports };

  // csound-no-entry.wasm is a reactor: initialize it, do not call _start.
  wasi.initialize(runtime);

  exports.__wasi_js_csoundSetMessageStringCallback?.();

  function withCString(value, callback) {
    const bytes = textEncoder.encode(value);
    const pointer = exports.allocStringMem(bytes.length);

    if (!pointer) {
      throw new Error("allocStringMem() failed");
    }

    try {
      new Uint8Array(memory.buffer, pointer, bytes.length).set(bytes);
      return callback(pointer);
    } finally {
      exports.freeStringMem(pointer);
    }
  }

  // Adapt the raw exports to the interface used by the shared processor.
  return {
    csoundCreate() {
      return exports.csoundCreateWasi();
    },

    csoundDestroy(csound) {
      exports.csoundDestroy(csound);
    },

    csoundSetOption(csound, option) {
      return withCString(option, (pointer) => exports.csoundSetOption(csound, pointer));
    },

    csoundCompileOrc(csound, orchestra) {
      return withCString(orchestra, (pointer) => exports.csoundCompileOrc(csound, pointer));
    },

    csoundStart(csound) {
      return exports.csoundStartWasi(csound);
    },

    csoundPerformKsmps(csound) {
      return exports.csoundPerformKsmps(csound);
    },

    csoundGetKsmps(csound) {
      return exports.csoundGetKsmps(csound);
    },

    csoundGetNchnls(csound) {
      return exports.csoundGetChannels(csound, 0);
    },

    csoundGet0dBFS(csound) {
      return exports.csoundGet0dBFS(csound);
    },

    csoundGetSpout(csound) {
      return exports.csoundGetSpout(csound);
    },

    getMemory() {
      return memory;
    },
  };
}

registerProcessor("multi-csound-wasm-bin", makeMultiCsoundProcessor(loadRawCsound));
```

Bundle `wasm-bin.processor.js`, `multi-csound-processor.js`, and the WASI shim
into the module loaded by `audioWorklet.addModule()`. Confirm that the chosen
WASI implementation and its globals are available in the target browser's
`AudioWorkletGlobalScope`; a worklet bundle may need `TextEncoder`,
`TextDecoder`, or other small environment polyfills.

## Direct-host responsibilities

The direct `@csound/wasm-bin` example deliberately implements only enough
host behavior to demonstrate several Csound instances rendering into Web
Audio. A production host must decide how to provide the following features:

- persistent files or preloaded sound files;
- stdout, stderr, and Csound message buffering;
- WASM plugin memory and table allocation;
- plugin instantiation and `csoundLoadModules()` behavior;
- MIDI input and per-instance MIDI routing;
- audio input through each instance's spin buffer;
- handling of WebAssembly memory growth; and
- compatibility with changes to the module's imports and memory requirements.

The direct host should inspect `WebAssembly.Module.imports()` and fail with a
clear error if a package update introduces an import it does not implement.

## Isolation limitations

Multiple `CSOUND*` pointers provide separate Csound orchestras, scores,
tables, spin buffers, and spout buffers. They do not provide separate WASM
heaps or module globals. Everything created from one
`WebAssembly.Instance` shares:

- linear memory;
- WebAssembly globals and function tables;
- the WASI filesystem;
- imported callbacks; and
- any C globals compiled into the browser-hosted module.

The current WASM-specific MIDI queue is module-global, for example. A host
that depends on MIDI isolation must account for that limitation or use
separate WebAssembly instances.

A live `WebAssembly.Instance` also cannot be shared transparently between
independent workers or worklet global scopes. Sharing several Csound pointers
is practical only when all of them execute in the same JavaScript context.

## Choosing an approach

Use `libcsound()` when the application wants direct C API access while
retaining Csound's existing browser bootstrap, filesystem, callbacks, plugin
support, and memory setup.

Use `csound-no-entry.wasm` directly when the application needs full ownership
of the WASI host and accepts responsibility for maintaining that integration.

If isolated memory is more important than sharing live runtime state, compile
one `WebAssembly.Module` and instantiate it multiple times. This still avoids
repeating WebAssembly compilation while giving every Csound runtime its own
memory and globals.
