/* Copyright (c) The Csound Developers. Licensed under Apache-2.0. */

// LLVM imports and exports a default-visible weak definition under the same
// name. Bind it to the host when present, or to its own export after creation.
export const preparePluginImports = ({
  imports,
  exports,
  hostExports,
  options,
  importedSymbols = [],
  memoryBase,
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
  const hostValue = (name) => (Object.hasOwn(hostExports, name) ? hostExports[name] : undefined);

  const functionFor = (name, module) => {
    const host = hostValue(name);
    if (typeof host === "function") return host;
    if (exportKinds.get(name) === "function") {
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
      const global = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
      options[module][name] = global;
      addressImports.push({ module, name, global });
    } else if (module === "env" && kind === "function") {
      options["env"][name] = functionFor(name, module);
    } else if (
      module === "env" &&
      kind === "tag" &&
      ["__c_longjmp", "__cpp_exception"].includes(name)
    ) {
      // Statically linked runtimes own their exception state. Exceptions may
      // propagate through the engine, but cannot be caught by another plugin.
      options["env"][name] = new WebAssembly.Tag({ parameters: ["i32"] });
    }
  }

  return (instanceExports, addFunction) => {
    pluginExports = instanceExports;
    for (const { module, name, global } of addressImports) {
      const host = hostValue(name);
      const local = pluginExports[name];
      if (module === "GOT.func") {
        const func = typeof host === "function" ? host : local;
        if (typeof func === "function") {
          global.value = addFunction(func);
        } else if (!isWeak(module, name)) {
          throw new TypeError(`Missing WebAssembly function address: ${name}`);
        }
      } else if (host instanceof WebAssembly.Global) {
        global.value = host.value;
      } else if (local instanceof WebAssembly.Global) {
        global.value = Number(local.value) + memoryBase;
      } else if (!isWeak(module, name)) {
        throw new TypeError(`Missing WebAssembly data address: ${name}`);
      }
    }
  };
};
