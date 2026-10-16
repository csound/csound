/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

import { createWasiThreadImports } from "../utils/wasi-threads-imports.js";
import { withWasiThreadStack } from "../utils/wasi-thread-stack.js";
import { instantiateThreadedPlugins } from "../utils/threaded-plugins.js";
import {
  WASI_THREAD_IDLE,
  WASI_THREAD_RUNNING,
  WASI_THREAD_STRIDE,
  stopWasiThreads,
} from "../utils/wasi-threads.js";

/* eslint-disable object-shorthand -- Wire keys cross separately compiled worker bundles. */

let connected = false;
globalThis.addEventListener("message", ({ data }) => {
  if (connected || data["type"] !== "connect") return;
  connected = true;
  const port = data["port"];
  let initialized = false;
  port.addEventListener("message", async ({ data }) => {
    if (initialized || data["type"] !== "initialize") return;
    initialized = true;
    const control = new Int32Array(data["control"]);
    const slot = 1 + data["index"] * WASI_THREAD_STRIDE;
    try {
      // The render worker may be asleep in Atomics.wait or joining a C thread.
      // Send logs directly to the UI instead of relying on its JS event loop.
      const postMessage = (message) =>
        globalThis.postMessage({ "type": "wasi-thread-message", "message": message });
      const imports = createWasiThreadImports({
        module: data["module"],
        memory: data["memory"],
        control,
        index: data["index"],
        clockOrigin: data["clockOrigin"],
        postMessage,
      });
      const instance = await WebAssembly.instantiate(data["module"], imports);
      if (data["plugins"]?.length && !data["initializationStackTop"])
        throw new Error("Threaded plugins require a private setup stack");
      const setup = (callback) =>
        withWasiThreadStack(instance["exports"], data["initializationStackTop"], callback);
      const pluginRuntime = setup(() =>
        instantiateThreadedPlugins({
          plugins: data["plugins"] || [],
          memory: data["memory"],
          hostExports: instance["exports"],
          index: data["index"],
          importsForModule: (module) =>
            createWasiThreadImports({
              module,
              memory: data["memory"],
              control,
              index: data["index"],
              clockOrigin: data["clockOrigin"],
              postMessage,
            }),
        }),
      );
      const start = instance["exports"]["wasi_thread_start"];
      if (typeof start !== "function")
        throw new TypeError("WASI module does not export wasi_thread_start");
      // Constructors run once in the parent. wasi_thread_start sets this
      // instance's stack and TLS for each C thread before calling its entry.
      if (Atomics.load(control, 0) !== 0) return;
      Atomics.store(control, slot, WASI_THREAD_IDLE);
      port.postMessage({ "type": "ready" });
      while (Atomics.load(control, 0) === 0) {
        const state = Atomics.load(control, slot);
        if (state !== WASI_THREAD_RUNNING) {
          Atomics.wait(control, slot, state);
          continue;
        }
        setup(() => pluginRuntime.initializeTLS());
        start(Atomics.load(control, slot + 1), Atomics.load(control, slot + 2));
        Atomics.store(control, slot, WASI_THREAD_IDLE);
      }
    } catch (error) {
      if (Atomics.load(control, 0) === 1) return;
      stopWasiThreads(control, true);
      const message = error instanceof Error ? error.message : String(error);
      port.postMessage({ "type": "fatal", "message": message });
      // The UI owns every worker. It must see errors even if the parent is
      // blocked inside a Wasm join and cannot service its MessagePort.
      globalThis.postMessage({ "type": "wasi-thread-fatal", "message": message });
    }
  });
  port.start();
});
