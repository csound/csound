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
// Bound table growth and failed-load cleanup for one plugin.
const MAX_PLUGIN_TABLE_ENTRIES = 1000000;
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

function equalWasmBytes(left, right) {
  if (left === right) {
    return true;
  }
  if (!left || !right || left.length !== right.length) {
    return false;
  }
  for (const [index, byte] of left.entries()) {
    if (byte !== right[index]) {
      return false;
    }
  }
  return true;
}

const assertPluginExports = (pluginExports) => {
  const exportNames = new Set(pluginExports.map(({ name }) => name));
  if (!exportNames.has("__wasm_call_ctors")) {
    console.error(
      "A csound plugin didn't export __wasm_call_ctors.\n" +
        "Please re-run wasm-ld with either --export-all or include --export=__wasm_call_ctors",
    );
    return false;
  } else if (
    !exportNames.has("csoundModuleCreate") &&
    !exportNames.has("csound_opcode_init") &&
    !exportNames.has("csound_fgen_init")
  ) {
    console.error(
      pluginExports,
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
  const withPlugins_ = [];
  const cachedPlugins = [];
  const pluginByRequestedPath = new Map();
  const registrationsByCsound = new Map();
  const pluginRuntime = { instantiatePlugin: undefined, table: undefined };
  let sharedTableEnd = 0;

  const hostRuntime = { instance: undefined };

  const registrationState = (csoundInstance) => {
    let state = registrationsByCsound.get(csoundInstance);
    if (!state) {
      state = { paths: new Set(), plugins: new Set() };
      registrationsByCsound.set(csoundInstance, state);
    }
    return state;
  };

  const registerPlugin = (pluginItem, csoundInstance, state) => {
    if (state.plugins.has(pluginItem)) {
      return;
    }
    const pluginInstance = pluginItem.instance || pluginItem;
    const pluginTable = pluginItem.table || pluginRuntime.table;
    // dlinit falls back to direct plugin initialization until the host is ready.
    try {
      dlinit(
        hostRuntime.instance,
        pluginInstance,
        pluginRuntime.table,
        csoundInstance,
        pluginTable,
        memory,
      );
    } finally {
      sharedTableEnd = Math.max(sharedTableEnd, pluginRuntime.table.length);
    }
    state.plugins.add(pluginItem);
  };

  const csoundLoadModules = (csoundInstance) => {
    // Csound calls this once for each new module lifecycle, including reset.
    const state = { paths: new Set(), plugins: new Set() };
    registrationsByCsound.set(csoundInstance, state);
    withPlugins_.forEach((pluginItem) => {
      registerPlugin(pluginItem, csoundInstance, state);
    });
    return 0;
  };

  const readCString = (pointer) => {
    if (!pointer) {
      return "";
    }
    const bytes = new Uint8Array(memory.buffer, pointer);
    let length = 0;
    while (length < bytes.length && bytes[length] !== 0) {
      length += 1;
    }
    return uint2String(bytes.subarray(0, length));
  };

  const csoundLoadExternals = (csoundInstance, requestedPluginsPointer) => {
    if (typeof pluginRuntime.instantiatePlugin !== "function") {
      return -1;
    }

    const requested = readCString(requestedPluginsPointer);
    const requestedPaths = [...new Set(requested.split(",").filter((path) => path.length > 0))];
    requestedPaths.sort();

    const state = registrationState(csoundInstance);
    let result = 0;

    requestedPaths.forEach((path) => {
      if (state.paths.has(path)) {
        return;
      }

      try {
        const wasmPluginBytes = wasi.readFile(path);
        if (!isWasmBinary(wasmPluginBytes)) {
          console.error(`Unable to load requested Csound plugin '${path}' from the WASI filesystem.`);
          result = -1;
          return;
        }

        let pluginItem = pluginByRequestedPath.get(path);
        if (!pluginItem || !equalWasmBytes(pluginItem.wasmPluginBytes, wasmPluginBytes)) {
          pluginItem = cachedPlugins.find((item) =>
            equalWasmBytes(item.wasmPluginBytes, wasmPluginBytes),
          );
          if (!pluginItem) {
            pluginItem = pluginRuntime.instantiatePlugin({
              headerData: getBinaryHeaderData(wasmPluginBytes),
              wasmPluginBytes,
            });
            if (pluginItem) {
              cachedPlugins.push(pluginItem);
            }
          }
          if (pluginItem) {
            pluginByRequestedPath.set(path, pluginItem);
          }
        }

        if (!pluginItem) {
          result = -1;
          return;
        }

        registerPlugin(pluginItem, csoundInstance, state);
        state.paths.add(path);
      } catch (error) {
        console.error(`Error while loading requested Csound plugin '${path}'`, error);
        result = -1;
      }
    });

    return result;
  };

  options["env"] = options["env"] || {};
  options["env"]["memory"] = memory;
  // options["env"]["table"] = table;
  // options["env"]["__indirect_function_table"] = table;
  options["env"]["__memory_base"] = memoryBase;
  options["env"]["__table_base"] = tableBase;
  options["env"]["csoundLoadModules"] = csoundLoadModules;
  options["env"]["csoundLoadExternals"] = csoundLoadExternals;

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
  const nativeCsoundDestroy = moduleExports["csoundDestroy"];
  if (typeof nativeCsoundDestroy === "function") {
    moduleExports["csoundDestroy"] = (csoundInstance) => {
      try {
        return nativeCsoundDestroy(csoundInstance);
      } finally {
        registrationsByCsound.delete(csoundInstance);
      }
    };
  }

  /** @suppress {checkTypes} */
  instance_["exports"] = moduleExports;
  hostRuntime.instance = instance_;

  pluginRuntime.table = instance_["exports"]["__indirect_function_table"];
  sharedTableEnd = pluginRuntime.table.length;
  wasi.start(instance_);
  const hasLegacyWasiPluginLoader =
    typeof instance_["exports"]["csoundWasiLoadPlugin"] === "function" ||
    typeof instance_["exports"]["csoundWasiLoadOpcodeLibrary"] === "function";

  const allocatePluginMemory = (size, alignmentExponent) => {
    if (size === 0) {
      return { allocation: 0, memoryBaseValue: 0 };
    }

    const allocate = instance_["exports"]["allocStringMem"];
    const free = instance_["exports"]["freeStringMem"];
    if (typeof allocate !== "function") {
      throw new TypeError("The WebAssembly host does not export allocStringMem");
    }
    if (typeof free !== "function") {
      throw new TypeError("The WebAssembly host does not export freeStringMem");
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
    try {
      new Uint8Array(memory.buffer, memoryBaseValue, size).fill(0);
    } catch (error) {
      free(allocation);
      throw error;
    }
    return { allocation, memoryBaseValue };
  };

  pluginRuntime.instantiatePlugin = ({
    headerData,
    plugin: precompiledPlugin,
    wasmPluginBytes,
  }) => {
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
    const plugin = precompiledPlugin || new WebAssembly.Module(wasmPluginBytes);
    const pluginOptions = wasi.getImports(plugin);
    const pluginImports = WebAssembly.Module.imports(plugin);
    const pluginExports = WebAssembly.Module.exports(plugin);
    if (!assertPluginExports(pluginExports)) {
      return undefined;
    }
    const hasOpcodeInit = pluginExports.some(
      ({ name }) => name === "csound_opcode_init" || name === "csound_fgen_init",
    );
    const hasModuleInit = pluginExports.some(({ name }) => name === "csoundModuleInit");
    const useIsolatedPluginTable =
      !hasDylink && (!hasLegacyWasiPluginLoader || (hasOpcodeInit && !hasModuleInit));
    let pluginMemoryAllocation = 0;
    let sharedTableBase = 0;
    let sharedTableLimit = 0;
    let sharedTableReserved = false;

    try {
      let pluginTable = pluginRuntime.table;
      let pluginTableBaseValue = pluginRuntime.table.length;

      if (useIsolatedPluginTable) {
        const isolatedInitial = Math.max(
          pluginRuntime.table.length + Math.max(pluginTableSize, 0) + 16,
          16,
        );
        pluginTable = new WebAssembly.Table({ initial: isolatedInitial, element: "anyfunc" });
        for (let tableIndex = 0; tableIndex < pluginRuntime.table.length; tableIndex += 1) {
          const tableFn = pluginRuntime.table.get(tableIndex);
          if (tableFn) {
            pluginTable.set(tableIndex, tableFn);
          }
        }
      } else {
        if (
          !Number.isSafeInteger(pluginTableSize) ||
          pluginTableSize < 0 ||
          pluginTableSize > MAX_PLUGIN_TABLE_ENTRIES
        ) {
          throw new TypeError("Invalid WebAssembly plugin table size");
        }
        const pluginTableAlignment = alignmentFromExponent(pluginTableAlign);
        pluginTableBaseValue = alignUp(sharedTableEnd, pluginTableAlignment);
        const tableLimit = pluginTableBaseValue + pluginTableSize;
        if (!Number.isSafeInteger(tableLimit) || tableLimit > MAX_SIGNED_WASM_I32) {
          throw new TypeError("The WebAssembly plugin table request is too large");
        }
        const tableGrowth = tableLimit - pluginRuntime.table.length;
        if (tableGrowth > MAX_PLUGIN_TABLE_ENTRIES) {
          throw new TypeError("The WebAssembly plugin table request is too large");
        }
        if (tableGrowth > 0) {
          pluginRuntime.table.grow(tableGrowth);
        }
        sharedTableBase = pluginTableBaseValue;
        sharedTableLimit = tableLimit;
        sharedTableReserved = true;
      }

      pluginOptions["env"] = Object.assign({}, pluginOptions["env"]);
      pluginOptions["env"]["memory"] = memory;
      pluginOptions["env"]["__indirect_function_table"] = pluginTable;
      const pluginMemory = hasDylink
        ? allocatePluginMemory(pluginMemorySize, pluginMemoryAlign)
        : { allocation: 0, memoryBaseValue: 0 };
      pluginMemoryAllocation = pluginMemory.allocation;
      pluginOptions["env"]["__memory_base"] = new WebAssembly.Global(
        { value: "i32", mutable: false },
        pluginMemory.memoryBaseValue,
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
      /**
       * @suppress {checkTypes}
       * @type {WasmInst} */
      const pluginInstance = new WebAssembly.Instance(plugin, pluginOptions);

      if (typeof pluginInstance.exports["__wasm_apply_data_relocs"] === "function") {
        pluginInstance.exports["__wasm_apply_data_relocs"]();
      }
      pluginInstance.exports.__wasm_call_ctors();
      pluginMemoryAllocation = 0;
      if (sharedTableReserved) {
        sharedTableEnd = sharedTableLimit;
        sharedTableReserved = false;
      }
      return { instance: pluginInstance, table: pluginTable, wasmPluginBytes };
    } finally {
      if (sharedTableReserved) {
        for (let tableIndex = sharedTableBase; tableIndex < sharedTableLimit; tableIndex += 1) {
          pluginRuntime.table.set(tableIndex, null);
        }
      }
      if (pluginMemoryAllocation !== 0) {
        instance_["exports"]["freeStringMem"](pluginMemoryAllocation);
      }
    }
  };

  const compiledWithPlugins = await Promise.all(
    withPlugins.map(async ({ headerData, wasmPluginBytes }) => {
      try {
        const plugin = await WebAssembly.compile(wasmPluginBytes);
        return { headerData, plugin, wasmPluginBytes };
      } catch (error) {
        console.error("Error while compiling csound-plugin", error);
        return undefined;
      }
    }),
  );

  compiledWithPlugins.forEach((pluginInput) => {
    if (!pluginInput) {
      return;
    }
    try {
      const pluginItem = pluginRuntime.instantiatePlugin(pluginInput);
      if (pluginItem) {
        cachedPlugins.push(pluginItem);
        withPlugins_.push(pluginItem);
      }
    } catch (error) {
      console.error("Error while instantiating csound-plugin", error);
    }
  });

  instance_["exports"]["__wasi_js_csoundSetMessageStringCallback"]();
  return [instance_, wasi];
}
