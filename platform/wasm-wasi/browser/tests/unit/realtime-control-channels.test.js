import assert from "node:assert/strict";

let createRealtimeControlChannels;
let originalGoogDescriptor;
describe("realtime numeric channel snapshot", () => {
  before(async () => {
    originalGoogDescriptor = Object.getOwnPropertyDescriptor(globalThis, "goog");
    Object.defineProperty(globalThis, "goog", {
      configurable: true,
      value: { define: (_name, value) => value },
    });
    ({ createRealtimeControlChannels } =
      await import("../../src/utils/realtime-control-channels.js"));
  });
  after(() => {
    if (originalGoogDescriptor) Object.defineProperty(globalThis, "goog", originalGoogDescriptor);
    else delete globalThis.goog;
  });
  const fixture = (precision = 4) => {
    const memory = new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true });
    new Uint32Array(memory.buffer, 64, 3).set([1, 128, 256]);
    new Uint8Array(memory.buffer, 128, 5).set(new TextEncoder().encode("gain\0"));
    const values = new Float32Array(memory.buffer, 256, 1);
    values[0] = 0.125;
    const freed = [];
    const wasm = {
      wasi: { memory },
      exports: {
        csoundGetControlChannelsWasi: (csound) => {
          assert.equal(csound, 42);
          return 64;
        },
        csoundGetSizeOfMYFLT: () => precision,
        freeByteMem: (pointer) => {
          freed.push(pointer);
        },
      },
    };
    return { wasm, values, freed };
  };
  it("copies names once and uses atomic values without further native calls", () => {
    const { wasm, values, freed } = fixture();
    const controls = createRealtimeControlChannels(wasm, 42);
    assert.deepEqual(freed, [64]);
    wasm.exports = new Proxy(
      {},
      {
        get() {
          assert.fail("live access called native code");
        },
      },
    );
    new Uint8Array(wasm.wasi.memory.buffer, 64, 100).fill(0);
    assert.equal(controls.get("gain"), 0.125);
    controls.set("gain", -0.75);
    assert.equal(values[0], -0.75);
    values[0] = 0.625;
    assert.equal(controls.get("gain"), 0.625);
    assert.throws(() => controls.get("new"), /before start/);
    assert.throws(() => controls.set("new", 1), /before start/);
  });
  it("keeps instance snapshots separate", () => {
    const first = createRealtimeControlChannels(fixture().wasm, 42);
    const second = createRealtimeControlChannels(fixture().wasm, 42);
    first.set("gain", 2);
    assert.equal(second.get("gain"), 0.125);
  });
  it("frees the snapshot if ABI validation fails", () => {
    const { wasm, freed } = fixture(8);
    assert.throws(() => createRealtimeControlChannels(wasm, 42), /single precision/);
    assert.deepEqual(freed, [64]);
    wasm.exports.csoundGetControlChannelsWasi = () => 0;
    assert.throws(() => createRealtimeControlChannels(wasm, 42), /Could not snapshot/);
    assert.deepEqual(freed, [64]);
  });
});
