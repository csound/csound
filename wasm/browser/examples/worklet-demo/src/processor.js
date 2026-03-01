/**
 * Csound UGEN AudioWorklet Processor
 *
 * This file is bundled by esbuild (via the Vite plugin) into a single IIFE
 * that runs in AudioWorkletGlobalScope.  The polyfills for window/navigator
 * are injected by the esbuild banner *before* this code — ensuring csound.js's
 * DOM-probing IIFE is safely skipped.
 *
 * Architecture:
 *   - libcsound wasm is compiled & instantiated inside the worklet (async)
 *   - csoundUgenGraphPerform() runs directly in process()
 *   - ksmps = 128 = Web Audio rendering quantum → one perform per process() call
 *   - Parameter changes arrive via port.postMessage()
 *   - No SharedArrayBuffer, no ring buffer, no main-thread rendering
 */

import { libcsound } from "@csound/browser";

// Cache the libcsound promise so it's shared if multiple nodes are created.
let csPromise = null;

class CsoundUgenProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    const { sr, ksmps, freq, amp } = options.processorOptions;
    this.sr = sr;
    this.ksmps = ksmps;
    this.freq = freq;
    this.amp = amp;
    this.ready = false;
    this.running = true;

    this.port.onmessage = (e) => {
      const { type, value } = e.data;
      switch (type) {
        case "freq":
          this.freq = value;
          break;
        case "amp":
          this.amp = value;
          break;
        case "stop":
          this.running = false;
          this._cleanup();
          break;
      }
    };

    this._init();
  }

  async _init() {
    try {
      if (!csPromise) csPromise = libcsound();
      const cs = await csPromise;

      const csound = cs.csoundCreate();
      cs.csoundSetOption(csound, "-d");
      cs.csoundSetOption(csound, "-n");
      cs.csoundSetOption(csound, "--nchnls=1");
      cs.csoundSetOption(csound, "--0dbfs=1");
      cs.csoundSetOption(csound, `--sr=${this.sr}`);
      cs.csoundSetOption(csound, `--ksmps=${this.ksmps}`);
      // Minimal orchestra so csoundStart() succeeds
      cs.csoundCompileOrc(csound, "instr 1\n  out oscili(p4,p5)\nendin");
      cs.csoundStart(csound);

      const factory = cs.csoundUgenFactoryNew(csound);
      const osc = cs.csoundUgenNew(factory, "oscili", "a", "kkjo");
      if (osc === 0) {
        throw new Error("csoundUgenNew('oscili','a','kkjo') returned 0");
      }

      cs.csoundUgenSetValue(osc, 0, this.amp);
      cs.csoundUgenSetValue(osc, 1, this.freq);

      const graph = cs.csoundUgenGraphNew(factory);
      cs.csoundUgenGraphAdd(graph, osc);
      cs.csoundUgenGraphInit(graph);

      this.cs = cs;
      this.csound = csound;
      this.factory = factory;
      this.osc = osc;
      this.graph = graph;
      this.outVar = cs.csoundUgenGetOutVar(osc, 0);
      this.ready = true;

      this.port.postMessage({ type: "ready" });
    } catch (err) {
      console.error("CsoundUgenProcessor init error:", err);
      this.port.postMessage({ type: "error", message: err.message });
    }
  }

  _cleanup() {
    if (this.cs && this.graph) {
      this.cs.csoundUgenGraphDeleteAll(this.graph);
      this.cs.csoundUgenFactoryDelete(this.factory);
      this.cs.csoundStop(this.csound);
      this.cs.csoundDestroy(this.csound);
      this.cs = null;
    }
  }

  process(_inputs, outputs) {
    if (!this.running) return false;
    if (!this.ready) return true; // output silence while wasm loads

    const cs = this.cs;
    const output = outputs[0];

    // Update parameters before rendering
    cs.csoundUgenSetValue(this.osc, 0, this.amp);
    cs.csoundUgenSetValue(this.osc, 1, this.freq);

    // Render one ksmps block (ksmps=128 = rendering quantum)
    cs.csoundUgenGraphPerform(this.graph);
    const samples = cs.csoundUgenVarGetFloat64Array(this.outVar);

    // Copy to output (mono source duplicated to all channels)
    for (let ch = 0; ch < output.length; ch++) {
      for (let i = 0; i < output[ch].length; i++) {
        output[ch][i] = samples[i];
      }
    }

    return true;
  }
}

registerProcessor("csound-ugen-processor", CsoundUgenProcessor);
