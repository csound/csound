/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

// Shared memory, atomic loads and standard Wasm exception handling match the
// threaded reactor's ABI. A small module avoids compiling Csound on the UI.
// (module (import "env" "memory" (memory 1 1 shared)) (tag $error)
//   (func i32.const 0 i32.atomic.load drop
//     (block $caught (result exnref)
//       (try_table (catch_all_ref $caught) throw $error) unreachable) throw_ref))
const THREAD_FEATURES = new Uint8Array([
  0, 97, 115, 109, 1, 0, 0, 0, 1, 4, 1, 96, 0, 0, 2, 16, 1, 3, 101, 110, 118, 6, 109, 101, 109, 111,
  114, 121, 2, 3, 1, 1, 3, 2, 1, 0, 13, 3, 1, 0, 0, 10, 24, 1, 22, 0, 65, 0, 254, 16, 2, 0, 26, 2,
  105, 31, 64, 1, 3, 0, 8, 0, 11, 0, 11, 10, 11,
]);

export function canUseSharedWasm() {
  if (typeof WebAssembly === "undefined") return false;
  try {
    // A local shared Wasm heap is allowed without isolation in supported
    // browsers, even when the SharedArrayBuffer constructor is hidden.
    return (
      WebAssembly.validate(THREAD_FEATURES) &&
      new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true }).buffer.byteLength === 65536
    );
  } catch {
    return false;
  }
}

export function canUseThreadedWasm({
  workletSupport = typeof AudioWorkletNode !== "undefined",
  sharedMemorySupport = typeof Atomics !== "undefined" && typeof SharedArrayBuffer !== "undefined",
} = {}) {
  if (
    !workletSupport ||
    !sharedMemorySupport ||
    !globalThis.crossOriginIsolated ||
    typeof SharedArrayBuffer === "undefined" ||
    typeof WebAssembly === "undefined"
  )
    return false;
  try {
    return (
      canUseSharedWasm() &&
      new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true }).buffer instanceof
        SharedArrayBuffer
    );
  } catch {
    return false;
  }
}
