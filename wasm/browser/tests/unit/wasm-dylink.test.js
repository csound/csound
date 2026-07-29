/* eslint-env mocha */

import assert from "node:assert/strict";
import { existsSync, readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

import { getBinaryHeaderData } from "../../src/utils/wasm-dylink.js";

const encodeULEB = (input) => {
  const bytes = [];
  let value = input;
  do {
    let byte = value % 128;
    value = Math.floor(value / 128);
    if (value > 0) {
      byte += 128;
    }
    bytes.push(byte);
  } while (value > 0);
  return bytes;
};

const encodeName = (value) => {
  const bytes = [...new TextEncoder().encode(value)];
  return [...encodeULEB(bytes.length), ...bytes];
};

const customSection = (name, payload) => {
  const contents = [...encodeName(name), ...payload];
  return [0, ...encodeULEB(contents.length), ...contents];
};

const subsection = (type, payload) => [type, ...encodeULEB(payload.length), ...payload];

const wasmWith = (...sections) =>
  new Uint8Array([0, 97, 115, 109, 1, 0, 0, 0, ...sections.flat()]);

describe("WebAssembly dynamic link metadata", () => {
  it("returns empty metadata for a static module", () => {
    assert.deepEqual(getBinaryHeaderData(wasmWith()), {
      hasDylink: false,
      sectionSize: 0,
      memorySize: 0,
      memoryAlign: 0,
      tableSize: 0,
      tableAlign: 0,
      neededDynlibsCount: 0,
      neededDynlibs: [],
    });
  });

  it("parses the legacy dylink layout", () => {
    const payload = [
      ...encodeULEB(1024),
      ...encodeULEB(4),
      ...encodeULEB(3),
      ...encodeULEB(1),
      ...encodeULEB(1),
      ...encodeName("libfoo.so"),
    ];

    assert.deepEqual(getBinaryHeaderData(wasmWith(customSection("dylink", payload))), {
      hasDylink: true,
      sectionSize: encodeName("dylink").length + payload.length,
      memorySize: 1024,
      memoryAlign: 4,
      tableSize: 3,
      tableAlign: 1,
      neededDynlibsCount: 1,
      neededDynlibs: ["libfoo.so"],
    });
  });

  it("parses dylink.0 subsections and skips unknown ones", () => {
    const memoryInfo = [
      ...encodeULEB(2048),
      ...encodeULEB(5),
      ...encodeULEB(7),
      ...encodeULEB(2),
    ];
    const needed = [...encodeULEB(2), ...encodeName("libfoo.so"), ...encodeName("libbar.so")];
    const payload = [
      ...subsection(3, [1, 2, 3]),
      ...subsection(1, memoryInfo),
      ...subsection(2, needed),
    ];

    assert.deepEqual(getBinaryHeaderData(wasmWith(customSection("dylink.0", payload))), {
      hasDylink: true,
      sectionSize: encodeName("dylink.0").length + payload.length,
      memorySize: 2048,
      memoryAlign: 5,
      tableSize: 7,
      tableAlign: 2,
      neededDynlibsCount: 2,
      neededDynlibs: ["libfoo.so", "libbar.so"],
    });
  });

  it("rejects a truncated dylink.0 subsection", () => {
    const payload = [1, 10, 1];
    assert.throws(
      () => getBinaryHeaderData(wasmWith(customSection("dylink.0", payload))),
      /subsection extends past its section/,
    );
  });

  const pluginPath = fileURLToPath(
    new URL("../../../lib/plugin_example_cpp.wasm", import.meta.url),
  );
  const pluginTest = existsSync(pluginPath) ? it : it.skip;

  pluginTest("builds the C++ plugin with a relocatable table", () => {
    const bytes = readFileSync(pluginPath);
    const module = new WebAssembly.Module(bytes);
    const imports = WebAssembly.Module.imports(module);
    const header = getBinaryHeaderData(bytes);

    assert.equal(header.hasDylink, true);
    assert.ok(header.tableSize > 0);
    assert.ok(
      imports.some(
        ({ module: namespace, name, kind }) =>
          namespace === "env" && name === "__table_base" && kind === "global",
      ),
    );
  });
});
