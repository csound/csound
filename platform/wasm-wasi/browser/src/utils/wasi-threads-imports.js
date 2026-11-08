/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

/* eslint-disable object-shorthand -- Import names and output keys are part of the host ABI. */

import { bufferMessageText } from "./message-stream.js";

const ENOSYS = 52;
const EINVAL = 28;
const ENOTSUP = 58;
const EAGAIN = 6;
const MAX_OUTPUT = 16000;

const clockOriginNow = () => globalThis.performance?.timeOrigin ?? 0;
const monotonicMilliseconds = (clockOrigin) => {
  if (globalThis.performance) return performance.timeOrigin + performance.now() - clockOrigin;
  // AudioWorklet exposes its sample clock, but has no performance object.
  return typeof globalThis["currentTime"] === "number"
    ? globalThis["currentTime"] * 1000
    : Date.now() - clockOrigin;
};
/** @return {bigint} */
const nowNanoseconds = (id, clockOrigin) =>
  BigInt(Math.floor((id === 0 ? Date.now() : monotonicMilliseconds(clockOrigin)) * 1e6));

export function createWasiClockImports({
  memory,
  control,
  mayBlock = false,
  waitIndex = 0,
  clockOrigin = clockOriginNow(),
}) {
  return {
    "clock_time_get": (id, precision, pointer) => {
      if (id !== 0 && id !== 1) return EINVAL;
      new DataView(memory.buffer).setBigUint64(pointer, nowNanoseconds(id, clockOrigin), true);
      return 0;
    },
    "clock_res_get": (id, pointer) => {
      if (id !== 0 && id !== 1) return EINVAL;
      new DataView(memory.buffer).setBigUint64(pointer, BigInt(1000000), true);
      return 0;
    },
    "poll_oneoff": (input, output, count, events) => {
      // libc nanosleep uses one clock subscription. File polling needs a shared
      // filesystem owner and cannot be emulated with per-worker descriptor copies.
      if (count === 0) return EINVAL;
      if (count !== 1) return ENOTSUP;
      const view = new DataView(memory.buffer);
      if (view.getUint8(input + 8) !== 0) return ENOTSUP;
      const clock = view.getUint32(input + 16, true);
      const flags = view.getUint16(input + 40, true);
      if ((clock !== 0 && clock !== 1) || flags > 1) return EINVAL;
      let delay = view.getBigUint64(input + 24, true);
      if (flags === 1) delay -= nowNanoseconds(clock, clockOrigin);
      if (mayBlock && control) {
        const expected = Atomics.load(control, waitIndex);
        if (Atomics.load(control, 0) !== 0) throw new Error("WASI thread host stopped");
        if (delay > BigInt(0)) Atomics.wait(control, waitIndex, expected, Number(delay) / 1e6);
        if (Atomics.load(control, 0) !== 0) throw new Error("WASI thread host stopped");
      } else if (delay > BigInt(0)) {
        // UI and AudioWorklet hosts cannot block. ENOSYS would make libc's
        // caller retry nanosleep forever, so fail the unsupported operation.
        throw new Error("Blocking WASI sleep requires a worker thread host");
      }
      const result = new DataView(memory.buffer);
      new Uint8Array(memory.buffer, output, 32).fill(0);
      result.setBigUint64(output, view.getBigUint64(input, true), true);
      result.setUint32(events, 1, true);
      return 0;
    },
  };
}

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

/**
 * Child imports share only Wasm memory, never copied filesystem state. Until a
 * separate filesystem owner exists, file access in background init must fail.
 */
export function createWasiThreadImports({
  module,
  memory,
  control,
  index,
  clockOrigin,
  postMessage,
}) {
  const clocks = createWasiClockImports({
    memory,
    control,
    mayBlock: true,
    waitIndex: 1 + index * 3,
    clockOrigin,
  });
  const emptySizes = (count, size) => {
    const view = new DataView(memory.buffer);
    view.setUint32(count, 0, true);
    view.setUint32(size, 0, true);
    return 0;
  };
  const supported = {
    ...clocks,
    "args_sizes_get": emptySizes,
    "environ_sizes_get": emptySizes,
    "args_get": () => 0,
    "environ_get": () => 0,
    "proc_exit": (code) => {
      throw new Error(`WASI thread called proc_exit(${code})`);
    },
    "random_get": (pointer, length) => {
      for (let offset = 0; offset < length; offset += 65536) {
        const bytes = new Uint8Array(Math.min(65536, length - offset));
        crypto.getRandomValues(bytes);
        new Uint8Array(memory.buffer, pointer + offset, bytes.length).set(bytes);
      }
      return 0;
    },
    "fd_fdstat_get": (fd, pointer) => {
      if (fd !== 1 && fd !== 2) return ENOSYS;
      const view = new DataView(memory.buffer);
      new Uint8Array(memory.buffer, pointer, 24).fill(0);
      view.setUint8(pointer, 2);
      view.setBigUint64(pointer + 8, BigInt(64), true);
      return 0;
    },
    "fd_write": (fd, vectors, count, written) => {
      if (fd !== 1 && fd !== 2) return ENOSYS;
      const view = new DataView(memory.buffer);
      let total = 0;
      let output = "";
      for (let index = 0; index < count; index++) {
        const pointer = view.getUint32(vectors + index * 8, true);
        const length = view.getUint32(vectors + index * 8 + 4, true);
        const bytes = new Uint8Array(memory.buffer, pointer, length);
        if (total < MAX_OUTPUT)
          output += new TextDecoder().decode(bytes.slice(0, MAX_OUTPUT - total));
        total += length;
      }
      if (output) postMessage({ "log": output });
      view.setUint32(written, total, true);
      return 0;
    },
  };
  const wasi = {};
  for (const item of WebAssembly.Module.imports(module)) {
    if (item.module === "wasi_snapshot_preview1" && item.kind === "function")
      wasi[item.name] = supported[item.name] || (() => ENOSYS);
  }
  const nestedSpawn = (argument, resultPointer) => {
    if (resultPointer === undefined) return -EAGAIN;
    const view = new DataView(memory.buffer);
    view.setUint32(resultPointer, 1, true);
    view.setUint32(resultPointer + 4, EAGAIN, true);
    return undefined;
  };
  return {
    "env": createWasiThreadEnvironment({ memory, postMessage }),
    "wasi_snapshot_preview1": wasi,
    "wasi": { "thread-spawn": nestedSpawn },
  };
}
