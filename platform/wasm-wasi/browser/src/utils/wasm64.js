import { WASI } from "../filesystem/preview2.js";
import { decoder } from "./utf8.js";

// JS typed-array offsets use exact Numbers. Keep the native i64 values at the
// Wasm boundary and reject addresses JS cannot represent instead of truncating.
export function addressNumber(value) {
  const number = Number(value);
  if (!Number.isSafeInteger(number))
    throw new RangeError("Wasm address exceeds JavaScript's exact integer range");
  return number;
}

export function moduleABI(module) {
  const sections = WebAssembly.Module.customSections(module, "csound.abi");
  return sections.length > 0 ? JSON.parse(decoder.decode(new Uint8Array(sections[0]))) : undefined;
}

export function tableNumbers(table, table64 = false) {
  if (!table64) return table;
  return {
    "raw": table,
    get length() {
      return addressNumber(table.length);
    },
    get: (index) => table.get(BigInt(index)),
    set: (index, value) => table.set(BigInt(index), value),
    grow: (count) => addressNumber(table.grow(BigInt(count))),
  };
}

export function nativeExports(exports, abi) {
  const result = Object.assign({}, exports);
  if (!abi) return result;
  for (const [name, [parameters, returns]] of Object.entries(abi["exports"])) {
    const fn = exports[name];
    if (typeof fn !== "function" || (!parameters.includes("i64") && !returns.includes("i64")))
      continue;
    result[name] = (...args) => {
      const value = fn(
        ...args.map((arg, index) => (parameters[index] === "i64" ? BigInt(arg ?? 0) : arg)),
      );
      return returns[0] === "i64" ? addressNumber(value) : value;
    };
  }
  for (const [name, value] of Object.entries(exports))
    if (value instanceof WebAssembly.Table) result[name] = tableNumbers(value, abi["table64"]);
  result["__pointerSize"] = abi["pointerSize"];
  return result;
}

export function nativeCallbacks(imports, abi) {
  if (!abi) return;
  for (const [key, [parameters, returns]] of Object.entries(abi["imports"])) {
    const separator = key.indexOf("#");
    const module = key.slice(0, separator);
    const name = key.slice(separator + 1);
    // The WASI adapter already has native signatures. Only adapt JS callbacks.
    if (module !== "env" || typeof imports[module]?.[name] !== "function") continue;
    const fn = imports[module][name];
    imports[module][name] = (...args) => {
      const value = fn(
        ...args.map((arg, index) => (parameters[index] === "i64" ? addressNumber(arg) : arg)),
      );
      return returns[0] === "i64" ? BigInt(value) : value;
    };
  }
}

export function memory64Imports(module, memory, wasi) {
  const sections = WebAssembly.Module.customSections(module, "csound.wasi-bridge");
  if (sections.length !== 1)
    throw new TypeError("The wasm64 engine requires its Preview 2 adapter");
  const bridgeModule = new WebAssembly.Module(sections[0]);
  const host = new WASI({
    filesystem: wasi.filesystem,
    root: wasi.root,
    getRandomValues: wasi.getRandomValues,
  });
  host.args = wasi.args;
  host.command = wasi.command;
  host.cwd = wasi.cwd;
  host.fd = wasi.fd;
  host.output = wasi.output;
  const imports = host.getImports(bridgeModule);
  imports["env"] = { memory };
  const bridge = new WebAssembly.Instance(bridgeModule, imports);
  host.setInstance(bridge);
  const engineImports = {};
  for (const item of WebAssembly.Module.imports(module)) {
    if (!item.module.startsWith("wasi:")) continue;
    const fn = bridge.exports[`${item.module}#${item.name}`];
    if (typeof fn !== "function") throw new TypeError(`Missing wasm64 WASI adapter: ${item.name}`);
    engineImports[item.module] ||= {};
    engineImports[item.module][item.name] = fn;
  }
  wasi.pointerSize = 8;
  return {
    imports: engineImports,
    connect: (instance) =>
      bridge.exports["engine-functions"].set(0, instance.exports["cabi_realloc"]),
  };
}

export function instantiateCommand(module, wasi) {
  const abi = moduleABI(module);
  if (!abi) return new WebAssembly.Instance(module, wasi.getImports(module));
  // Closure has not added the memory64 descriptor to its built-in externs.
  /** @suppress {checkTypes} */
  const memory = new WebAssembly.Memory({ "address": "i64", initial: BigInt(abi["memoryMinimum"]) });
  wasi.setMemory(memory);
  const bridge = memory64Imports(module, memory, wasi);
  bridge.imports["env"] = { memory };
  const instance = new WebAssembly.Instance(module, bridge.imports);
  bridge.connect(instance);
  return { exports: nativeExports(instance.exports, abi) };
}

export const pointerSize = (wasm) => wasm.exports["__pointerSize"] || 4;
export const readPointer = (wasm, view, offset) =>
  pointerSize(wasm) === 8
    ? addressNumber(view.getBigUint64(offset, true))
    : view.getUint32(offset, true);
export const sampleArrayType = (wasm) =>
  wasm.exports["csoundGetSizeOfMYFLT"]() === 8 ? Float64Array : Float32Array;
