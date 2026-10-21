/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

/* eslint-disable object-shorthand -- Wire keys cross separately compiled worker bundles. */

export const WASI_THREAD_IDLE = 1;
export const WASI_THREAD_RESERVED = 2;
export const WASI_THREAD_RUNNING = 3;
export const WASI_THREAD_STOPPED = 4;
export const WASI_THREAD_STRIDE = 3;
const MAX_THREAD_ID = 0x1fffffff;
const EAGAIN = 6;

export function stopWasiThreads(control, failed = false) {
  if (failed) Atomics.store(control, 0, 2);
  else Atomics.compareExchange(control, 0, 0, 1);
  Atomics.notify(control, 0);
  for (let slot = 1; slot < control.length; slot += WASI_THREAD_STRIDE) {
    Atomics.store(control, slot, WASI_THREAD_STOPPED);
    Atomics.notify(control, slot);
  }
}

/**
 * Prepare all instances before playback. thread-spawn only reserves a ready slot
 * and publishes its argument; it never waits for JavaScript in another worker.
 * The caller owns the Workers so it can also kill a render worker stuck in join.
 */
export function createWasiThreadHost({
  module,
  memory,
  plugins = [],
  initializationStackTops = [],
  createWorker,
  onError = () => {},
  poolSize = 4,
  clockOrigin = performance.timeOrigin,
  readyTimeout = 30000,
}) {
  if (!(memory.buffer instanceof SharedArrayBuffer))
    throw new TypeError("WASI threads require shared WebAssembly memory");
  if (!Number.isInteger(poolSize) || poolSize < 1 || poolSize > 32)
    throw new RangeError("WASI thread pool size must be between 1 and 32");
  const control = new Int32Array(new SharedArrayBuffer((1 + poolSize * WASI_THREAD_STRIDE) * 4));
  const workers = [];
  let nextThreadId = 2;
  let disposed = false;
  let failed = false;
  let readyCount = 0;
  let readyResolve;
  let readyReject;
  const ready = new Promise((resolve, reject) => {
    readyResolve = resolve;
    readyReject = reject;
  });
  const dispose = () => {
    if (disposed) return;
    disposed = true;
    clearTimeout(timer);
    stopWasiThreads(control);
    for (const worker of workers) worker.terminate();
    readyReject(new Error("WASI thread host was disposed"));
  };
  const fail = (error) => {
    if (disposed || failed) return;
    failed = true;
    stopWasiThreads(control, true);
    readyReject(error);
    dispose();
    onError(error);
  };
  const timer = setTimeout(
    () => fail(new Error("WASI thread workers did not become ready")),
    readyTimeout,
  );
  try {
    for (let index = 0; index < poolSize; index++) {
      const worker = createWorker(index);
      workers.push(worker);
      let workerReady = false;
      worker.addEventListener("message", ({ data }) => {
        if (disposed) return;
        switch (data["type"]) {
          case "ready": {
            if (workerReady) return;
            workerReady = true;
            if (++readyCount === poolSize) {
              clearTimeout(timer);
              readyResolve();
            }
            break;
          }
          case "fatal": {
            fail(new Error(data["message"]));
            break;
          }
          default: {
            break;
          }
        }
      });
      worker.addEventListener("error", (event) =>
        fail(new Error(event.message || "WASI thread worker failed")),
      );
      worker.addEventListener("messageerror", () =>
        fail(new Error("WASI thread worker sent an unreadable message")),
      );
      worker.postMessage({
        "type": "initialize",
        "module": module,
        "memory": memory,
        "plugins": plugins,
        "initializationStackTop": initializationStackTops[index] || 0,
        "control": control.buffer,
        "index": index,
        "clockOrigin": clockOrigin,
      });
      if (worker.start) worker.start();
    }
  } catch (error) {
    fail(error);
  }

  const spawn = (argument) => {
    if (
      disposed ||
      readyCount !== poolSize ||
      Atomics.load(control, 0) !== 0 ||
      nextThreadId > MAX_THREAD_ID
    )
      return -EAGAIN;
    for (let index = 0; index < poolSize; index++) {
      const slot = 1 + index * WASI_THREAD_STRIDE;
      if (
        Atomics.compareExchange(control, slot, WASI_THREAD_IDLE, WASI_THREAD_RESERVED) !==
        WASI_THREAD_IDLE
      )
        continue;
      const releaseReservation = () => {
        Atomics.compareExchange(control, slot, WASI_THREAD_RESERVED, WASI_THREAD_IDLE);
        Atomics.notify(control, slot);
        return -EAGAIN;
      };
      const tid = nextThreadId;
      Atomics.store(control, slot + 1, tid);
      Atomics.store(control, slot + 2, argument);
      // Stop may race reservation. Do not turn a stopped slot back into a run.
      if (Atomics.load(control, 0) !== 0) return releaseReservation();
      if (
        Atomics.compareExchange(control, slot, WASI_THREAD_RESERVED, WASI_THREAD_RUNNING) !==
        WASI_THREAD_RESERVED
      )
        return releaseReservation();
      nextThreadId += 1;
      Atomics.notify(control, slot);
      return tid;
    }
    return -EAGAIN;
  };
  const threadSpawn = (argument, resultPointer) => {
    const result = spawn(argument);
    if (resultPointer === undefined) return result;
    // wasi-libc supports both the original return-value ABI and its struct ABI.
    const view = new DataView(memory.buffer);
    view.setUint32(resultPointer, result < 0 ? 1 : 0, true);
    view.setUint32(resultPointer + 4, Math.abs(result), true);
    return undefined;
  };
  return { ready, threadSpawn, dispose, control };
}

// The render loop runs after start() returns. A rejected loop must reach the
// UI supervisor; an unhandled Promise does not raise a Worker error event.
export const superviseWasiPerformance = (perform, onError) => async (payload) => {
  try {
    await perform(payload);
  } catch (error) {
    onError(error);
  }
};
