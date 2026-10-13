/* eslint-env mocha */
import assert from "node:assert/strict";
import { Worker } from "node:worker_threads";
import { setTimeout as delay } from "node:timers/promises";
import { AUDIO_STATE, initialSharedState } from "../../src/constants.js";
import { stopSAB } from "../../src/utils/sab-transport.js";

const workers = [];
function pausedWorker(holdBeforeWait = false) {
  const state = new Int32Array(new SharedArrayBuffer(initialSharedState.length * 4));
  state.set(initialSharedState);
  Atomics.store(state, AUDIO_STATE.IS_PAUSED, 1);
  Atomics.store(state, AUDIO_STATE.IS_PERFORMING, 1);
  const gate = new Int32Array(new SharedArrayBuffer(4));
  const progress = new Int32Array(new SharedArrayBuffer(4));
  const worker = new Worker(new URL("./fixtures/sab-pause-worker.js", import.meta.url), {
    workerData: {
      state: state.buffer,
      gate: gate.buffer,
      progress: progress.buffer,
      holdBeforeWait,
    },
  });
  workers.push(worker);
  const events = [];
  let paused;
  const ready = new Promise((resolve) => {
    paused = resolve;
  });
  const done = new Promise((resolve, reject) => {
    worker.on("error", reject);
    worker.on("message", (event) => {
      events.push(event);
      if (event === "paused") paused();
      if (event === "done") resolve();
    });
  });
  const releaseGate = () => {
    Atomics.store(gate, 0, 1);
    Atomics.notify(gate, 0);
  };
  const resume = () => {
    Atomics.store(state, AUDIO_STATE.IS_PAUSED, 0);
    Atomics.notify(state, AUDIO_STATE.IS_PAUSED);
  };
  return { state, progress, events, ready, done, resume, releaseGate };
}

describe("SAB pause/wakeup", () => {
  afterEach(async () => {
    await Promise.all(workers.splice(0).map((worker) => worker.terminate()));
  });

  it("does not advance while paused, including a notification without a state change", async () => {
    const run = pausedWorker();
    await run.ready;
    // Confirm the notification reached a sleeping worker, not just an early signal.
    let sleeping = false;
    const deadline = Date.now() + 1000;
    while (
      !(sleeping = Atomics.notify(run.state, AUDIO_STATE.IS_PAUSED) > 0) &&
      Date.now() < deadline
    ) {
      await delay(1);
    }
    assert.ok(sleeping, "the paused worker must sleep until its state changes");
    await delay(30);
    assert.equal(Atomics.load(run.progress, 0), 0);
    assert.deepEqual(run.events, ["paused"]);
    run.resume();
    await run.done;
    assert.equal(Atomics.load(run.progress, 0), 1);
    assert.deepEqual(run.events, ["paused", "resumed", "done"]);
  });

  it("cannot miss a resume that arrives before entering wait", async () => {
    const run = pausedWorker(true);
    await run.ready;
    run.resume();
    run.releaseGate();
    await run.done;
    assert.deepEqual(run.events, ["paused", "resumed", "done"]);
  });

  for (const early of [false, true]) {
    it(`stops a paused worker without announcing resume (early: ${early})`, async () => {
      const run = pausedWorker(early);
      await run.ready;
      stopSAB(run.state);
      run.releaseGate();
      await run.done;
      assert.equal(Atomics.load(run.progress, 0), 0);
      assert.deepEqual(run.events, ["paused", "done"]);
      assert.equal(Atomics.load(run.state, AUDIO_STATE.IS_PERFORMING), 0);
      assert.equal(Atomics.load(run.state, AUDIO_STATE.STOP), 1);
      assert.equal(Atomics.load(run.state, AUDIO_STATE.IS_PAUSED), 0);
      assert.equal(Atomics.load(run.state, AUDIO_STATE.CSOUND_LOCK), 0);
      assert.equal(Atomics.wait(run.state, AUDIO_STATE.CSOUND_LOCK, 1, 1), "not-equal");
    });
  }
});
