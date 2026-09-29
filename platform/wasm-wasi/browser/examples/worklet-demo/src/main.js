/**
 * Main-thread UI for the Csound UGEN AudioWorklet demo.
 *
 * This file runs on the main thread.  It creates an AudioContext,
 * loads the bundled worklet processor (/processor.js — served by the
 * Vite plugin as an esbuild-bundled IIFE), and forwards slider changes
 * to the worklet via MessagePort.
 */

const logEl = document.getElementById("log");
const startBtn = document.getElementById("startBtn");
const stopBtn = document.getElementById("stopBtn");
const freqSlider = document.getElementById("freq");
const ampSlider = document.getElementById("amp");
const freqVal = document.getElementById("freqVal");
const ampVal = document.getElementById("ampVal");

function log(msg) {
  logEl.textContent += msg + "\n";
  logEl.scrollTop = logEl.scrollHeight;
}

let audioCtx = null;
let workletNode = null;

startBtn.addEventListener("click", async () => {
  startBtn.disabled = true;
  stopBtn.disabled = false;

  try {
    const sr = 48000;
    const ksmps = 128; // matches Web Audio rendering quantum exactly

    log("Creating AudioContext...");
    audioCtx = new AudioContext({ sampleRate: sr });

    log("Loading worklet processor (bundled libcsound wasm)...");
    await audioCtx.audioWorklet.addModule("/processor.js");

    workletNode = new AudioWorkletNode(audioCtx, "csound-ugen-processor", {
      outputChannelCount: [1],
      processorOptions: {
        sr,
        ksmps,
        freq: Number(freqSlider.value),
        amp: Number(ampSlider.value) / 100,
      },
    });

    workletNode.port.onmessage = (e) => {
      if (e.data.type === "ready") {
        log(
          `UGEN graph running in worklet: oscili(a, kkjo) sr=${sr} ksmps=${ksmps}`,
        );
        log("Audio playing! Adjust sliders to change frequency/amplitude.");
      } else if (e.data.type === "error") {
        log("WORKLET ERROR: " + e.data.message);
      }
    };

    workletNode.connect(audioCtx.destination);
    log("AudioWorklet connected — waiting for wasm init...");

    // ---- Slider controls ----
    freqSlider.oninput = () => {
      const f = Number(freqSlider.value);
      freqVal.textContent = f;
      workletNode.port.postMessage({ type: "freq", value: f });
    };
    ampSlider.oninput = () => {
      const a = Number(ampSlider.value) / 100;
      ampVal.textContent = a.toFixed(2);
      workletNode.port.postMessage({ type: "amp", value: a });
    };

    // ---- Stop handler ----
    stopBtn.onclick = () => {
      stopBtn.disabled = true;
      startBtn.disabled = false;
      log("Stopping...");

      if (workletNode) {
        workletNode.port.postMessage({ type: "stop" });
        workletNode.disconnect();
        workletNode = null;
      }
      if (audioCtx) {
        audioCtx.close();
        audioCtx = null;
      }
      log("Stopped.");
    };
  } catch (err) {
    log("ERROR: " + err.message);
    console.error(err);
    startBtn.disabled = false;
    stopBtn.disabled = true;
  }
});
