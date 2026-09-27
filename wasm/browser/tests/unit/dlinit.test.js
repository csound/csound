/* eslint-env mocha */

import assert from "node:assert/strict";

import { dlinit } from "../../src/dlinit.js";

const opcodeInit = () => 0;

describe("WASM plugin registration", () => {
  it("reuses host table slots when Csound rebuilds its module state", () => {
    const calls = [];
    const hostInstance = {
      exports: {
        csoundWasiLoadOpcodeLibrary: (...args) => calls.push(args),
      },
    };
    const pluginInstance = { exports: { csound_opcode_init: opcodeInit } };
    const entries = [null];
    const table = {
      get length() {
        return entries.length;
      },
      get: (index) => entries[index],
      grow: (amount) => entries.push(...Array.from({ length: amount }, () => null)),
      set: (index, value) => {
        entries[index] = value;
      },
    };

    dlinit(hostInstance, pluginInstance, table, 1);
    const tableLength = table.length;
    dlinit(hostInstance, pluginInstance, table, 2);

    assert.equal(calls.length, 2);
    assert.equal(calls[0][2].value, calls[1][2].value);
    assert.equal(table.get(calls[0][2].value), opcodeInit);
    assert.equal(table.length, tableLength);
  });
});

// Exercise the fallback for hosts without csoundWasiLoadOpcodeLibrary.
describe("WASM opcode list allocation failure", () => {
  for (const failAt of [1, 2]) {
    it(`stops when allocation ${failAt} fails and frees earlier allocations`, () => {
      const memory = new WebAssembly.Memory({ initial: 1 });
      const freed = [];
      let allocations = 0;
      let initCalls = 0;
      const host = {
        exports: {
          memory,
          allocByteMem: () => (++allocations === failAt ? 0 : 64),
          freeByteMem: (pointer) => freed.push(pointer),
          csoundAppendOpcodes: () => assert.fail("Must not register an unallocated list"),
        },
      };
      const plugin = {
        exports: {
          csound_opcode_init: (_csound, pointer) => {
            initCalls++;
            new DataView(memory.buffer).setUint32(pointer, 128, true);
            return 40; // One wasm32 OENTRY.
          },
        },
      };
      assert.throws(() => dlinit(host, plugin, {}, 1), /Could not allocate/);
      assert.equal(initCalls, failAt - 1);
      assert.deepEqual(freed, failAt === 1 ? [] : [64]);
    });
  }
});
