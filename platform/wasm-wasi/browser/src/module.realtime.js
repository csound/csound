/* eslint-disable object-shorthand -- Host import and message keys must remain stable. */
import { WASI } from "./filesystem/wasi.js";
import { createSystem } from "./filesystem/system.js";
import { Inflate } from "./zlib/inflate.js";
import { uint2String } from "./utils/text-encoders.js";
import { csoundWasiJsMessageCallback } from "./module.js";
import { createWasiThreadHost } from "./utils/wasi-threads.js";
import { createThreadedPluginRegistry } from "./utils/threaded-plugins.js";
import { isWasmBinary } from "./utils/wasm-dylink.js";
import {
  createWasiClockImports,
  createWasiThreadEnvironment,
} from "./utils/wasi-threads-imports.js";

export default async function loadRealtimeWasm({
  wasmBytes,
  ports = [],
  withPlugins = [],
  messagePort,
}) {
  const input = new Uint8Array(wasmBytes);
  const binary =
    input[0] === 0 && input[1] === 97 && input[2] === 115 && input[3] === 109
      ? input
      : new Inflate(input).decompress();
  const module = await WebAssembly.compile(binary);
  const names = new Set(WebAssembly.Module.exports(module).map(({ name }) => name));
  if (!names.has("wasi_thread_start") || !names.has("_initialize"))
    throw new Error("Expected the threaded Csound browser reactor");
  const memory = new WebAssembly.Memory({ initial: 4096, maximum: 4096, shared: true });
  const wasi = new WASI({ preopens: { "/": "/" } });
  wasi.setMemory(memory);
  let host = null;
  let plugins = null;
  const imports = wasi.getImports(module);
  imports["env"] = createWasiThreadEnvironment({
    memory,
    postMessage: (data) => messagePort.post(data),
    decodeText: uint2String,
  });
  // The parent keeps the full host API when --realtime is off. Child imports
  // remain restricted because file handles cannot be copied across threads.
  imports["env"]["csoundWasiJsMessageCallback"] = csoundWasiJsMessageCallback({
    memory,
    messagePort,
    streamBuffer: [],
  });
  imports["env"]["csoundWasiJsSystem"] = createSystem(wasi, (log) =>
    messagePort.post({ "log": log }),
  );
  imports["env"]["csoundWasiJsReadlineCallback"] = (csound, requestId, offset, length) => {
    const prompt = offset === 0 ? null : uint2String(new Uint8Array(memory.buffer, offset, length));
    messagePort.post({
      "readline": { "csound": csound, "requestId": requestId >>> 0, "prompt": prompt },
    });
  };
  imports["env"]["csoundWasiJsSupportsThreads"] = () => (ports.length > 0 ? 1 : 0);
  imports["env"]["csoundLoadModules"] = (csound) => plugins.loadModules(csound);
  imports["env"]["csoundLoadExternals"] = (csound, pointer) =>
    plugins.loadExternals(csound, pointer);
  imports["wasi"] = {
    "thread-spawn": (argument, resultPointer) => {
      if (host) return host.threadSpawn(argument, resultPointer);
      // Plugin constructors and parent-only hosts may try pthread_create.
      // Report EAGAIN through either WASI ABI instead of trapping in JS.
      if (resultPointer === undefined) return -6;
      const view = new DataView(memory.buffer);
      view.setUint32(resultPointer, 1, true);
      view.setUint32(resultPointer + 4, 6, true);
      return undefined;
    },
  };
  // A local shared Wasm memory also works when the browser hides the global
  // SharedArrayBuffer constructor. UI/AudioWorklet hosts still cannot wait.
  const clockControl = new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true });
  const clocks = createWasiClockImports({ memory, control: new Int32Array(clockControl.buffer) });
  Object.assign(imports["wasi_snapshot_preview1"], clocks);
  const instance = await WebAssembly.instantiate(module, imports);
  // Constructors and the main thread's TLS run once, never on child instances.
  wasi.start(instance);
  const importsForModule = (pluginModule) => {
    const options = wasi.getImports(pluginModule);
    options["env"] = imports["env"];
    options["wasi"] = imports["wasi"];
    options["wasi_snapshot_preview1"] = Object.assign(
      {},
      options["wasi_snapshot_preview1"],
      clocks,
    );
    return options;
  };
  plugins = createThreadedPluginRegistry({
    memory,
    hostExports: instance.exports,
    importsForModule,
    wasi,
    poolSize: ports.length,
    onError: (error) => messagePort.post({ "log": error.message }),
  });
  for (const bytes of withPlugins) {
    const view = new Uint8Array(bytes);
    // A bad URL can return HTML. Skip it as the baseline loader does, while
    // leaving Wasm ABI and initialization failures to the plugin registry.
    if (!isWasmBinary(view)) {
      messagePort.post({
        "log":
          "Skipping plugin payload because it is not a wasm binary. " +
          "Check plugin URL/path and server mapping.",
      });
      continue;
    }
    plugins.addInitial(view);
  }
  instance.exports["__wasi_js_csoundSetMessageStringCallback"]();
  // Keep runtime fields stable across separately compiled worker bundles.
  const runtime = {
    "exports": instance.exports,
    "wasi": wasi,
    "threadHost": null,
    "prepareThreads": async (csound) => {
      if (ports.length === 0) return;
      if (!host) {
        // --opcode-lib may still be queued until native start. Resolve it before
        // the worker tables are copied, then keep this layout for every restart.
        if (instance.exports["csoundLoadExternals"](csound) !== 0)
          throw new Error("Could not load Csound plugins before preparing threads");
        plugins.freeze();
        host = createWasiThreadHost({
          module,
          memory,
          plugins: plugins.snapshot(),
          poolSize: ports.length,
          createWorker: (index) => {
            const port = ports[index];
            return {
              postMessage: (data) => port.postMessage(data),
              addEventListener: (...args) => port.addEventListener(...args),
              start: () => port.start(),
              terminate: () => port.close(),
            };
          },
          onMessage: (data) => messagePort.post(data),
          onError: (error) =>
            globalThis.postMessage({ "type": "wasi-thread-fatal", "message": error.message }),
        });
        runtime["threadHost"] = host;
      }
      await host.ready;
    },
  };
  return [runtime, wasi];
}
