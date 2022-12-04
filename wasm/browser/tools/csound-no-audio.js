// using statically built wasm-bin
// we can create a minimal wrapper
// for calling csound without any audio
// bindings

import pako from "pako/dist/pako.min.js";
import wasmDataURI from "../dist/__csound_wasm_static_tools.inline.js";
import libcsoundFactory from "../src/libcsound";
import { WASI } from "../src/filesystem/wasi";
import { uint2String } from "../src/utils/text-encoders";
import { clearArray } from "../src/utils/clear-array";
import { csoundApiRename } from "../src/utils";
import fs from "../src/filesystem/worker-fs";

const fsMethods = Object.keys(fs);

let streamBuffer = [];
let arrayBuffer;

const makeCsoundCallback =
  (csoundInstance, apiCallback) =>
  async (...arguments_) => {
    return await apiCallback.apply({}, [csoundInstance, ...arguments_]);
  };

export const csoundWasiJsMessageCallback = ({ memory, messagePort, streamBuffer, wasi }) => {
  return function (csound_, attribute, length_, offset) {
    if (!memory) {
      return;
    }
    const buf = new Uint8Array(memory.buffer, offset, length_);
    const string = uint2String(buf);
    const endsWithNewline = /\n$/g.test(string);
    const startsWithNewline = /^\n/g.test(string);
    const chunks = string.split("\n").filter((item) => item.length > 0);
    const printableChunks = [];
    if ((chunks.length === 0 && endsWithNewline) || startsWithNewline) {
      printableChunks.push(streamBuffer.join(""));
      clearArray(streamBuffer);
    }
    chunks.forEach((chunk, index) => {
      // if it's last chunk
      if (index + 1 === chunks.length) {
        if (endsWithNewline) {
          if (index === 0) {
            printableChunks.push(streamBuffer.join("") + chunk);
            clearArray(streamBuffer);
          } else {
            printableChunks.push(chunk);
          }
        } else {
          streamBuffer.push(chunk);
        }
      } else if (index === 0) {
        printableChunks.push(streamBuffer.join("") + chunk);
        clearArray(streamBuffer);
      } else {
        printableChunks.push(chunk);
      }
    });
    printableChunks.forEach((chunk) => {
      const maybePrintable = chunk.replace(/(\r\n|\n|\r)/gm, "");
      if (maybePrintable) {
        messagePort.post({ log: chunk });
      }
    });
  };
};

const Csound = async ({ logCallback }) => {
  streamBuffer = [];
  if (!arrayBuffer) {
    arrayBuffer = wasmDataURI();
  }
  const wasi = new WASI({ preopens: { "/": "/" } });
  const wasmCompressed = new Uint8Array(arrayBuffer);
  const wasmBytes = pako.inflate(wasmCompressed);
  const module = await WebAssembly.compile(wasmBytes);
  const memory = new WebAssembly.Memory({ initial: 65536 / 4 });
  const options = wasi.getImports(module);
  options.env = options.env || {};
  options.env.csoundLoadModules = () => 0;
  options.env.memory = memory;

  const messagePort = { post: ({ log }) => typeof logCallback === "function" && logCallback(log) };

  options.env.csoundWasiJsMessageCallback = csoundWasiJsMessageCallback({
    memory: options.env.memory,
    streamBuffer,
    messagePort,
  });

  options.env.printDebugCallback = (offset, length) => {
    if (typeof logCallback === "function") {
      const buf = new Uint8Array(memory.buffer, offset, length);
      const string = uint2String(buf);
      logCallback(string);
    }
  };

  const instance = await WebAssembly.instantiate(module, options);

  instance.memory = memory;
  instance.wasi = wasi;

  wasi.setMemory(memory);
  wasi.start(instance);
  instance.exports.__wasi_js_csoundSetMessageStringCallback();

  const libcsound = libcsoundFactory(instance);

  const csoundInstance = libcsound.csoundCreate(0);

  const csoundApi = Object.keys(libcsound).reduce((accumulator, apiName) => {
    if (!libcsound[apiName]) {
      return accumulator;
    }

    if (fsMethods.includes(apiName)) {
      accumulator.fs = accumulator.fs || {};
      const reference = libcsound[apiName];
      const callback = async (...arguments_) =>
        makeCsoundCallback(csoundInstance, libcsound[apiName]).apply({}, arguments_);
      callback.toString = reference.toString;
      accumulator.fs[apiName] = callback;
    } else {
      const renamedApiName = csoundApiRename(apiName);
      accumulator[renamedApiName] = (...arguments_) => {
        return makeCsoundCallback(csoundInstance, libcsound[apiName]).apply({}, arguments_);
      };
      accumulator[renamedApiName].toString = libcsound[apiName].toString;
    }

    return accumulator;
  }, {});

  return csoundApi;
};

goog.exportSymbol("__Csound__", Csound);
