import { parentPort, workerData } from "node:worker_threads";
import { waitForSABOutput } from "../../../src/utils/sab-transport.js";
const state = new Int32Array(workerData.state);
const gate = new Int32Array(workerData.gate);
const wait = Atomics.wait;
let firstWait = true;
Atomics.wait = (...args) => {
  if (firstWait) {
    firstWait = false;
    parentPort.postMessage("armed");
    if (workerData.holdBeforeWait) while (Atomics.load(gate, 0) === 0) wait(gate, 0, 0);
  }
  return wait(...args);
};
waitForSABOutput(state, 0);
parentPort.postMessage("done");
