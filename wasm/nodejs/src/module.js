import fs from "fs";
import path from "path";
import { Readable } from "stream";
import { createRequire } from "module";
import { promisify } from "util";
import { csoundWasiJsMessageCallback } from "./filesystem.js";
import { WASI } from "@wasmer/wasi";
import { default as wasiBindings } from "@wasmer/wasi/lib/bindings/node.js";
import { lowerI64Imports } from "@wasmer/wasm-transformer";
import { WasmFs } from "@wasmer/wasmfs";
import Speaker from "speaker";

const require = createRequire(import.meta.url);
const readFilePromise = promisify(fs.readFile);

const wasmPath = require.resolve("@csound/wasm/lib/libcsound.wasm");
const PAGE_SIZE = 65536;

const getBinaryHeaderData = (wasmBytes) => {
  const magicBytes = new Uint32Array(new Uint8Array(wasmBytes.subarray(0, 24)).buffer);
  if (magicBytes[0] !== 0x6d736100) {
    console.error("Wasm magic number is missing!");
  }
  if (wasmBytes[8] !== 0) {
    log("Dylink section wasn't found in wasm binary, assuming static wasm.");
    return "static";
  }

  let next = 9;
  function getLEB() {
    let returnValue = 0;
    let mul = 1;
    while (1) {
      const byte = wasmBytes[next++];
      returnValue += (byte & 0x7f) * mul;
      mul *= 0x80;
      if (!(byte & 0x80)) break;
    }
    return returnValue;
  }
  const sectionSize = getLEB();
  // 6, size of "dylink" string = 7
  next += 7;
  const memorySize = getLEB();
  const memoryAlign = getLEB();
  const tableSize = getLEB();
  const tableAlign = getLEB();
  const neededDynlibsCount = getLEB();
  return { sectionSize, memorySize, memoryAlign, neededDynlibsCount, tableSize, tableAlign };
};

export default async function ({ withPlugins = [] }) {
  const fileBuffer = await readFilePromise(wasmPath);
  const { memorySize, memoryAlign, tableSize } = getBinaryHeaderData(fileBuffer);

  const wasi = new WASI({
    // --dir <wasm path>:<host path>
    preopens: {
      "/": "/",
    },
    env: {},
    wasiBindings,
  });
  const loweredFileBuffer = await lowerI64Imports(fileBuffer);
  const module = await WebAssembly.compile(loweredFileBuffer);
  const options = wasi.getImports(module);

  const fixedMemoryBase = 512;
  const initialMemory = Math.ceil((memorySize + memoryAlign) / PAGE_SIZE);
  const pluginsMemory = 0;
  const totalInitialMemory = initialMemory + pluginsMemory + fixedMemoryBase;

  const memory = new WebAssembly.Memory({
    initial: totalInitialMemory,
  });

  const table = new WebAssembly.Table({ initial: tableSize + 1, element: "anyfunc" });

  const stackPointer = new WebAssembly.Global(
    { value: "i32", mutable: true },
    totalInitialMemory * PAGE_SIZE,
  );
  const heapBase = new WebAssembly.Global(
    { value: "i32", mutable: true },
    totalInitialMemory * PAGE_SIZE,
  );
  const memoryBase = new WebAssembly.Global({ value: "i32", mutable: false }, fixedMemoryBase);
  const tableBase = new WebAssembly.Global({ value: "i32", mutable: false }, 1);
  const __dummy = new WebAssembly.Global({ value: "i32", mutable: true }, 0);

  options.env = options.env || {};
  options.env.memory = memory;
  options.env.__indirect_function_table = table;
  options.env.__stack_pointer = stackPointer;
  options.env.__memory_base = memoryBase;
  options.env.__table_base = tableBase;
  options.env.csoundLoadModules = () => {};

  const streamBuffer = [];
  const messagePort = {};
  options.env.csoundWasiJsMessageCallback = csoundWasiJsMessageCallback({
    memory,
    streamBuffer,
    messagePort,
  });

  options["GOT.mem"] = options["GOT.mem"] || {};
  options["GOT.mem"].__heap_base = heapBase;
  options["GOT.func"] = options["GOT.func"] || {};

  const instance = await WebAssembly.instantiate(module, options);

  const moduleExports = Object.assign({}, instance.exports);
  const instance_ = {};
  instance_.exports = Object.assign(moduleExports, {
    memory,
  });

  wasi.start(instance_);
  instance.exports.__wasi_js_csoundSetMessageStringCallback();
  return instance;
}
