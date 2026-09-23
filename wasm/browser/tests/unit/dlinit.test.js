/* eslint-env mocha */

import assert from "node:assert/strict";

import { dlinit } from "../../src/dlinit.js";

const opcodeInit = () => 0;

describe("WASM plugin registration", () => {
  for (const [hostSampleSize, hostDoubleSize] of [[8, 8], [4, 8], [4, 4]]) {
    for (const info of [undefined, 0, 7 << 16, (7 << 16) | 8,
      (7 << 16) | 4, (7 << 16) | 0x84]) {
      for (const kind of ["module", "opcode", "fgen"]) {
        it(`checks ${kind} precision ${info} against ${hostSampleSize}/${hostDoubleSize} before registration`, () => {
          let called = false;
          const register = () => { called = true; };
          const pluginExports = kind === "module"
            ? { csoundModuleCreate: register, csoundModuleInit: register }
            : kind === "opcode" ? { csound_opcode_init: register }
              : { csound_fgen_init: register };
          if (info !== undefined) {
            pluginExports.csoundModuleInfo = () => info;
          }
          const host = { exports: {
            csoundGetSizeOfCsFloat: () => hostSampleSize,
            csoundGetSizeOfCsDouble: () => hostDoubleSize,
            csoundWasiLoadOpcodeLibrary: register,
          } };
          // Table changes must also follow the compatibility check.
          const table = { length: 1, grow: register, set: register };
          const load = () => dlinit(host, { exports: pluginExports }, table, 1);
          const sampleSize = info & 0x7f;
          const matches = Boolean(info & 0x80) === (hostDoubleSize === 4) &&
            (!sampleSize || sampleSize === hostSampleSize);
          if (matches) {
            load();
            assert.equal(called, true);
          } else {
            assert.throws(load, /Incompatible plugin/);
            assert.equal(called, false);
          }
        });
      }
    }
  }

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
