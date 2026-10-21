/* Copyright (c) The Csound Developers. Licensed under Apache-2.0. */
/* eslint-disable object-shorthand -- Plugin plans cross separately compiled worker bundles. */

import { dlinit } from "../dlinit.js";
import { preparePluginImports } from "./plugin-imports.js";
import { getBinaryHeaderData } from "./wasm-dylink.js";
import { decoder, uint2String } from "./text-encoders.js";

const MAX_I32 = 0x7fffffff;
const MAX_TABLE_GROWTH = 1000000;
const alignUp = (value, alignment) => Math.ceil(value / alignment) * alignment;
const alignment = (value) => {
  if (!Number.isSafeInteger(value) || value < 1 || value > 2 ** 30 || value & (value - 1))
    throw new RangeError("Invalid threaded plugin alignment");
  return value;
};
const alignmentExponent = (value) => {
  if (!Number.isInteger(value) || value < 0 || value > 30)
    throw new RangeError("Invalid threaded plugin alignment exponent");
  return alignment(2 ** value);
};
const growTable = (table, end) => {
  const growth = end - table.length;
  if (!Number.isSafeInteger(end) || end > MAX_I32 || growth > MAX_TABLE_GROWTH)
    throw new RangeError("Threaded plugin function table is too large");
  if (growth > 0) table.grow(growth);
};
const sameBytes = (a, b) => a.length === b.length && a.every((byte, index) => byte === b[index]);

const instantiatePlugin = ({ plan, memory, hostExports, importsForModule }) => {
  const module = plan["module"];
  const table = hostExports["__indirect_function_table"];
  growTable(table, plan["tableEnd"]);
  const options = importsForModule(module);
  options["env"] = Object.assign({}, options["env"], {
    "memory": memory,
    "__indirect_function_table": table,
    "__stack_pointer": hostExports["__stack_pointer"],
    "__memory_base": new WebAssembly.Global({ value: "i32", mutable: false }, plan["memoryBase"]),
    "__table_base": new WebAssembly.Global({ value: "i32", mutable: false }, plan["tableBase"]),
  });
  const moduleImports = WebAssembly.Module.imports(module);
  for (const item of moduleImports) {
    if (item.module === "env" && item.kind === "tag") {
      // Each plugin links its own C++ runtime. Let preparePluginImports give
      // that instance a private tag, including when instantiated in a worker.
      // C longjmp still uses the reactor's runtime and must share its tag.
      if (item.name === "__cpp_exception") continue;
      if (!(hostExports[item.name] instanceof WebAssembly.Tag))
        throw new TypeError(`Missing threaded plugin exception tag: ${item.name}`);
      options["env"][item.name] = hostExports[item.name];
    }
  }
  const link = preparePluginImports({
    imports: moduleImports,
    exports: WebAssembly.Module.exports(module),
    hostExports,
    options,
    memoryBase: plan["memoryBase"],
    importedSymbols: plan["symbols"].map(([module, name, flags]) => ({ module, name, flags })),
  });
  let instance;
  try {
    instance = new WebAssembly.Instance(module, options);
  } catch (error) {
    if (error instanceof WebAssembly.LinkError)
      throw new TypeError(
        `Threaded plugins require the shared-memory plugin SDK: ${error.message}`,
        { cause: error },
      );
    throw error;
  }
  let nextFunction = plan["gotBase"];
  link(instance.exports, (func) => {
    if (nextFunction >= plan["gotEnd"])
      throw new RangeError("Threaded plugin function address exceeds its reserved range");
    const index = nextFunction++;
    table.set(index, func);
    return index;
  });
  for (const [index, name] of plan["extraFunctions"]) {
    const func = instance.exports[name];
    if (typeof func !== "function") throw new TypeError(`Missing plugin export: ${name}`);
    table.set(index, func);
  }
  return instance;
};

// Each worker owns its Wasm table and globals. Only numeric addresses and
// compiled modules cross workers; function objects cannot be shared.
export function instantiateThreadedPlugins({
  plugins,
  memory,
  hostExports,
  importsForModule,
  index,
}) {
  const tls = [];
  for (const plan of plugins) {
    const instance = instantiatePlugin({ plan, memory, hostExports, importsForModule });
    if (plan["tlsBlocks"].length > 0) {
      const pointer = plan["tlsBlocks"][index + 1];
      if (!pointer) throw new RangeError("Missing worker plugin TLS allocation");
      tls.push(() => instance.exports["__wasm_init_tls"](pointer));
    }
    // LLVM's shared-memory start function guards data initialization atomically.
    // Relocations and C constructors belong to the parent and must not run here.
  }
  return {
    initializeTLS: () => {
      for (const initialize of tls) initialize();
    },
  };
}

export function createThreadedPluginRegistry({
  memory,
  hostExports,
  importsForModule,
  wasi,
  poolSize,
  onError = () => {},
}) {
  const table = hostExports["__indirect_function_table"];
  if (!(table instanceof WebAssembly.Table))
    throw new TypeError("The threaded reactor must export its function table");
  const plugins = [];
  const initial = new Set();
  const registrations = new Map();
  let frozen = false;
  let tableEnd = table.length;

  const allocate = (size, align, allocations) => {
    alignment(align);
    if (!Number.isSafeInteger(size) || size < 0 || size + align - 1 > MAX_I32)
      throw new RangeError("Threaded plugin memory is too large");
    if (!size) return 0;
    const allocation = hostExports["allocByteMem"](size + align - 1);
    if (!allocation) throw new Error("Could not allocate threaded plugin memory");
    allocations.push(allocation);
    const pointer = alignUp(allocation, align);
    new Uint8Array(memory.buffer, pointer, size).fill(0);
    return pointer;
  };

  const load = (input) => {
    const bytes = new Uint8Array(input);
    const cached = plugins.find((plugin) => sameBytes(plugin.bytes, bytes));
    if (cached) return cached;
    if (frozen) throw new Error("New plugins must be loaded before the first threaded performance");
    const header = getBinaryHeaderData(bytes, decoder);
    if (!header.hasDylink || header.neededDynlibs.length > 0)
      throw new TypeError("Threaded plugins must be standalone PIC modules without dependencies");
    if (!Number.isSafeInteger(header.tableSize) || header.tableSize > MAX_TABLE_GROWTH)
      throw new RangeError("Threaded plugin function table is too large");
    const module = new WebAssembly.Module(bytes);
    const tlsSymbols = new Set(
      [...header.importedSymbols, ...header.exportedSymbols]
        .filter(({ flags }) => flags & 0x100)
        .map(({ name }) => name),
    );
    if (
      WebAssembly.Module.imports(module).some(
        ({ module, name }) =>
          module === "GOT.tls" ||
          (module === "GOT.mem" && tlsSymbols.has(name)) ||
          (module === "env" && name === "__tls_get_addr"),
      )
    )
      throw new TypeError(
        "Threaded plugins support module-local TLS, not cross-module TLS addresses",
      );
    const names = new Set(WebAssembly.Module.exports(module).map(({ name }) => name));
    const tlsFields = ["__tls_size", "__tls_align", "__wasm_init_tls"];
    // Even zero-sized TLS must advertise the SDK contract. Without metadata
    // the loader cannot distinguish no TLS from hidden, uninitialized TLS.
    if (!tlsFields.every((name) => names.has(name)))
      throw new TypeError(
        "Threaded plugin TLS requires size, alignment and initializer exports; rebuild with the shared-memory plugin SDK",
      );
    if (
      !names.has("__wasm_call_ctors") ||
      !["csoundModuleInit", "csound_opcode_init", "csound_fgen_init"].some((name) =>
        names.has(name),
      )
    )
      throw new TypeError("The threaded plugin has no Csound entry point or constructors");
    if (names.has("csoundModuleInit") && !names.has("csoundModuleCreate"))
      throw new TypeError("A Csound module must export csoundModuleCreate");
    const tableBase = alignUp(tableEnd, alignmentExponent(header.tableAlign));
    const gotBase = tableBase + header.tableSize;
    const gotEnd =
      gotBase +
      WebAssembly.Module.imports(module).filter(({ module }) => module === "GOT.func").length;
    const allocations = [];
    const oldTableEnd = tableEnd;
    try {
      const plan = {
        "module": module,
        "memoryBase": allocate(
          header.memorySize,
          alignmentExponent(header.memoryAlign),
          allocations,
        ),
        "tableBase": tableBase,
        "gotBase": gotBase,
        "gotEnd": gotEnd,
        "tableEnd": gotEnd,
        "symbols": header.importedSymbols.map(({ module, name, flags }) => [module, name, flags]),
        "extraFunctions": [],
        "tlsBlocks": [],
      };
      const instance = instantiatePlugin({ plan, memory, hostExports, importsForModule });
      const exports = instance.exports;
      const tlsSize = exports["__tls_size"];
      if (
        !(tlsSize instanceof WebAssembly.Global) ||
        !(exports["__tls_align"] instanceof WebAssembly.Global) ||
        typeof exports["__wasm_init_tls"] !== "function" ||
        typeof tlsSize.value !== "number" ||
        !Number.isSafeInteger(tlsSize.value) ||
        tlsSize.value < 0
      )
        throw new TypeError("Invalid threaded plugin TLS metadata");
      const tlsAlign = alignment(Number(exports["__tls_align"].value));
      if (tlsSize.value !== 0) {
        for (let index = 0; index <= poolSize; index++) {
          plan["tlsBlocks"].push(allocate(tlsSize.value, tlsAlign, allocations));
        }
        exports["__wasm_init_tls"](plan["tlsBlocks"][0]);
      }
      if (typeof exports["__wasm_apply_data_relocs"] === "function")
        exports["__wasm_apply_data_relocs"]();
      exports["__wasm_call_ctors"]();

      const functions = new WeakMap();
      for (let index = 0; index < table.length; index++) {
        const func = table.get(index);
        if (func) functions.set(func, index);
      }
      const intern = (_, func) => {
        let index = functions.get(func);
        if (index !== undefined) return index;
        const name = Object.keys(exports).find((name) => exports[name] === func);
        if (!name) throw new TypeError("Plugin registration needs an exported function");
        index = plan["tableEnd"];
        growTable(table, index + 1);
        table.set(index, func);
        functions.set(func, index);
        plan["extraFunctions"].push([index, name]);
        plan["tableEnd"] = index + 1;
        return index;
      };
      // Csound may first register its default modules inside start(), after
      // workers have received this layout. Reserve those exported slots now.
      for (const name of [
        "csoundModuleCreate",
        "csoundModuleInit",
        "csoundModuleDestroy",
        "csoundModuleErrorCodeToString",
        "csound_opcode_init",
        "csound_fgen_init",
      ]) {
        if (typeof exports[name] === "function") intern(table, exports[name]);
      }
      const plugin = { bytes, plan, instance, intern };
      tableEnd = plan["tableEnd"];
      plugins.push(plugin);
      return plugin;
    } catch (error) {
      for (let index = oldTableEnd; index < table.length; index++) table.set(index, null);
      for (const allocation of allocations) hostExports["freeByteMem"](allocation);
      throw error;
    }
  };

  const register = (plugin, csound, registered) => {
    if (registered.has(plugin)) return;
    dlinit({ exports: hostExports }, plugin.instance, table, csound, table, memory, plugin.intern);
    registered.add(plugin);
  };
  const loadModules = (csound) => {
    const registered = new Set();
    registrations.set(csound, registered);
    try {
      for (const plugin of initial) register(plugin, csound, registered);
      return 0;
    } catch (error) {
      onError(error);
      return -1;
    }
  };
  const loadExternals = (csound, pointer) => {
    const bytes = new Uint8Array(memory.buffer, pointer);
    const end = bytes.indexOf(0);
    if (end === -1) return -1;
    const requested = uint2String(bytes.subarray(0, end));
    const registered = registrations.get(csound) || new Set();
    registrations.set(csound, registered);
    try {
      for (const path of new Set(requested.split(",").filter(Boolean)))
        register(load(wasi.readFile(path)), csound, registered);
      return 0;
    } catch (error) {
      onError(error);
      return -1;
    }
  };
  return {
    loadModules,
    loadExternals,
    addInitial: (bytes) => initial.add(load(bytes)),
    snapshot: () => plugins.map(({ plan }) => plan),
    freeze: () => {
      frozen = true;
    },
  };
}
