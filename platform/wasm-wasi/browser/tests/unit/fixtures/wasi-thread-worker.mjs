import { parentPort } from "node:worker_threads";

globalThis.goog = { define: (_name, value) => value };
globalThis.addEventListener = (type, listener) => {
  if (type === "message") parentPort.on("message", (data) => listener({ data }));
};
globalThis.postMessage = (data) => parentPort.postMessage(data);
await import("../../../src/workers/wasi-thread.worker.js");
