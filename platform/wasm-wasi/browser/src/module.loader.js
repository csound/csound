import { sharedEngine } from "../dist/__csound_engine.inline.js";
import loadLegacyWasm from "./module.js";
import loadRealtimeWasm from "./module.realtime.js";

// Parent-only hosts use the same engine without a thread pool. The native host
// capability keeps --realtime off on UI, AudioWorklet, and MessagePort paths.
export default function loadWasm(options) {
  return sharedEngine
    ? loadRealtimeWasm({
        wasmBytes: options.wasmDataURI,
        withPlugins: options.withPlugins,
        messagePort: options.messagePort,
        ports: [],
        mayBlock: options.mayBlock === true,
      })
    : loadLegacyWasm(options);
}
