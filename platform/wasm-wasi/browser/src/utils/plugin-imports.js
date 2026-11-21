/* Copyright (c) The Csound Developers. Licensed under Apache-2.0. */

// These wasi-libc aliases have the same signatures as their public functions.
// Resolve them to the same Wasm functions without requiring private exports.
const hostFunctionAliases = new Map([
  ["__libc_malloc", "malloc"],
  ["__libc_calloc", "calloc"],
  ["__libc_free", "free"],
]);
const sharedFunctions = new Set([
  "malloc",
  "calloc",
  "realloc",
  "free",
  "aligned_alloc",
  "posix_memalign",
  "memcpy",
  "memmove",
  "memset",
]);
const ownValue = (object, name) =>
  Object.prototype.hasOwnProperty.call(object, name) ? object[name] : undefined;
const hostFunction = (exports, name) => ownValue(exports, hostFunctionAliases.get(name) || name);

// Use the engine for shared heap functions and weak definitions. Other strong
// definitions stay in the plugin, so its static runtime keeps its own state.
export const preparePluginImports = ({
  imports,
  exports,
  hostExports,
  hostTableExports = hostExports,
  options,
  importedSymbols = [],
  memoryBase,
  memory64 = false,
}) => {
  const exportKinds = new Map(exports.map(({ name, kind }) => [name, kind]));
  const weak = new Set(
    importedSymbols.filter(({ flags }) => flags & 1).map(({ module, name }) => `${module}:${name}`),
  );
  // LLVM records address-only weak references under their source namespace,
  // even when the binary imports only a GOT global for that symbol.
  const isWeak = (module, name) =>
    weak.has(`${module}:${name}`) ||
    ((module === "GOT.func" || module === "GOT.mem") && weak.has(`env:${name}`));
  let pluginExports;
  const addressImports = [];
  const preferHostFunction = (name, module) =>
    sharedFunctions.has(hostFunctionAliases.get(name) || name) || isWeak(module, name);

  const functionFor = (name, module) => {
    const host = hostFunction(hostExports, name);
    const hasLocal = exportKinds.get(name) === "function";
    if (typeof host === "function" && (!hasLocal || preferHostFunction(name, module))) {
      return host;
    }
    if (hasLocal) {
      return (...args) => pluginExports[name](...args);
    }
    if (isWeak(module, name)) {
      return () => {
        throw new WebAssembly.RuntimeError(`Unresolved weak function: ${name}`);
      };
    }
    throw new TypeError(`Missing WebAssembly host function: env.${name}`);
  };

  for (const { module, name, kind } of imports) {
    if (module === "GOT.func" || module === "GOT.mem") {
      if (kind !== "global")
        throw new TypeError(`Invalid WebAssembly address import: ${module}.${name}`);
      options[module] ||= Object.create(null);
      const global = new WebAssembly.Global(
        { value: memory64 ? "i64" : "i32", mutable: true },
        memory64 ? BigInt(0) : 0,
      );
      options[module][name] = global;
      addressImports.push({ module, name, global });
    } else if (module === "env" && kind === "function") {
      // Respect imports supplied by the caller before resolving engine symbols.
      if (ownValue(options["env"], name) === undefined) {
        options["env"][name] = functionFor(name, module);
      }
    } else if (
      module === "env" &&
      kind === "tag" &&
      ["__c_longjmp", "__cpp_exception"].includes(name) &&
      ownValue(options["env"], name) === undefined
    ) {
      // Statically linked runtimes own their exception state. Exceptions may
      // propagate through the engine, but cannot be caught by another plugin.
      options["env"][name] = new WebAssembly.Tag({ parameters: [memory64 ? "i64" : "i32"] });
    }
  }

  return (instanceExports, addFunction) => {
    pluginExports = instanceExports;
    for (const { module, name, global } of addressImports) {
      const local = pluginExports[name];
      if (module === "GOT.func") {
        const host = hostFunction(hostTableExports, name);
        const func =
          typeof host === "function" &&
          (typeof local !== "function" || preferHostFunction(name, module))
            ? host
            : local;
        if (typeof func === "function") {
          const address = addFunction(func);
          global.value = memory64 ? BigInt(address) : address;
        } else if (!isWeak(module, name)) {
          throw new TypeError(`Missing WebAssembly function address: ${name}`);
        }
      } else {
        const host = ownValue(hostExports, name);
        if (
          host instanceof WebAssembly.Global &&
          (!(local instanceof WebAssembly.Global) || isWeak(module, name))
        ) {
          global.value = host.value;
        } else if (local instanceof WebAssembly.Global) {
          const address = Number(local.value) + memoryBase;
          global.value = memory64 ? BigInt(address) : address;
        } else if (!isWeak(module, name)) {
          throw new TypeError(`Missing WebAssembly data address: ${name}`);
        }
      }
    }
  };
};
