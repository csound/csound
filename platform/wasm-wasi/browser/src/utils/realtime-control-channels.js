import { pointerSize, readPointer } from "./wasm64.js";
import { ptr2string } from "./string-pointers.js";

export function createRealtimeControlChannels(wasm, csound) {
  const pointer = wasm.exports["csoundGetControlChannelsWasi"](csound);
  if (!pointer) throw new Error("Could not snapshot numeric control channels before playback");
  const channels = new Map();
  const sampleBytes = wasm.exports["csoundGetSizeOfMYFLT"]();
  const Integer = sampleBytes === 8 ? BigInt64Array : Int32Array;
  try {
    if (sampleBytes !== 4 && sampleBytes !== 8) throw new Error("Unsupported Csound sample width");
    const width = pointerSize(wasm);
    const buffer = wasm.wasi.memory.buffer;
    const view = new DataView(buffer);
    const count = readPointer(wasm, view, pointer);
    for (let index = 0; index < count; index++) {
      const entry = pointer + width * (1 + index * 2);
      channels.set(
        ptr2string(wasm, readPointer(wasm, view, entry)),
        new Integer(buffer, readPointer(wasm, view, entry + width), 1),
      );
    }
  } finally {
    wasm.exports["freeByteMem"](pointer);
  }
  const scratch = new ArrayBuffer(sampleBytes);
  const value = sampleBytes === 8 ? new Float64Array(scratch) : new Float32Array(scratch);
  const bits = new Integer(scratch);
  const channel = (name) => {
    const slot = channels.get(name);
    if (!slot) throw new Error(`Declare numeric control channel "${name}" before start`);
    return slot;
  };
  // Shared builds require atomic builtins: chnget/chnset and the numeric C API
  // use sample-width atomic accesses. This worker runs JS calls between performKsmps
  // calls; direct pointer access from other threads must also use atomics.
  // Alignment alone does not make a plain Wasm store atomic.
  return {
    has: (name) => channels.has(name),
    get: (name) => {
      bits[0] = Atomics.load(channel(name), 0);
      return value[0];
    },
    set: (name, next) => {
      value[0] = next;
      Atomics.store(channel(name), 0, bits[0]);
    },
  };
}
