/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

/* eslint-disable object-shorthand -- Import names and output keys are part of the host ABI. */

import { WASI } from "../filesystem/preview3.js";
import { bufferMessageText } from "./message-stream.js";

const MAX_OUTPUT = 16000;

export function createWasiThreadEnvironment({
  memory,
  postMessage,
  decodeText = (bytes) => new TextDecoder().decode(bytes),
}) {
  const streamBuffer = [];
  const messagePort = { post: postMessage };
  const text = (pointer, length) =>
    decodeText(new Uint8Array(memory.buffer, pointer, Math.min(length, MAX_OUTPUT)).slice());
  return {
    "memory": memory,
    "csoundWasiJsSupportsThreads": () => 1,
    "csoundWasiApiComplete": (id, generation, status, number, pointer) => {
      /** @type {number|string} */
      let answer = number;
      if (pointer) {
        const bytes = new Uint8Array(memory.buffer, pointer);
        const length = bytes.indexOf(0);
        if (length === -1) throw new Error("Realtime API returned an unterminated string");
        // The native worker frees its result after this import returns.
        answer = decodeText(bytes.slice(0, length));
      }
      postMessage({
        "type": "wasi-api-completion",
        "id": id >>> 0,
        "generation": generation >>> 0,
        "status": status,
        "answer": answer,
      });
    },
    // Children call only the plugin functions already mirrored in their table.
    "csoundLoadModules": () => -1,
    "csoundLoadExternals": () => -1,
    "csoundWasiJsSystem": () => -1,
    "csoundWasiJsMessageCallback": (csound, attr, length, pointer) =>
      bufferMessageText(text(pointer, length), messagePort, streamBuffer),
    "printDebugCallback": (pointer, length) => postMessage({ "log": text(pointer, length) }),
    "csoundWasiJsDebugCallback": () => postMessage({ "debugCallback": true }),
    "csoundWasiJsReadlineCallback": () => {
      throw new Error("WASI thread workers do not support readline");
    },
  };
}

export function createWasiThreadImports({ module, memory, postMessage }) {
  const wasi = new WASI();
  wasi.setMemory(memory);
  const imports = wasi.getImports(module);
  imports["env"] = {
    ...imports["env"],
    ...createWasiThreadEnvironment({ memory, postMessage }),
  };
  return imports;
}
