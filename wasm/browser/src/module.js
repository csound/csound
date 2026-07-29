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
import { decoder, uint2String } from "./utils/text-encoders.js";
import { getBinaryHeaderData as parseBinaryHeaderData } from "./utils/wasm-dylink.js";
import { Inflate } from "./zlib/inflate.js";

export const getBinaryHeaderData = (input) => parseBinaryHeaderData(input, decoder);

const PAGE_SIZE = 65536;
const PAGES_PER_MB = 16; // 1048576 bytes per MB / PAGE_SIZE
const WASI_LONGJMP_PREFIX = "CSOUND_WASI_LONGJMP:";
const MAX_ALIGNMENT_EXPONENT = 30;
const MAX_SIGNED_WASM_I32 = 0x7fffffff;

const alignmentFromExponent = (exponent) => {
  if (!Number.isInteger(exponent) || exponent < 0 || exponent > MAX_ALIGNMENT_EXPONENT) {
    throw new Error(`Invalid WebAssembly alignment exponent: ${exponent}`);
  }
  return 2 ** exponent;
};

const alignUp = (value, alignment) => Math.ceil(value / alignment) * alignment;

const wasmLongjmp = (_, value) => {
  throw new Error(`csound exit with code: ${value}`);
};

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
      const maybePrintable = chunk.replaceAll(/(\r\n|\n|\r)/gm, "");
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

export default async function loadWasm({ wasmDataURI, withPlugins = [], messagePort }) {
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
    throw new Error(`csound exit with code: ${value}`);
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
  const initialMemory = Math.ceil(
    (memorySize + alignmentFromExponent(memoryAlign)) / PAGE_SIZE,
  );
  const pluginsMemory = Math.ceil(
    withPlugins.reduce(
      (accumulator, { headerData }) =>
        accumulator +
        headerData.memorySize +
        alignmentFromExponent(headerData.memoryAlign),
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

  const hostRuntime = { instance: undefined };

  const csoundLoadModules = (csoundInstance) => {
    withPlugins_.forEach((pluginItem) => {
      const pluginInstance =
        pluginItem && pluginItem.instance ? pluginItem.instance : pluginItem;
      const pluginTable = pluginItem && pluginItem.table ? pluginItem.table : table;
      // dlinit falls back to direct plugin initialization until the host is ready.
      dlinit(hostRuntime.instance, pluginInstance, table, csoundInstance, pluginTable, memory);
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
  options["env"]["__wasm_longjmp"] = wasmLongjmp;
  options["env"]["getTempRet0"] = getTempRet0;
  options["env"]["setTempRet0"] = setTempRet0;

  const streamBuffer = [];
  options["env"]["csoundWasiJsMessageCallback"] = csoundWasiJsMessageCallback({
    memory,
    messagePort,
    streamBuffer,
  });

  options["env"]["csoundWasiJsDebugCallback"] = () => {
    messagePort.post({ debugCallback: true });
  };

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
  hostRuntime.instance = instance_;

  const table = instance_["exports"]["__indirect_function_table"];
  const hasLegacyWasiPluginLoader =
    typeof instance_["exports"]["csoundWasiLoadPlugin"] === "function" ||
    typeof instance_["exports"]["csoundWasiLoadOpcodeLibrary"] === "function";
  const pluginMemoryAllocations = [];

  wasi.start(instance_);

  const allocatePluginMemory = (size, alignmentExponent) => {
    if (size === 0) {
      return 0;
    }

    const allocate = instance_["exports"]["allocStringMem"];
    if (typeof allocate !== "function") {
      throw new TypeError("The WebAssembly host does not export allocStringMem");
    }

    const alignment = alignmentFromExponent(alignmentExponent);
    const allocationSize = size + alignment - 1;
    if (!Number.isSafeInteger(allocationSize) || allocationSize > MAX_SIGNED_WASM_I32) {
      throw new TypeError("The WebAssembly plugin memory request is too large");
    }

    const allocation = allocate(allocationSize);
    if (allocation === 0) {
      throw new Error(`Could not reserve ${allocationSize} bytes for a WebAssembly plugin`);
    }

    const memoryBaseValue = alignUp(allocation, alignment);
    new Uint8Array(memory.buffer, memoryBaseValue, size).fill(0);
    pluginMemoryAllocations.push(allocation);
    return memoryBaseValue;
  };

  withPlugins_ = await withPlugins.reduce(async (accumulator, { headerData, wasmPluginBytes }) => {
    accumulator = await accumulator;
    try {
      const {
        hasDylink,
        memorySize: pluginMemorySize,
        memoryAlign: pluginMemoryAlign,
        tableSize: pluginTableSize,
        tableAlign: pluginTableAlign,
        neededDynlibs,
      } = headerData;
      if (neededDynlibs.length > 0) {
        throw new Error(
          `WebAssembly plugin dependencies are not supported: ${neededDynlibs.join(", ")}`,
        );
      }

      /** @suppress {checkTypes} */
      const plugin = await WebAssembly.compile(wasmPluginBytes);
      const pluginOptions = wasi.getImports(plugin);
      const pluginImports = WebAssembly.Module.imports(plugin);
      const pluginExports = WebAssembly.Module.exports(plugin);
      const hasOpcodeInit = pluginExports.some(
        ({ name }) => name === "csound_opcode_init" || name === "csound_fgen_init",
      );
      const hasModuleInit = pluginExports.some(({ name }) => name === "csoundModuleInit");
      const useIsolatedPluginTable =
        !hasDylink &&
        (!hasLegacyWasiPluginLoader || (hasOpcodeInit && !hasModuleInit));
      let pluginTable = table;
      let pluginTableBaseValue = table.length;

      if (useIsolatedPluginTable) {
        const isolatedInitial = Math.max(table.length + Math.max(pluginTableSize, 0) + 16, 16);
        pluginTable = new WebAssembly.Table({ initial: isolatedInitial, element: "anyfunc" });
        for (let tableIndex = 0; tableIndex < table.length; tableIndex += 1) {
          const tableFn = table.get(tableIndex);
          if (tableFn) {
            pluginTable.set(tableIndex, tableFn);
          }
        }
      } else {
        const pluginTableAlignment = alignmentFromExponent(pluginTableAlign);
        pluginTableBaseValue = alignUp(table.length, pluginTableAlignment);
        const tableGrowth = pluginTableBaseValue + pluginTableSize - table.length;
        if (tableGrowth > 0) {
          table.grow(tableGrowth);
        }
      }

      pluginOptions["env"] = Object.assign({}, pluginOptions["env"]);
      pluginOptions["env"]["memory"] = memory;
      pluginOptions["env"]["__indirect_function_table"] = pluginTable;
      const pluginMemoryBaseValue = hasDylink
        ? allocatePluginMemory(pluginMemorySize, pluginMemoryAlign)
        : 0;
      pluginOptions["env"]["__memory_base"] = new WebAssembly.Global(
        { value: "i32", mutable: false },
        pluginMemoryBaseValue,
      );
      pluginOptions["env"]["__table_base"] = new WebAssembly.Global(
        { value: "i32", mutable: false },
        pluginTableBaseValue,
      );

      for (const pluginImport of pluginImports) {
        if (pluginImport.module !== "env") {
          continue;
        }
        if (pluginImport.kind === "function" && !pluginOptions["env"][pluginImport.name]) {
          const hostFunction = instance_["exports"][pluginImport.name];
          if (typeof hostFunction !== "function") {
            throw new TypeError(`Missing WebAssembly host function: env.${pluginImport.name}`);
          }
          pluginOptions["env"][pluginImport.name] = hostFunction;
        } else if (
          pluginImport.kind === "global" &&
          pluginImport.name === "__stack_pointer"
        ) {
          const hostStackPointer = instance_["exports"]["__stack_pointer"];
          if (!hostStackPointer) {
            throw new Error("The WebAssembly host does not export __stack_pointer");
          }
          pluginOptions["env"]["__stack_pointer"] = hostStackPointer;
        }
      }
      delete pluginOptions["env"]["csoundWasiJsMessageCallback"];

      /**
       * @suppress {checkTypes}
       * @type {WasmInst} */
      const pluginInstance = await WebAssembly.instantiate(plugin, pluginOptions);

      if (assertPluginExports(pluginInstance)) {
        if (typeof pluginInstance.exports.__wasm_apply_data_relocs === "function") {
          pluginInstance.exports.__wasm_apply_data_relocs();
        }
        pluginInstance.exports.__wasm_call_ctors();
        accumulator.push({ instance: pluginInstance, table: pluginTable });
      }
    } catch (error) {
      console.error("Error while compiling csound-plugin", error);
    }
    return accumulator;
  }, []);

  instance_["exports"]["__wasi_js_csoundSetMessageStringCallback"]();
  return [instance_, wasi];
}
