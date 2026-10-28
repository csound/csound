import assert from "node:assert/strict";
import { deflateSync } from "node:zlib";
import { CS_MIDIDEVICE } from "../../src/structures.js";
import { sizeOfStruct, structLayout } from "../../src/utils/native-sizes.js";

const engine = (pointerBytes, floatBytes, doubleBytes = floatBytes) => ({
  exports: {
    __pointerSize: pointerBytes,
    csoundGetSizeOfCsFloat: () => floatBytes,
    csoundGetSizeOfCsDouble: () => doubleBytes,
  },
});

describe("native structure decoding", () => {
  let decode, midiDevices, loadWasm, Inflate;
  before(async () => {
    const previous = Object.getOwnPropertyDescriptor(globalThis, "goog");
    globalThis.goog = { define: (_name, value) => value, global: globalThis };
    try {
      ({ structBufferToObject: decode } =
        await import("../../src/utils/structure-buffer-to-object.js"));
      ({ csoundGetMIDIDevList: midiDevices } = await import("../../src/modules/rtmidi.js"));
      ({ default: loadWasm } = await import("../../src/module.js"));
      ({ Inflate } = await import("../../src/zlib/inflate.js"));
    } finally {
      if (previous) Object.defineProperty(globalThis, "goog", previous);
      else delete globalThis.goog;
    }
  });

  it("loads the reactor and its decompressor through native ESM imports", () => {
    assert.equal(typeof loadWasm, "function");
    const bytes = new TextEncoder().encode("Csound native ESM loader");
    assert.deepEqual(new Inflate(deflateSync(bytes)).decompress(), bytes);
  });

  for (const [pointerBytes, floatBytes, doubleBytes, offsets, total] of [
    [4, 4, 4, [0, 4, 8, 12, 16, 24], 32],
    [8, 8, 8, [0, 8, 16, 24, 32, 40], 48],
    [4, 4, 8, [0, 4, 8, 16, 20, 24], 32],
  ]) {
    it(`reads aligned C values with ${pointerBytes}/${floatBytes}/${doubleBytes}-byte pointer/sample/score widths`, () => {
      const wasm = engine(pointerBytes, floatBytes, doubleBytes);
      const schema = [
        ["integer", "int"],
        ["sample", "cs_float"],
        ["score", "cs_double"],
        ["pointer", "ptr"],
        ["text", "char", 3],
        ["counter", "uint64"],
      ];
      const layout = structLayout(schema, wasm);
      assert.equal(layout.size, total);
      assert.deepEqual(
        layout.fields.map((field) => field.offset),
        offsets,
      );
      const bytes = new Uint8Array(new ArrayBuffer(total + 7), 7, total);
      const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      view.setInt32(offsets[0], -123456, true);
      view[floatBytes === 4 ? "setFloat32" : "setFloat64"](offsets[1], -0.125, true);
      view[doubleBytes === 4 ? "setFloat32" : "setFloat64"](offsets[2], 0.375, true);
      const pointer = pointerBytes === 8 ? 0x1_0000_0040 : 0xf000_0040;
      if (pointerBytes === 8) view.setBigUint64(offsets[3], BigInt(pointer), true);
      else view.setUint32(offsets[3], pointer, true);
      bytes.set(new TextEncoder().encode("hi"), offsets[4]);
      const counter = (1n << 63n) + 123n;
      view.setBigUint64(offsets[5], counter, true);
      assert.deepEqual(decode(schema, bytes, wasm), {
        integer: -123456,
        sample: -0.125,
        score: 0.375,
        pointer,
        text: "hi",
        counter,
      });
      assert.equal(
        sizeOfStruct(
          [
            ["pointer", "ptr"],
            ["flag", "char"],
          ],
          wasm,
        ),
        pointerBytes * 2,
      );
    });
  }

  for (const width of [4, 8]) {
    it(`reads multiple MIDI records after memory grows on the ${width * 8}-bit engine`, () => {
      const wasm = engine(width, width);
      const memory = new WebAssembly.Memory({ initial: 1 });
      wasm.wasi = { memory };
      const devices = [
        {
          device_name: "A".repeat(100),
          interface_name: "interface A",
          device_id: "device A",
          midi_module: "module A",
          isOutput: 1,
        },
        {
          device_name: "B".repeat(100),
          interface_name: "interface B",
          device_id: "device B",
          midi_module: "module B",
          isOutput: 0,
        },
      ];
      const freed = [];
      Object.assign(wasm.exports, {
        allocCsMidiDeviceStruct: (count) => {
          assert.equal(count, 2);
          memory.grow(1);
          return 65536;
        },
        freeCsMidiDeviceStruct: (pointer) => freed.push(pointer),
        csoundGetMIDIDevList: (_csound, pointer) => {
          if (!pointer) return 2;
          memory.grow(1);
          for (const [index, device] of devices.entries()) {
            const base = pointer + index * 516;
            for (const [field, name] of [
              "device_name",
              "interface_name",
              "device_id",
              "midi_module",
            ].entries()) {
              new Uint8Array(memory.buffer, base + field * 128, 128).set(
                new TextEncoder().encode(device[name]),
              );
            }
            new DataView(memory.buffer).setInt32(base + 512, device.isOutput, true);
          }
          return 2;
        },
      });
      assert.equal(sizeOfStruct(CS_MIDIDEVICE, wasm), 516);
      assert.deepEqual(midiDevices(wasm)(1, true), devices);
      assert.deepEqual(freed, [65536]);
    });
  }
});
