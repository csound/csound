import { parentPort, workerData } from "node:worker_threads";
import { waitForSABResume } from "../../../src/utils/sab-transport.js";

const state = new Int32Array(workerData.state);
const gate = new Int32Array(workerData.gate);
const progress = new Int32Array(workerData.progress);
waitForSABResume(
  state,
  () => {
    parentPort.postMessage("paused");
    // Hold the worker before the production wait to force an early wakeup.
    if (workerData.holdBeforeWait) {
      while (Atomics.load(gate, 0) === 0) Atomics.wait(gate, 0, 0);
    }
  },
  () => {
    Atomics.add(progress, 0, 1);
    parentPort.postMessage("resumed");
  },
);
parentPort.postMessage("done");
