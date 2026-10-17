/* Copyright (c) The Csound Developers. Licensed under Apache-2.0. */
import { assertRealtimeCallAllowed, realtimeBackgroundOperations } from "./realtime-api.js";

const encoder = new TextEncoder();

// Only the render worker submits. Each preallocated native slot keeps its text
// until the background worker finishes; submitting never waits for that worker.
export function submitRealtimeApiCall(wasm, csound, apiKey, arguments_, id, generation) {
  assertRealtimeCallAllowed(apiKey, arguments_);
  const operation = realtimeBackgroundOperations.get(apiKey);
  if (!operation) throw new Error("Unsupported background realtime API");
  const texts = operation === 4 ? [arguments_[0], arguments_[1]] : [arguments_[0]];
  const capacity = wasm.exports["csoundRealtimeApiCapacityWasi"]();
  // Reject oversized UTF-16 input before encoding, then check its UTF-8 size.
  if (texts.reduce((size, value) => size + value.length + 1, 0) > capacity)
    throw new RangeError("Realtime API text exceeds the request capacity");
  // Buffer is a peek, not a reservation. Reject a full queue before encoding
  // another large edit on the render worker; validation still precedes writes.
  const pointer = wasm.exports["csoundRealtimeApiBufferWasi"](csound);
  if (!pointer) throw new Error("Realtime API queue is full or stopped");
  const encoded = texts.map((text) => encoder.encode(text));
  const size = encoded.reduce((size, bytes) => size + bytes.length + 1, 0);
  if (size > capacity) throw new RangeError("Realtime API text exceeds the request capacity");
  const output = new Uint8Array(wasm.wasi.memory.buffer, pointer, size);
  let offset = 0;
  for (const bytes of encoded) {
    output.set(bytes, offset);
    offset += bytes.length;
    output[offset++] = 0;
  }
  const status = wasm.exports["csoundRealtimeApiSubmitWasi"](
    csound,
    operation,
    id,
    generation,
    size,
    operation === 6 ? arguments_[1] : 0,
  );
  if (status !== 0) throw new Error(`Realtime API request was rejected (${status})`);
}

export function settleRealtimeApiCompletion(callbacks, payload) {
  const pending = callbacks[payload["id"]];
  if (!pending || pending["generation"] !== payload["generation"]) return false;
  if (payload["status"] === 0) {
    const operation = realtimeBackgroundOperations.get(pending["apiKey"]);
    pending["resolveCallback"](operation === 4 || operation === 6 ? undefined : payload["answer"]);
  } else {
    const message =
      payload["status"] === -2
        ? "Performance stopped before the call ran"
        : `Realtime API request failed (${payload["status"]})`;
    pending["rejectCallback"](new Error(message));
  }
  delete callbacks[payload["id"]];
  return true;
}
