import { dlinit } from "./dlinit";
import { WASI } from "./filesystem/wasi";
import { clearArray } from "./utils/clear-array";
import { uint2String } from "./utils/text-encoders.js";
import { logWasmModule as log } from "./logger";
import { Inflate } from "./zlib/inflate.js";

const { assert } = goog.require("goog.asserts");

const PAGE_SIZE = 65536;
const PAGES_PER_MB = 16; // 1048576 bytes per MB / PAGE_SIZE

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
      pluginInstance.exports,
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
  next++; // size of "dylink" string
  assert(wasmBytes[next] === "d".codePointAt(0));
  next++;
  assert(wasmBytes[next] === "y".codePointAt(0));
  next++;
  assert(wasmBytes[next] === "l".codePointAt(0));
  next++;
  assert(wasmBytes[next] === "i".codePointAt(0));
  next++;
  assert(wasmBytes[next] === "n".codePointAt(0));
  next++;
  assert(wasmBytes[next] === "k".codePointAt(0));
  next++;
  assert(wasmBytes[next] === ".".codePointAt(0));
  next++;
  assert(wasmBytes[next] === "0".codePointAt(0));
  next += 3;

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

  wasi.setMemory(memory);
  wasi.start(instance);
  instance.exports.__wasi_js_csoundSetMessageStringCallback();
  return [instance, wasi];
};

export default async function ({ wasmDataURI, withPlugins = [], messagePort }) {
  const wasmFs = {};

  const wasi = new WASI({ preopens: { "/": "/" } });

  const wasmCompressed = new Uint8Array(wasmDataURI);
  const wasmZlib = new Inflate(wasmCompressed);

  const wasmBytes = wasmZlib.decompress();

  const magicData = getBinaryHeaderData(wasmBytes);
  if (magicData === "static") {
    return await loadStaticWasm({ messagePort, wasmBytes, wasmFs, wasi });
  }
  const { memorySize, memoryAlign, tableSize } = magicData;

  // get the header data from plugins which we need before
  // initializing the main module
  withPlugins = await withPlugins.reduce(async (accumulator, wasmPlugin) => {
    const accumulator_ = await accumulator;

    let wasmPluginBytes;
    let pluginHeaderData;
    try {
      wasmPluginBytes = new Uint8Array(wasmPlugin);
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
        headerData === "static" ? 0 : accumulator + (headerData.memorySize + memoryAlign),
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

  let currentMemorySegment = initialMemory;

  const csoundLoadModules = (csoundInstance) => {
    withPlugins_.forEach((pluginInstance) => {
      if (instance === undefined) {
        console.error("csound-wasm internal: timing problem detected!");
      } else {
        dlinit(instance, pluginInstance, table, csoundInstance);
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

  // TODO find out what's leaking this thread-local errno (cpp?)
  options.env._ZTH5errno = function () {};

  const streamBuffer = [];
  options.env.csoundWasiJsMessageCallback = csoundWasiJsMessageCallback({
    memory,
    messagePort,
    streamBuffer,
  });

  options.env.printDebugCallback = (offset, length) => {
    const buf = new Uint8Array(memory.buffer, offset, length);
    const string = uint2String(buf);
    console.log(string);
  };

  options["GOT.mem"] = options["GOT.mem"] || {};
  options["GOT.mem"].__heap_base = heapBase;

  options["GOT.func"] = options["GOT.func"] || {};

  const instance = await WebAssembly.instantiate(module, options);
  const moduleExports = Object.assign({}, instance.exports);
  const instance_ = {};
  instance_.exports = Object.assign(moduleExports, {
    memory,
  });

  withPlugins_ = await withPlugins.reduce(async (accumulator, { headerData, wasmPluginBytes }) => {
    accumulator = await accumulator;
    try {
      const {
        memorySize: pluginMemorySize,
        memoryAlign: pluginMemoryAlign,
        tableSize: pluginTableSize,
      } = headerData;

      const plugin = await WebAssembly.compile(wasmPluginBytes);
      const pluginOptions = wasi.getImports(plugin);

      const pluginMemoryBase = new WebAssembly.Global(
        { value: "i32", mutable: false },
        currentMemorySegment * PAGE_SIZE,
      );

      table.grow(pluginTableSize);

      pluginOptions.env = Object.assign({}, pluginOptions.env);
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
