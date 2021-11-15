// import path from "path-browserify";
// import { lowerI64Imports } from "@wasmer/wasm-transformer/lib/unoptimized/wasm-transformer.esm.js";
// import { WasmFs } from "./filesystem/memfs/index";
import { dlinit } from "./dlinit";
import { csoundWasiJsMessageCallback } from "./filesystem/worker-fs";
import { logWasmModule as log } from "./logger";
goog.require("Zlib");
goog.require("Zlib.Inflate");
goog.require("csound.filesystem.WASI");
goog.require("csound.utils.transform_imports");

const PAGE_SIZE = 65536;
const PAGES_PER_MB = 16; // 1048576 bytes per MB / PAGE_SIZE

const assertPluginExports = (pluginInstance) => {
  if (
    !pluginInstance ||
    typeof pluginInstance !== "object" ||
    typeof pluginInstance.exports !== "object"
  ) {
    console.error("Error instantiating a csound plugin, instance and/or export is missing!");
    return false;
  } else if (!pluginInstance.exports.__wasm_call_ctors) {
    console.error(
      "A csound plugin didn't export __wasm_call_ctors.\n" +
        "Please re-run wasm-ld with either --export-all or include --export=__wasm_call_ctors",
    );
    return false;
  } else if (
    !pluginInstance.exports.csoundModuleCreate &&
    !pluginInstance.exports.csound_opcode_init &&
    !pluginInstance.exports.csound_fgen_init
  ) {
    console.error(
      "A csound plugin turns out to be neither a plugin, opcode or module.\n" +
        "Perhaps csdl.h or module.h wasn't imported correctly?",
    );
    return false;
  } else {
    return true;
  }
};

const getBinaryHeaderData = (wasmBytes) => {
  const magicBytes = new Uint32Array(new Uint8Array(wasmBytes.subarray(0, 24)).buffer);
  // eslint-disable-next-line unicorn/number-literal-case
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
      // eslint-disable-next-line unicorn/number-literal-case
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

// currently dl is default, static is good for low level debugging
const loadStaticWasm = async ({ wasmBytes, wasmFs, wasi, messagePort }) => {
  const module = await WebAssembly.compile(wasmBytes);
  const memory = new WebAssembly.Memory({ initial: 65536 / 4 });
  const streamBuffer = [];
  const options = wasi.getImports(module);
  options.env = options.env || {};
  options.env.csoundLoadModules = () => 0;
  options.env.memory = memory;
  options.env.csoundWasiJsMessageCallback = csoundWasiJsMessageCallback({
    memory: options.env.memory,
    streamBuffer,
    messagePort,
  });

  const instance = await WebAssembly.instantiate(module, options);
  wasi.start(instance);
  // await initFS(wasmFs, messagePort);
  return instance;
};

export default async function ({
  wasmDataURI,
  wasmTransformerDataURI,
  withPlugins = [],
  messagePort,
}) {
  const wasmFs = {}; // new WasmFs();

  // const bindings = {
  //   // ...WASI.defaultBindings,
  //   fs: wasmFs.fs,
  //   path,
  // };

  // {
  //   // --dir <wasm path>:<host path>
  //   preopens: {
  //     "/": "/",
  //   },
  //   env: {},
  //   // bindings,
  // }

  const wasi = new csound.filesystem.WASI({ preopens: { "/": "/" } });
  await wasi.initDb();
  // await wasmFs.volume.mkdirpSync("/sandbox");

  const wasmCompressed = new Uint8Array(wasmDataURI);
  const wasmZlib = new Zlib.Inflate(wasmCompressed);

  const wasmInflated = wasmZlib.decompress();
  console.log({ wasmInflated });
  const wasmBytes = await csound.utils.transform_imports.lowerI64Imports(
    wasmInflated,
    wasmTransformerDataURI,
  );

  // const wasmBytes =
  //   typeof BigUint64Array === "undefined"
  //     ? await csound.utils.transform_imports.lowerI64Imports(wasmInflated, wasmTransformerDataURI)
  //       : wasmInflated;

  const magicData = getBinaryHeaderData(wasmInflated);
  if (magicData === "static") {
    return [await loadStaticWasm({ messagePort, wasmBytes, wasmFs, wasi }), wasmFs];
  }
  const { memorySize, memoryAlign, tableSize } = magicData;
  // get the header data from plugins which we need before
  // initializing the main module
  withPlugins = await withPlugins.reduce(async (accumulator, wasmPlugin) => {
    const accumulator_ = await accumulator;
    let loweredWasmPluginBytes;
    let wasmPluginBytes;
    let pluginHeaderData;
    try {
      wasmPluginBytes = new Uint8Array(wasmPlugin);
      loweredWasmPluginBytes =
        typeof BigUint64Array === "undefined"
          ? await lowerI64Imports(wasmPluginBytes)
          : wasmPluginBytes;
      pluginHeaderData = getBinaryHeaderData(wasmPluginBytes);
    } catch (error) {
      console.error("Error in plugin", error);
    }
    if (pluginHeaderData) {
      accumulator_.push({ headerData: pluginHeaderData, wasmPluginBytes: loweredWasmPluginBytes });
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
      (accumulator, { headerData: { memorySize } }) => accumulator + (memorySize + memoryAlign),
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

  const table = new WebAssembly.Table({ initial: tableSize + 1, element: "anyfunc" });

  wasi.setMemory(memory);

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

  const module = await WebAssembly.compile(wasmBytes);
  const options = wasi.getImports(module);
  let withPlugins_ = [];

  const csoundLoadModules = (csoundInstance) => {
    withPlugins_.forEach((pluginInstance) => {
      if (typeof instance !== "undefined") {
        dlinit(instance, pluginInstance, table, csoundInstance);
      } else {
        console.error("csound-wasm internal: timing problem detected!");
      }
    });
    return 0;
  };

  options.env = options.env || {};
  options.env.memory = memory;
  options.env.__indirect_function_table = table;
  options.env.__stack_pointer = stackPointer;
  options.env.__memory_base = memoryBase;
  options.env.__table_base = tableBase;
  options.env.csoundLoadModules = csoundLoadModules;

  const streamBuffer = [];
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

  let currentMemorySegment = initialMemory;

  withPlugins_ = await withPlugins.reduce(async (accumulator, { headerData, wasmPluginBytes }) => {
    accumulator = await accumulator;
    try {
      const {
        memorySize: pluginMemorySize,
        memoryAlign: pluginMemoryAlign,
        tableSize: pluginTableSize,
      } = headerData;

      const plugin = await WebAssembly.compile(wasmPluginBytes);
      const pluginOptions = {};
      const pluginMemoryBase = new WebAssembly.Global(
        { value: "i32", mutable: false },
        currentMemorySegment * PAGE_SIZE,
      );

      table.grow(pluginTableSize);
      pluginOptions.wasi_snapshot_preview1 = options.wasi_snapshot_preview1 || {};
      pluginOptions.env = Object.assign(pluginOptions.env || {}, {});
      pluginOptions.env.memory = memory;
      pluginOptions.env.__indirect_function_table = table;
      pluginOptions.env.__memory_base = pluginMemoryBase;
      pluginOptions.env.__stack_pointer = stackPointer;
      pluginOptions.env.__table_base = tableBase;
      pluginOptions.env.csoundLoadModules = __dummy;
      delete pluginOptions.env.csoundWasiJsMessageCallback;

      currentMemorySegment += Math.ceil((pluginMemorySize + pluginMemoryAlign) / PAGE_SIZE);
      const pluginInstance = await WebAssembly.instantiate(plugin, pluginOptions);
      if (assertPluginExports(pluginInstance)) {
        pluginInstance.exports.__wasm_call_ctors();
        accumulator.push(pluginInstance);
      }
    } catch (error) {
      console.error("Error while compiling csound-plugin", error);
    }
    return accumulator;
  }, []);

  wasi.start(instance_);
  instance_.exports.__wasi_js_csoundSetMessageStringCallback();
  return [instance_, wasi];
}
