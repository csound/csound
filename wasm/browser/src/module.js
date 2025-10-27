import { dlinit } from "./dlinit";
import { WASI } from "./filesystem/wasi";
import { clearArray } from "./utils/clear-array";
import { uint2String } from "./utils/text-encoders.js";
import { logWasmModule as log } from "./logger";
import { Inflate } from "./zlib/inflate.js";

const { assert } = goog.require("goog.asserts");

const PAGE_SIZE = 65536;
const PAGES_PER_MB = 16; // 1048576 bytes per MB / PAGE_SIZE

// shared state
const jumpTable = new Map(); // maps jmpbuf pointers to JS frames
let currentJmpBuf = null;
let tempRet0 = 0; // for 64-bit return value handling

function saveSetjmp(jmpbuf, label) {
  jumpTable.set(jmpbuf, label);
  currentJmpBuf = jmpbuf;
  return 0;
}

function testSetjmp(jmpbuf) {
  return jumpTable.has(jmpbuf) ? 1 : 0;
}

function longjmp(jmpbuf, value) {
  const label = jumpTable.get(jmpbuf);
  if (!label) {
    throw new Error(`Invalid longjmp target ${jmpbuf}`);
  }
  throw `csound exit with code: ${value}`;
}

function __wasm_longjmp(_, value) {
  throw `csound exit with code: ${value}`;
}

function getTempRet0() {
  return tempRet0;
}

function setTempRet0(value) {
  tempRet0 = value;
}

export const csoundWasiJsMessageCallback = ({ memory, messagePort, streamBuffer }) => {
  console.log(111, { memory, messagePort, streamBuffer })
  return function (csound_, attribute, length_, offset) {
    console.log(222, {csound_, attribute, length_, offset});
    if (!memory) {
      return;
    }
    const buf = new Uint8Array(memory.buffer, offset, length_);
    const string = uint2String(buf);
    console.log({ buf, string });
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
  const magic =
    wasmBytes[0] |
    (wasmBytes[1] << 8) |
    (wasmBytes[2] << 16) |
    (wasmBytes[3] << 24);

  if (magic !== 0x6d736100) {
    console.error("Wasm magic number is missing!");
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
  const wasmFs = {};

  const wasi = new WASI({ preopens: { "/": "/" } });

  const wasmCompressed = new Uint8Array(wasmDataURI);
  const wasmZlib = new Inflate(wasmCompressed);

  const wasmBytes = wasmZlib.decompress();

  const magicData = getBinaryHeaderData(wasmBytes);
  // if (magicData === "static") {
  //   return await loadStaticWasm({ messagePort, wasmBytes, wasmFs, wasi });
  // }
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

  /** @suppress {checkTypes} */
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

  options["env"] = options["env"] || {};
  options["env"]["memory"] = memory;
  // options["env"]["table"] = table;
  // options["env"]["__indirect_function_table"] = table;
  // options["env"]["__stack_pointer"] = stackPointer;
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
    console.log(string);
  };

  // options["GOT.mem"] = options["GOT.mem"] || {};
  // options["GOT.mem"]["__heap_base"] = heapBase;
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

  const table = instance_["__indirect_function_table"];

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

      const pluginMemoryBase = new WebAssembly.Global(
        { value: "i32", mutable: false },
        currentMemorySegment * PAGE_SIZE,
      );

      table.grow(pluginTableSize);

      pluginOptions["env"] = Object.assign({}, pluginOptions["env"]);
      pluginOptions["env"]["memory"] = memory;
      pluginOptions["env"]["__indirect_function_table"] = table;
      // pluginOptions["env"]["__memory_base"] = pluginMemoryBase;
      // pluginOptions["env"]["__stack_pointer"] = stackPointer;
      // pluginOptions["env"]["__table_base"] = tableBase;
      // pluginOptions["env"]["csoundLoadModules"] = __dummy;
      delete pluginOptions["env"]["csoundWasiJsMessageCallback"];

      currentMemorySegment += Math.ceil((pluginMemorySize + pluginMemoryAlign) / PAGE_SIZE);

      /**
       * @suppress {checkTypes}
       * @type {WasmInst} */
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
  console.log({instance: instance_});

  instance_["exports"]["__wasi_js_csoundSetMessageStringCallback"]();
  console.log("message callback done");
  return [instance_, wasi];
}
