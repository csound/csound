import assert from "node:assert/strict";
import { Worker } from "node:worker_threads";
import { setTimeout as delay } from "node:timers/promises";
import { AUDIO_STATE, initialSharedState } from "../../src/constants.js";
import { stopSAB, wakeSAB } from "../../src/utils/sab-transport.js";

describe("SAB output demand", () => {
  for (const early of [false, true]) {
    for (const reason of ["audio", "callback", "pause", "stop"]) {
      it(`wakes for ${reason}, including before wait (early: ${early})`, async () => {
        const state = new Int32Array(new SharedArrayBuffer(initialSharedState.length * 4));
        state.set(initialSharedState);
        Atomics.store(state, AUDIO_STATE.IS_PERFORMING, 1);
        const gate = new Int32Array(new SharedArrayBuffer(4));
        const worker = new Worker(new URL("./fixtures/sab-output-worker.js", import.meta.url), {
          workerData: { state: state.buffer, gate: gate.buffer, holdBeforeWait: early },
        });
        let armed;
        const ready = new Promise((resolve) => {
          armed = resolve;
        });
        let finished = false;
        const done = new Promise((resolve, reject) => {
          worker.on("error", reject);
          worker.on("message", (event) => {
            if (event === "armed") armed();
            if (event === "done") {
              finished = true;
              resolve();
            }
          });
        });
        try {
          await ready;
          assert.equal(Atomics.load(state, AUDIO_STATE.CSOUND_LOCK), 1);
          if (!early) {
            const deadline = Date.now() + 1000;
            while (Atomics.notify(state, AUDIO_STATE.CSOUND_LOCK) === 0) {
              assert.ok(Date.now() < deadline, "producer must really sleep");
              await delay(1);
            }
            await delay(20);
            assert.equal(finished, false, "a spurious wake cannot advance output");
          }
          if (reason === "audio") Atomics.store(state, AUDIO_STATE.OUTPUT_READ_INDEX, 128);
          if (reason === "callback") Atomics.store(state, AUDIO_STATE.HAS_PENDING_CALLBACKS, 1);
          if (reason === "pause") Atomics.store(state, AUDIO_STATE.IS_PAUSED, 1);
          if (reason === "stop") stopSAB(state);
          else wakeSAB(state);
          Atomics.store(gate, 0, 1);
          Atomics.notify(gate, 0);
          await done;
          assert.equal(Atomics.load(state, AUDIO_STATE.CSOUND_LOCK), 0);
        } finally {
          await worker.terminate();
        }
      });
    }
  }
});
