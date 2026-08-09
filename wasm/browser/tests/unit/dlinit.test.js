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
