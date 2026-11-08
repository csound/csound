import { ptr2string } from "./string-pointers.js";

export function createRealtimeControlChannels(wasm, csound) {
  const pointer = wasm.exports["csoundGetControlChannelsWasi"](csound);
  if (!pointer) throw new Error("Could not snapshot numeric control channels before playback");
  const channels = new Map();
  try {
    // This browser ABI uses double precision, as do the native atomic channel
    // opcodes. Borrow stable value addresses, never the mutable channel table.
    if (wasm.exports["csoundGetSizeOfMYFLT"]() !== 8)
      throw new Error("Realtime control channels require double precision");
    const buffer = wasm.wasi.memory.buffer;
    const count = new Uint32Array(buffer, pointer, 1)[0];
    const entries = new Uint32Array(buffer, pointer + 4, count * 2);
    for (let index = 0; index < entries.length; index += 2)
      channels.set(
        ptr2string(wasm, entries[index]),
        new BigInt64Array(buffer, entries[index + 1], 1),
      );
  } finally {
    wasm.exports["freeByteMem"](pointer);
  }
  const scratch = new ArrayBuffer(8);
  const value = new Float64Array(scratch);
  const bits = new BigInt64Array(scratch);
  const channel = (name) => {
    const slot = channels.get(name);
    if (!slot) throw new Error(`Declare numeric control channel "${name}" before start`);
    return slot;
  };
  // Shared builds require atomic builtins: chnget/chnset and the numeric C API
  // use 64-bit atomic accesses. This worker runs JS calls between performKsmps
  // calls; direct pointer access from other threads must also use atomics.
  // Alignment alone does not make a plain Wasm store atomic.
  // Closure's Atomics externs only describe numeric typed arrays. These calls
  // use BigInt64Array and must preserve all 64 bits without Number conversion.
  return {
    has: (name) => channels.has(name),
    get: /** @suppress {checkTypes} */ (name) => {
      bits[0] = Atomics.load(channel(name), 0);
      return value[0];
    },
    set: /** @suppress {checkTypes} */ (name, next) => {
      value[0] = next;
      Atomics.store(channel(name), 0, bits[0]);
    },
  };
}
