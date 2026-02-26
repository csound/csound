/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { dlinit } from "./dlinit";
import { WASI } from "./filesystem/wasi";
import { clearArray } from "./utils/clear-array";
import { uint2String } from "./utils/text-encoders.js";
import { Inflate } from "./zlib/inflate.js";

const PAGE_SIZE = 65536;
const PAGES_PER_MB = 16; // 1048576 bytes per MB / PAGE_SIZE
const WASI_LONGJMP_PREFIX = "CSOUND_WASI_LONGJMP:";

export const csoundWasiJsMessageCallback = ({ memory, messagePort, streamBuffer }) => {
  return function (_, __, length_, offset) {
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

function isWasmBinary(wasmBytes) {
  return (
    wasmBytes &&
    wasmBytes.length >= 8 &&
    wasmBytes[0] === 0x00 &&
    wasmBytes[1] === 0x61 &&
    wasmBytes[2] === 0x73 &&
    wasmBytes[3] === 0x6d
  );
}

const assertPluginExports = (pluginInstance) => {
  if (
    !pluginInstance ||
    typeof pluginInstance !== "object" ||
    typeof pluginInstance.exports !== "object"
  ) {
    console.error("Error instantiating a csound plugin, instance and/or export is missing!");
    return false;
  } else if (!pluginInstance.exports["__wasm_call_ctors"]) {
    console.error(
      "A csound plugin didn't export __wasm_call_ctors.\n" +
        "Please re-run wasm-ld with either --export-all or include --export=__wasm_call_ctors",
    );
    return false;
  } else if (
    !pluginInstance.exports["csoundModuleCreate"] &&
    !pluginInstance.exports["csound_opcode_init"] &&
    !pluginInstance.exports["csound_fgen_init"]
  ) {
    console.error(
      pluginInstance.exports,
      "A csound plugin turns out to be neither a plugin, opcode or module.\n" +
        "Perhaps csdl.h or module.h wasn't imported correctly?",
    );
    return false;
  } else {
    return true;
  }
};

// Accepts ArrayBuffer or Uint8Array
export function getBinaryHeaderData(input) {
  const wasmBytes = input instanceof Uint8Array ? input : new Uint8Array(input);

  // Check magic \0asm (little-endian 0x6d736100)
  if (wasmBytes.length < 8) {
    return { hasDylink: false, sectionSize: 0, memorySize: 0, memoryAlign: 0, tableSize: 0, tableAlign: 0, neededDynlibsCount: 0, neededDynlibs: [] };
  }
  if (!isWasmBinary(wasmBytes)) {
    return { hasDylink: false, sectionSize: 0, memorySize: 0, memoryAlign: 0, tableSize: 0, tableAlign: 0, neededDynlibsCount: 0, neededDynlibs: [] };
  }

  // Cursor after magic(4) + version(4)
  let pos = 8;

  // helpers
  function readULEB() {
    let result = 0;
    let shift = 0;
    for (;;) {
      if (pos >= wasmBytes.length) throw new Error("Unexpected EOF reading ULEB");
      const byte = wasmBytes[pos++];
      result |= (byte & 0x7f) << shift;
      if ((byte & 0x80) === 0) break;
      shift += 7;
    }
    return result;
  }

  function readBytes(n) {
    if (pos + n > wasmBytes.length) throw new Error("Unexpected EOF reading bytes");
    const sub = wasmBytes.subarray(pos, pos + n);
    pos += n;
    return sub;
  }

  function readName() {
    const len = readULEB();
    const bytes = readBytes(len);
    // ASCII for section names
    let s = "";
    for (let i = 0; i < bytes.length; i++) s += String.fromCharCode(bytes[i]);
    return s;
  }

  // Defaults for static WASM (no dylink)
  const defaults = { hasDylink: false, sectionSize: 0, memorySize: 0, memoryAlign: 0, tableSize: 0, tableAlign: 0, neededDynlibsCount: 0, neededDynlibs: [] };

  // Iterate sections
  while (pos < wasmBytes.length) {
    const id = wasmBytes[pos++];                 // section id (0 = custom)
    const size = readULEB();                      // section size (bytes)
    const sectionStart = pos;

    if (id === 0 /* custom */) {
      const namePos = pos;                        // remember to compute name+payload size
      const name = readName();                    // custom section name
      if (name === "dylink" || name === "dylink.0") {
        // Parse dylink(.0) payload
        // Spec (Emscripten): ULEB memorySize, ULEB memoryAlign, ULEB tableSize, ULEB tableAlign, ULEB neededDynlibsCount, then that many length-prefixed names
        const memorySize = readULEB();
        const memoryAlign = readULEB();
        const tableSize = readULEB();
        const tableAlign = readULEB();
        const neededDynlibsCount = readULEB();

        const neededDynlibs = [];
        for (let i = 0; i < neededDynlibsCount; i++) {
          neededDynlibs.push(readName());
        }

        const sectionSize = size; // raw size field for the custom section

        // Done — return immediately
        return {
          hasDylink: true,
          sectionSize,
          memorySize,
          memoryAlign,
          tableSize,
          tableAlign,
          neededDynlibsCount,
          neededDynlibs,
        };
      } else {
        // Not the dylink section; skip remainder of this custom section payload
        const consumed = pos - namePos; // bytes after we started inside payload
        const remaining = size - consumed;
        pos += Math.max(0, remaining);
      }
    } else {
      // Non-custom: just skip the payload
      pos += size;
    }

    // Safety: ensure we don't desync
    if (pos !== sectionStart + size) {
      pos = sectionStart + size;
    }
  }

  // No dylink section found => static binary
  return defaults;
}


export default async function ({ wasmDataURI, withPlugins = [], messagePort }) {
  const wasi = new WASI({ preopens: { "/": "/" } });
  const jumpTable = new Map(); // maps jmpbuf pointers to JS frames
  let tempRet0 = 0; // for 64-bit return value handling

  const saveSetjmp = (jmpbuf, label) => {
    jumpTable.set(jmpbuf, label);
    return 0;
  };

  const testSetjmp = (jmpbuf) => (jumpTable.has(jmpbuf) ? 1 : 0);

  const longjmp = (jmpbuf, value) => {
    const label = jumpTable.get(jmpbuf);
    if (!label) {
      throw new Error(`Invalid longjmp target ${jmpbuf}`);
    }
    throw `csound exit with code: ${value}`;
  };

  const __wasm_longjmp = (_, value) => {
    throw `csound exit with code: ${value}`;
  };

  const getTempRet0 = () => tempRet0;

  const setTempRet0 = (value) => {
    tempRet0 = value;
  };

  const wasmCompressed = new Uint8Array(wasmDataURI);
  const wasmZlib = new Inflate(wasmCompressed);

  const wasmBytes = wasmZlib.decompress();

  const magicData = getBinaryHeaderData(wasmBytes);
  // if (magicData === "static") {
  //   return await loadStaticWasm({ messagePort, wasmBytes, wasi });
  // }
  const { memorySize, memoryAlign } = magicData;

  // get the header data from plugins which we need before
  // initializing the main module
  withPlugins = await withPlugins.reduce(async (accumulator, wasmPlugin) => {
    const accumulator_ = await accumulator;

    let wasmPluginBytes;
    let pluginHeaderData;
    try {
      wasmPluginBytes = new Uint8Array(wasmPlugin);
      if (!isWasmBinary(wasmPluginBytes)) {
        console.warn(
          "Skipping plugin payload because it is not a wasm binary. " +
            "Check plugin URL/path and server mapping.",
        );
        return accumulator_;
      }
      pluginHeaderData = getBinaryHeaderData(wasmPluginBytes);
    } catch (error) {
      console.error("Error in plugin", error);
    }
    if (pluginHeaderData) {
      accumulator_.push({ headerData: pluginHeaderData, wasmPluginBytes });
    }
    return accumulator_;
  }, []);

  // The `fixedMemoryBase` is equivalent to the stack size. Note that the stack size grows down towards the code
  // section. This means that if the stack overflows then it will write over the Csound and plugin code which will
  // cause all kinds of strange behavior including errors that make no sense, no output of sound, or sound output will
  // be horrendously loud static and garbage sounds.
  //
  // TODO: Investigate using the --stack-first linker flag to move the stack to the beginning of memory so it doesn't
  // write over anything if it overflows.
  //
  const fixedMemoryBase = 128 * PAGES_PER_MB;
  const initialMemory = Math.ceil((memorySize + memoryAlign) / PAGE_SIZE);
  const pluginsMemory = Math.ceil(
    withPlugins.reduce(
      (accumulator, { headerData }) =>
        accumulator + (headerData.memorySize + memoryAlign),
      0,
    ) / PAGE_SIZE,
  );

  const totalInitialMemory = initialMemory + pluginsMemory + fixedMemoryBase;

  // Request a max of 1gb of memory so devices use less CPU when growing memory. This has a noticeable effect on low-
  // powered devices like the Oculus Quest 2.
  const memory = new WebAssembly.Memory({
    initial: totalInitialMemory,
    maximum: 1024 * PAGES_PER_MB,
  });

  // const table = new WebAssembly.Table({ initial: tableSize + 1, element: "anyfunc" });

  wasi.setMemory(memory);

  const memoryBase = new WebAssembly.Global({ value: "i32", mutable: false }, fixedMemoryBase);
  const tableBase = new WebAssembly.Global({ value: "i32", mutable: false }, 1);

  /** @suppress {checkTypes} */
  const module = await WebAssembly.compile(wasmBytes);
  const options = wasi.getImports(module);
  let withPlugins_ = [];

  let currentMemorySegment = initialMemory;

  let hostRuntimeInstance;

  const csoundLoadModules = (csoundInstance) => {
    withPlugins_.forEach((pluginItem) => {
      const pluginInstance =
        pluginItem && pluginItem.instance ? pluginItem.instance : pluginItem;
      const pluginTable = pluginItem && pluginItem.table ? pluginItem.table : table;
      if (hostRuntimeInstance === undefined) {
        console.error("csound-wasm internal: timing problem detected!");
      } else {
        dlinit(hostRuntimeInstance, pluginInstance, table, csoundInstance, pluginTable, memory);
      }
    });
    return 0;
  };

  options["env"] = options["env"] || {};
  options["env"]["memory"] = memory;
  // options["env"]["table"] = table;
  // options["env"]["__indirect_function_table"] = table;
  options["env"]["__memory_base"] = memoryBase;
  options["env"]["__table_base"] = tableBase;
  options["env"]["csoundLoadModules"] = csoundLoadModules;
  options["env"]["csoundLoadExternals"] = () => {};

  // Add setjmp/longjmp functions to the environment
  options["env"]["saveSetjmp"] = saveSetjmp;
  options["env"]["testSetjmp"] = testSetjmp;
  options["env"]["longjmp"] = longjmp;
  options["env"]["__wasm_longjmp"] = __wasm_longjmp;
  options["env"]["getTempRet0"] = getTempRet0;
  options["env"]["setTempRet0"] = setTempRet0;

  const streamBuffer = [];
  options["env"]["csoundWasiJsMessageCallback"] = csoundWasiJsMessageCallback({
    memory,
    messagePort,
    streamBuffer,
  });

  options["env"]["printDebugCallback"] = (offset, length) => {
    const buf = new Uint8Array(memory.buffer, offset, length);
    const string = uint2String(buf);
    if (string.startsWith(WASI_LONGJMP_PREFIX)) {
      const code = Number.parseInt(string.slice(WASI_LONGJMP_PREFIX.length), 10);
      throw new Error(`csound longjmp with code: ${Number.isNaN(code) ? -1 : code}`);
    }
    console.log(string);
  };

  // options["GOT.mem"] = options["GOT.mem"] || {};
  // options["GOT.func"] = options["GOT.func"] || {};

  /**
   * @suppress {checkTypes}
   * @type {WasmInst} */
  const instance = await WebAssembly.instantiate(module, options);
  const moduleExports = Object.assign({}, instance["exports"]);
  /**
   * @suppress {checkTypes}
   * @type {WasmInst} */
  const instance_ = {};

  moduleExports["memory"] = memory;

  /** @suppress {checkTypes} */
  instance_["exports"] = moduleExports;
  hostRuntimeInstance = instance_;

  const table = instance_["exports"]["__indirect_function_table"];
  const hasLegacyWasiPluginLoader =
    typeof instance_["exports"]["csoundWasiLoadPlugin"] === "function" ||
    typeof instance_["exports"]["csoundWasiLoadOpcodeLibrary"] === "function";

  withPlugins_ = await withPlugins.reduce(async (accumulator, { headerData, wasmPluginBytes }) => {
    accumulator = await accumulator;
    try {
      const {
        memorySize: pluginMemorySize,
        memoryAlign: pluginMemoryAlign,
        tableSize: pluginTableSize,
      } = headerData;

      /** @suppress {checkTypes} */
      const plugin = await WebAssembly.compile(wasmPluginBytes);
      const pluginOptions = wasi.getImports(plugin);
      const pluginExports = WebAssembly.Module.exports(plugin);
      const hasOpcodeInit = pluginExports.some(
        ({ name }) => name === "csound_opcode_init" || name === "csound_fgen_init",
      );
      const hasModuleInit = pluginExports.some(({ name }) => name === "csoundModuleInit");
      const useIsolatedPluginTable = !hasLegacyWasiPluginLoader || (hasOpcodeInit && !hasModuleInit);
      let pluginTable = table;

      if (useIsolatedPluginTable) {
        const isolatedInitial = Math.max(table.length + Math.max(pluginTableSize, 0) + 16, 16);
        pluginTable = new WebAssembly.Table({ initial: isolatedInitial, element: "anyfunc" });
        for (let tableIndex = 0; tableIndex < table.length; tableIndex += 1) {
          const tableFn = table.get(tableIndex);
          if (tableFn) {
            pluginTable.set(tableIndex, tableFn);
          }
        }
      } else if (pluginTableSize > 0) {
        table.grow(pluginTableSize);
      }

      pluginOptions["env"] = Object.assign({}, pluginOptions["env"]);
      pluginOptions["env"]["memory"] = memory;
      pluginOptions["env"]["__indirect_function_table"] = pluginTable;
      // pluginOptions["env"]["__memory_base"] = currentMemorySegment * PAGE_SIZE;
      // pluginOptions["env"]["__table_base"] = tableBase;
      delete pluginOptions["env"]["csoundWasiJsMessageCallback"];

      currentMemorySegment += Math.ceil((pluginMemorySize + pluginMemoryAlign) / PAGE_SIZE);

      /**
       * @suppress {checkTypes}
       * @type {WasmInst} */
      const pluginInstance = await WebAssembly.instantiate(plugin, pluginOptions);

      if (assertPluginExports(pluginInstance)) {
        pluginInstance.exports.__wasm_call_ctors();
        accumulator.push({ instance: pluginInstance, table: pluginTable });
      }
    } catch (error) {
      console.error("Error while compiling csound-plugin", error);
    }
    return accumulator;
  }, []);

  wasi.start(instance_);
  instance_["exports"]["__wasi_js_csoundSetMessageStringCallback"]();
  return [instance_, wasi];
}
