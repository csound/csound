import { WASI } from "@wasmer/wasi";
import { inflate } from "pako";
import { initFS, preopens, wasmFs } from "@root/filesystem";
import * as path from "path";

const PAGE_SIZE = 65536;

let wasi;

export const bindings = {
  ...WASI.defaultBindings,
  fs: wasmFs.fs,
  path,
};

const getBinaryHeaderData = (wasmBytes) => {
  const magicBytes = new Uint32Array(new Uint8Array(wasmBytes.subarray(0, 24)).buffer);
  if (magicBytes[0] !== 0x6d736100) {
    console.error("Wasm magic number is missing!");
  }
  if (wasmBytes[8] !== 0) {
    console.error("Dylink section wasn't found in wasm binary");
    return;
  }

  let next = 9;
  function getLEB() {
    var ret = 0;
    var mul = 1;
    while (1) {
      var byte = wasmBytes[next++];
      ret += (byte & 0x7f) * mul;
      mul *= 0x80;
      if (!(byte & 0x80)) break;
    }
    return ret;
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

export default async function (wasmDataURI, withPlugins = []) {
  wasi = new WASI({
    preopens,
    env: {},
    bindings,
  });
  await wasmFs.volume.mkdirpSync("/sandbox");
  const wasmZlib = new Uint8Array(wasmDataURI);
  const wasmBytes = inflate(wasmZlib);

  const { memorySize, memoryAlign, tableSize } = getBinaryHeaderData(wasmBytes);
  // get the header data from plugins which we need before
  // initializing the main module
  withPlugins = withPlugins.reduce((acc, wasmPlugin) => {
    let wasmPluginBytes;
    let pluginHeaderData;
    try {
      wasmPluginBytes = new Uint8Array(wasmPlugin);
      pluginHeaderData = getBinaryHeaderData(wasmPluginBytes);
    } catch (error) {
      console.error("Error in plugin", error);
    }
    if (pluginHeaderData) {
      acc.push({ headerData: pluginHeaderData, wasmPluginBytes });
    }
    return acc;
  }, []);

  const initialMemory = Math.ceil((memorySize + memoryAlign) / PAGE_SIZE);
  const pluginsMemory = Math.ceil(
    withPlugins.reduce(
      (acc, { headerData: { memorySize } }) => acc + (memorySize + memoryAlign),
      0,
    ) / PAGE_SIZE,
  );
  const totalInitialMemory = initialMemory + pluginsMemory;

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
  const memoryBase = new WebAssembly.Global({ value: "i32", mutable: false }, 0);
  const tableBase = new WebAssembly.Global({ value: "i32", mutable: false }, 1);
  const __dummy = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
  const module = await WebAssembly.compile(wasmBytes);
  const options = wasi.getImports(module);

  options["env"] = options["env"] || {};
  options["env"]["memory"] = memory;
  options["env"]["__indirect_function_table"] = table;
  options["env"]["__stack_pointer"] = stackPointer;
  options["env"]["__memory_base"] = memoryBase;
  options["env"]["__table_base"] = tableBase;

  options["GOT.mem"] = options["GOT.mem"] || {};
  options["GOT.mem"]["__heap_base"] = heapBase;

  options["GOT.func"] = options["GOT.func"] || {};
  options["GOT.func"]["__wasilibc_find_relpath_alloc"] = __dummy;

  const instance = await WebAssembly.instantiate(module, options);

  const moduleExports = Object.assign({}, instance.exports);
  const instance_ = {};
  instance_["exports"] = Object.assign(moduleExports, {
    memory,
  });

  let currentMemorySegment = initialMemory;

  withPlugins = await withPlugins.reduce(async (acc, { headerData, wasmPluginBytes }) => {
    acc = await acc;
    try {
      const {
        memorySize: pluginMemorySize,
        memoryAlign: pluginMemoryAlign,
        tableSize: pluginTableSize,
      } = headerData;

      const plugin = await WebAssembly.compile(wasmPluginBytes);
      const pluginOptions = Object.assign({}, options);
      const pluginMemoryBase = new WebAssembly.Global(
        { value: "i32", mutable: false },
        currentMemorySegment * PAGE_SIZE,
      );

      table.grow(pluginTableSize);
      pluginOptions["env"] = Object.assign(pluginOptions["env"] || {}, {});
      pluginOptions["env"]["memory"] = memory;
      pluginOptions["env"]["__indirect_function_table"] = table;
      pluginOptions["env"]["__memory_base"] = pluginMemoryBase;
      pluginOptions["env"]["__stack_pointer"] = stackPointer;
      pluginOptions["env"]["__table_base"] = tableBase;
      currentMemorySegment += Math.ceil((pluginMemorySize + pluginMemoryAlign) / PAGE_SIZE);
      const pluginInstance = await WebAssembly.instantiate(plugin, pluginOptions);
      pluginInstance.exports.__wasm_call_ctors();
      acc.push(pluginInstance);
    } catch (error) {
      console.error("Error while compiling csound-plugin", error);
    }
    return acc;
  }, []);

  wasi.start(instance_);

  await initFS(instance_);

  return instance_;
}
