import { WASI } from "@wasmer/wasi";
// import browserBindings from '@wasmer/wasi/lib/bindings/browser';
import { lowerI64Imports } from "@wasmer/wasm-transformer";
import { inflate } from "pako";
import { initFS, preopens, wasmFs } from "@root/filesystem";
import * as path from "path";

let wasi;

export const bindings = {
  ...WASI.defaultBindings,
  fs: wasmFs.fs,
  path,
};

const align_memory = (addr) => (addr + 15) & -16;

const GOT = {};

const GOTHandler = {
  get: function (obj, symName) {
    if (!GOT[symName]) {
      GOT[symName] = new WebAssembly.Global({ value: "i32", mutable: true });
      // console.log("new GOT entry: " + symName);
    }
    return GOT[symName];
  },
};

export default async function (wasmDataURI, withPlugins = []) {
  console.log("WITHPLGZ", withPlugins);
  const TOTAL_STACK = 5 * 1024 * 1024;
  const GLOBAL_BASE = -1;
  const static_bump = 0;
  const stack_low = align_memory(GLOBAL_BASE + static_bump);

  wasi = new WASI({
    preopens,
    env: {},
    bindings,
  });
  await wasmFs.volume.mkdirpSync("/sandbox");
  const wasmZlib = new Uint8Array(wasmDataURI);
  const wasmBytes = inflate(wasmZlib);
  const memory = new WebAssembly.Memory({
    initial: 1024 * 4,
    maximum: 65535,
  });
  const table = new WebAssembly.Table({ initial: 16384, element: "anyfunc" });
  const stackPointer = new WebAssembly.Global(
    { value: "i32", mutable: true },
    align_memory(stack_low + TOTAL_STACK),
  );
  const memoryBase = new WebAssembly.Global({ value: "i32", mutable: false }, 0);
  // https://github.com/emscripten-core/emscripten/blob/d995dfaa109262c405fe4e1d5b39312dac675962/emscripten.py#L555
  const tableBase = new WebAssembly.Global({ value: "i32", mutable: false }, 0);
  // const pluginLoadPtr = new WebAssembly.Global({ value: "i32", mutable: false }, 0);

  const transformedBinary = wasmBytes; // await lowerI64Imports(wasmBytes);
  const module = await WebAssembly.compile(transformedBinary);
  const options = wasi.getImports(module);
  options["wasi_snapshot_preview1"]["args_sizes_get"] = (argc, argv) => 0;
  console.log("OPTS", options);
  options["env"] = options["env"] || {};
  options["env"]["memory"] = memory;
  options["env"]["__indirect_function_table"] = table;
  options["env"]["__stack_pointer"] = stackPointer;
  options["env"]["__memory_base"] = memoryBase;
  options["env"]["__table_base"] = tableBase;
  options["GOT.mem"] = new Proxy(options["env"], GOTHandler);
  options["GOT.func"] = new Proxy(options["env"], GOTHandler);
  // 'GOT.mem':
  // 'GOT.func': new Proxy(asmLibraryArg, GOTHandler),
  // options["env"]["__indirect_function_table"] = table;
  // options["env"]["loadWasmPluginFromOentries"] = pluginLoadPtr;
  // options["env"]["memory"] = memory;
  // options["env"]["__indirect_function_table"] = table;
  // console.log("optins 1", options);
  const instance = await WebAssembly.instantiate(module, options);
  // instance.exports.memory = memory;
  console.log("HOST INST", instance);

  const pluginOptions = Object.assign({}, options);
  const pluginMemoryBase = new WebAssembly.Global({ value: "i32", mutable: false }, 512);
  pluginOptions["env"] = Object.assign(pluginOptions["env"] || {}, GOT);
  pluginOptions["env"]["memory"] = memory; //
  // pluginOptions["env"]["memory"] = instance.exports.memory;
  pluginOptions["env"]["__indirect_function_table"] = table;
  pluginOptions["env"]["__memory_base"] = pluginMemoryBase;
  // pluginOptions["env"]["__indirect_function_table"] = instance.exports.__indirect_function_table;
  // pluginOptions["env"]["loadWasmPluginFromOentries"] = pluginLoadPtr;
  // when compiling plugins, we usue same options, but use the host's memory

  withPlugins = await withPlugins.reduce(async (acc, wasmPlugin) => {
    acc = await acc;
    try {
      const wasmPluginBytes = new Uint8Array(wasmPlugin);
      const transformedPluginBinary = await lowerI64Imports(wasmPluginBytes);
      const plugin = await WebAssembly.compile(transformedPluginBinary);
      // const pluginOptions = wasi.getImports(plugin);

      // console.log("MOD", module);
      // console.log("optins 2", pluginOptions);
      // options["env"] = {};
      // const pluginInstance = await WebAssembly.instantiate(plugin, pluginOptions);
      // wasi.start(pluginInstance);
      // await initFS(pluginInstance);
      // console.log("PLGI", pluginInstance);

      // acc.push(pluginInstance);
    } catch (error) {
      console.error("Error while compiling csound-plugin", error);
    }
    return acc;
  }, []);

  const hax = { exports: {} };
  hax["exports"] = Object.assign({ memory }, instance);
  wasi.start(hax);
  // wasi.setMemory(memory);
  // wasi.refreshMemory();
  // console.log(table.get(0), 0);
  // console.log(table.get(1), 1);
  // console.log(table.get(2), 2);
  // console.log(table.get(3), 3);
  // console.log(table.get(4), 4);
  // console.log(table.get(200), 200);
  // console.log(table.get(203), 203);
  // console.log(table.get(204)(), 204);
  // console.log(table.get(204)(), 204);
  // console.log(table.get(204)(), 204);
  // console.log(table.get(205), 205);
  // console.log(table.get(206)(), 206);
  // console.log("CR", table, GOT.csoundCreate, GOT, Object.keys(GOT).length);
  await initFS(instance);

  return [instance, withPlugins];
}
