import { WASI as Preview2, FilesystemError, CommandExit } from "./preview2.js";
import { writeBytes, sliceBuffers } from "./store.js";
import { encoder, decoder } from "../utils/utf8.js";

const supportedVersion = (version) => /^0\.3\.[0-2]$/.test(version);
const monotonic = () => globalThis.performance?.now() ?? Date.now();
const fileType = (node) => (node.type === "dir" ? 2 : node.type === "stream" ? 1 : 5);
const errorCode = (code) => (code > 1 ? code - 1 : code);
const BLOCKED = 0xffffffff;

// Core-module host for the synchronous C API on Preview 3. The filesystem is
// in memory; stream/future operations still keep their ownership and waitable
// state so cancellation and EOF follow the canonical ABI.
/** @unrestricted */
export class WASI extends Preview2 {
  constructor(options = {}) {
    super(options);
    this.stackPointer = 0;
    this.tlsBase = 0;
    this.context = 0;
  }

  createCommand(args) {
    return new WASI({
      filesystem: this.filesystem,
      root: this.storagePath(this.cwd),
      args,
      getRandomValues: this.getRandomValues,
    });
  }

  getStackPointer() {
    return this.stackPointer;
  }

  setStackPointer(value) {
    this.stackPointer = value;
  }

  wrapExports(exports) {
    const wrapped = { ...exports };
    for (const [name, value] of Object.entries(exports))
      if (typeof value === "function" && name !== "__wasm_task_hook")
        wrapped[name] = (...args) => this.task(() => value(...args));
    return wrapped;
  }

  task(action, start = 0) {
    if (this.getStackPointer()) return action();
    const hook = this.instance["exports"]["__wasm_task_hook"];
    if (!hook) return action();
    hook(start);
    const stack = this.getStackPointer();
    try {
      return action();
    } finally {
      this.setStackPointer(stack);
      hook(start + 1);
    }
  }

  start(instance) {
    this.setInstance(instance);
    this.task(() => instance["exports"]["_initialize"](), 6);
  }

  startCommand(instance) {
    this.setInstance(instance);
    const run = Object.entries(instance["exports"]).find(([name]) => {
      const [iface, version] = name.split("@");
      return (
        iface === "wasi:cli/run" &&
        version?.endsWith("#run") &&
        supportedVersion(version.slice(0, -4))
      );
    })?.[1];
    if (!run) throw new TypeError("Expected a WASI Preview 3 command");
    try {
      return this.task(() => (run() === 0 ? 0 : 1));
    } catch (error) {
      if (error instanceof CommandExit) return error.code;
      throw error;
    }
  }

  allocate(length, alignment = 1) {
    return this.task(() => super.allocate(length, alignment), 12);
  }

  result(pointer, alignment, action) {
    super.result(pointer, Math.max(4, alignment), action);
    if (this.getMemory().getUint8(pointer)) {
      const payload = pointer + Math.max(4, alignment);
      this.u8(payload, errorCode(this.getMemory().getUint8(payload)));
    }
  }

  statResult(pointer, node) {
    this.bytes(pointer, 104).fill(0);
    this.u8(pointer, fileType(node));
    this.u64(pointer + 16, 1);
    this.u64(pointer + 24, node.size);
  }

  pair(type) {
    const state = {
      type,
      reader: true,
      writer: true,
      items: [],
      read: undefined,
      write: undefined,
    };
    const reader = this.add(type + "-read", { state });
    const writer = this.add(type + "-write", { state });
    state.readerHandle = reader;
    state.writerHandle = writer;
    return { state, reader, writer };
  }

  future(type, value = undefined) {
    return this.add(type + "-read", { state: { type, value, reader: true, writer: false } });
  }

  readSource(node, position, type = "bytes") {
    return this.add(type + "-read", {
      state: { type, reader: true, writer: false, node, position },
    });
  }

  failure(error) {
    if (!(error instanceof FilesystemError)) throw error;
    return errorCode(error.code);
  }

  readViaStream(id, offset, ret, directory = false) {
    let stream, error;
    try {
      const node = directory ? this.directory(id) : this.requireFile(this.descriptor(id), 1);
      stream = this.readSource(
        node,
        directory ? 0 : this.offset(offset),
        directory ? "directory" : "bytes",
      );
      if (directory)
        this.resources.get(stream).state.items = [...this.filesystem.nodes.values()].filter(
          (entry) =>
            entry !== node &&
            (entry.path.slice(0, entry.path.lastIndexOf("/")) || "/") === node.path,
        );
    } catch (error_) {
      error = this.failure(error_);
      stream = this.readSource({ size: 0, buffers: [] }, 0, directory ? "directory" : "bytes");
      if (directory) this.resources.get(stream).state.items = [];
    }
    this.u32(ret, stream);
    this.u32(ret + 4, this.future("fs-result", error));
  }

  writeViaStream(id, reader, offset, append = false, fd = undefined) {
    const stream = this.resource(reader, "bytes-read");
    const state = stream.state;
    if (stream.pending || state.sink || state.node)
      throw new TypeError("Stream already transferred");
    let error;
    try {
      const node = fd === undefined ? this.requireFile(this.descriptor(id), 2) : this.fd[fd].node;
      state.sink = { node, position: this.offset(offset), append, cli: fd !== undefined };
    } catch (error_) {
      error = this.failure(error_);
      state.reader = false;
    }
    this.resources.delete(reader);
    const future = this.future(fd === undefined ? "fs-result" : "cli-result", error);
    this.resources.get(future).state.pendingStream = state;
    this.pump(state);
    return future;
  }

  transfer(type, direction, id, pointer, length = 1) {
    const endpoint = this.resource(id, type + "-" + direction);
    const state = endpoint.state;
    if (endpoint.pending) throw new TypeError("An operation is already pending on this endpoint");
    if (type.endsWith("-result")) {
      if (direction !== "read")
        throw new TypeError("Guest-written WASI result futures are unsupported");
      if (state.consumed) throw new TypeError("Future already consumed");
      if (state.pendingStream?.writer && state.pendingStream?.reader) {
        endpoint.pending = { pointer, length, direction };
        return BLOCKED;
      }
      state.consumed = true;
      if (state.pendingStream?.error !== undefined) state.value = state.pendingStream.error;
      this.u8(pointer, state.value === undefined ? 0 : 1);
      if (state.value !== undefined) this.u8(pointer + (type === "fs-result" ? 4 : 1), state.value);
      return 0;
    }
    if (!Number.isSafeInteger(length) || length < 0 || length > 0x0fffffff)
      throw new RangeError("Invalid stream item count");
    if (direction === "read" && state.node) {
      let count;
      if (type === "directory") {
        count = Math.min(length, state.items.length - state.position);
        for (let index = 0; index < count; index++) {
          const node = state.items[state.position + index];
          const ptr = pointer + index * 24;
          this.bytes(ptr, 24).fill(0);
          this.u8(ptr, fileType(node));
          this.list(ptr + 16, encoder.encode(node.path.slice(node.path.lastIndexOf("/") + 1)));
        }
      } else {
        const bytes = this.readBytes(state.node, state.position, length);
        this.bytes(pointer, bytes.length).set(bytes);
        count = bytes.length;
      }
      state.position += count;
      return (count * 16) | (count === 0 && length > 0 ? 1 : 0);
    }
    if (direction === "write" && state.sink) {
      if (!state.reader) return 1;
      const { node, append } = state.sink;
      const position = append ? node.size : state.sink.position;
      if (length && position + length > 0x7fffffff) {
        state.error = state.sink.cli ? 0 : 7;
        state.reader = false;
        this.pump(state);
        return 1;
      }
      const bytes = new Uint8Array(this.bytes(pointer, length));
      writeBytes(node, position, bytes);
      state.sink.position = position + length;
      if (node.type === "stream") {
        if (!this.command && length) console.log(decoder.decode(bytes));
        if (node.size > 16000) {
          node.buffers = sliceBuffers(node.buffers, node.size - 16000, 16000);
          node.size = 16000;
        }
      }
      return length * 16;
    }
    if (!state[direction === "read" ? "writer" : "reader"]) return 1;
    if (!length) return 0;
    endpoint.pending = { pointer, length, direction };
    state[direction] = endpoint;
    this.pump(state);
    if (endpoint.event !== undefined) {
      const status = endpoint.event;
      delete endpoint.event;
      return status;
    }
    return BLOCKED;
  }

  complete(endpoint, status) {
    endpoint.event = status;
    delete endpoint.pending;
  }

  pump(state) {
    if (state.sink && state.write?.pending) {
      const endpoint = state.write;
      const { pointer, length } = endpoint.pending;
      delete endpoint.pending;
      this.complete(
        endpoint,
        this.transfer(state.type, "write", this.handleOf(endpoint), pointer, length),
      );
    }
    if (state.read?.pending && state.write?.pending) {
      if (state.type !== "bytes") throw new TypeError("Unsupported guest stream payload");
      const read = state.read.pending;
      const write = state.write.pending;
      const count = Math.min(read.length, write.length);
      this.bytes(read.pointer, count).set(new Uint8Array(this.bytes(write.pointer, count)));
      this.complete(state.read, count * 16);
      this.complete(state.write, count * 16);
    }
    for (const direction of ["read", "write"]) {
      const endpoint = state[direction];
      if (endpoint?.pending && !state[direction === "read" ? "writer" : "reader"])
        this.complete(endpoint, 1);
    }
    for (const endpoint of this.resources.values()) {
      if (
        endpoint.pending &&
        endpoint.state?.pendingStream === state &&
        (!state.writer || !state.reader)
      ) {
        const { pointer } = endpoint.pending;
        delete endpoint.pending;
        this.complete(
          endpoint,
          this.transfer(endpoint.state.type, "read", this.handleOf(endpoint), pointer),
        );
      }
    }
  }

  handleOf(endpoint) {
    for (const [id, value] of this.resources) if (value === endpoint) return id;
    throw new TypeError("Unknown endpoint");
  }

  cancel(type, direction, id) {
    const endpoint = this.resource(id, type + "-" + direction);
    if (endpoint.event !== undefined) {
      const event = endpoint.event;
      delete endpoint.event;
      return event;
    }
    if (!endpoint.pending) throw new TypeError("No pending operation to cancel");
    delete endpoint.pending;
    return 2;
  }

  dropEndpoint(type, direction, id) {
    const endpoint = this.resource(id, type + "-" + direction);
    if (endpoint.pending) throw new TypeError("Cannot drop an endpoint with a pending operation");
    endpoint.state[direction === "read" ? "reader" : "writer"] = false;
    this.resources.delete(id);
    this.pump(endpoint.state);
  }

  clockTask(deadline) {
    return deadline <= monotonic() ? 2 : this.add("subtask", { deadline }) * 16 + 1;
  }

  wait(set, pointer, block) {
    const members = this.resource(set, "waitable-set").members;
    for (;;) {
      let deadline = Infinity;
      for (const id of members) {
        const value = this.resources.get(id);
        if (!value) throw new TypeError("Waitable was dropped while joined");
        if (value.kind === "subtask" && !value.delivered) {
          if (value.deadline <= monotonic()) value.event = 2;
          else deadline = Math.min(deadline, value.deadline);
        }
        if (value.event === undefined) continue;
        this.u32(pointer, id);
        this.u32(pointer + 4, value.event);
        delete value.event;
        value.delivered = true;
        return value.kind === "subtask"
          ? 1
          : value.kind.endsWith("-read")
            ? value.state.type.endsWith("-result")
              ? 4
              : 2
            : value.state.type.endsWith("-result")
              ? 5
              : 3;
      }
      if (!block) return 0;
      if (!Number.isFinite(deadline))
        throw new Error("No synchronous host event can complete this wait");
      const pollable = this.add("pollable", { deadline });
      try {
        this.block(pollable);
      } finally {
        this.drop(pollable, "pollable");
      }
    }
  }

  rootImports() {
    return {
      "[waitable-set-new]": () => this.add("waitable-set", { members: new Set() }),
      "[waitable-set-drop]": (set) => {
        if (this.resource(set, "waitable-set").members.size > 0)
          throw new TypeError("Waitable set is not empty");
        this.drop(set, "waitable-set");
      },
      "[waitable-join]": (id, set) => {
        const value = this.resources.get(id);
        if (!value) throw new TypeError("Invalid waitable");
        if (value.set) this.resource(value.set, "waitable-set").members.delete(id);
        if (set) this.resource(set, "waitable-set").members.add(id);
        value.set = set;
      },
      "[waitable-set-wait]": (set, pointer) => this.wait(set, pointer, true),
      "[waitable-set-poll]": (set, pointer) => this.wait(set, pointer, false),
      "[subtask-cancel]": (id) => {
        const subtask = this.resource(id, "subtask");
        const status = subtask.deadline <= monotonic() ? 2 : 3;
        subtask.event = status;
        return status;
      },
      "[subtask-drop]": (id) => this.drop(id, "subtask"),
      "[context-get-0]": () => this.context,
      "[context-set-0]": (value) => {
        this.context = value;
      },
      "[thread-yield]": () => 0,
      "[backpressure-inc]": () => {},
      "[backpressure-dec]": () => {},
    };
  }

  getInterfaces() {
    const interfaces = super.getInterfaces();
    const environment = interfaces["wasi:cli/environment"];
    environment["get-initial-cwd"] = environment["initial-cwd"];
    delete environment["initial-cwd"];
    interfaces["wasi:cli/stdin"] = {
      "read-via-stream": (ret) => {
        this.u32(ret, this.readSource(this.fd[0].node, 0));
        this.u32(ret + 4, this.future("cli-result"));
      },
    };
    for (const [name, fd] of [
      ["stdout", 1],
      ["stderr", 2],
    ])
      interfaces["wasi:cli/" + name] = {
        "write-via-stream": (reader) => this.writeViaStream(0, reader, BigInt(0), true, fd),
      };
    interfaces["wasi:clocks/system-clock"] = {
      now: interfaces["wasi:clocks/wall-clock"]["now"],
      "get-resolution": () => BigInt(1000000),
    };
    interfaces["wasi:clocks/monotonic-clock"] = {
      now: () => BigInt(Math.floor(monotonic() * 1000000)),
      "get-resolution": () => BigInt(1000000),
      "[async-lower]wait-until": (when) => this.clockTask(Number(when) / 1000000),
      "[async-lower]wait-for": (duration) =>
        this.clockTask(monotonic() + Number(duration) / 1000000),
    };
    const seed = interfaces["wasi:random/insecure-seed"];
    seed["get-insecure-seed"] = seed["insecure-seed"];
    delete seed["insecure-seed"];
    const fs = interfaces["wasi:filesystem/types"];
    for (const name of [
      "filesystem-error-code",
      "[method]descriptor.read",
      "[method]descriptor.write",
      "[method]directory-entry-stream.read-directory-entry",
      "[resource-drop]directory-entry-stream",
    ])
      delete fs[name];
    Object.assign(fs, {
      "[method]descriptor.read-via-stream": (...args) => this.readViaStream(...args),
      "[method]descriptor.write-via-stream": (...args) => this.writeViaStream(...args),
      "[method]descriptor.append-via-stream": (id, reader) =>
        this.writeViaStream(id, reader, BigInt(0), true),
      "[method]descriptor.read-directory": (id, ret) =>
        this.readViaStream(id, BigInt(0), ret, true),
      "[method]descriptor.get-type": (id, ret) =>
        this.result(ret, 4, (p) => {
          this.bytes(p, 16).fill(0);
          this.u8(p, fileType(this.descriptor(id).node));
        }),
      "[method]descriptor.is-same-object": (id, other, ret) =>
        this.u8(ret, this.descriptor(id).node === this.descriptor(other).node ? 1 : 0),
    });
    for (const [name, fn] of Object.entries(fs))
      if (
        name.startsWith("[method]descriptor.") &&
        !name.endsWith("-via-stream") &&
        !name.endsWith(".read-directory")
      )
        fs["[async-lower]" + name] = (...args) => {
          fn(...args);
          return 2;
        };
    for (const name of [
      "wasi:io/error",
      "wasi:io/streams",
      "wasi:io/poll",
      "wasi:clocks/wall-clock",
    ])
      delete interfaces[name];
    return interfaces;
  }

  intrinsic(module, name) {
    const match =
      /^(?:\[async-lower\])?\[(stream|future)-(new|read|write|cancel-read|cancel-write|drop-readable|drop-writable)-(\d+)\](.*)$/.exec(
        name,
      );
    if (!match) return undefined;
    const [, kind, operation, index, fn] = match;
    const cli = /^wasi:cli\/(stdin|stdout|stderr)$/.test(module);
    const fs = module === "wasi:filesystem/types";
    if (
      !(cli && /^(read|write)-via-stream$/.test(fn)) &&
      !(
        fs &&
        /^\[method\]descriptor\.(read-via-stream|write-via-stream|append-via-stream|read-directory)$/.test(
          fn,
        )
      )
    )
      return undefined;
    if ((kind === "stream" && index !== "0") || (kind === "future" && index !== "1"))
      return undefined;
    const type =
      kind === "future"
        ? cli
          ? "cli-result"
          : "fs-result"
        : fn.endsWith(".read-directory")
          ? "directory"
          : "bytes";
    if (operation === "new")
      return () => {
        const { reader, writer } = this.pair(type);
        return BigInt(reader) | (BigInt(writer) << BigInt(32));
      };
    const direction =
      operation.includes("write") || operation.endsWith("writable") ? "write" : "read";
    if (operation.startsWith("drop-")) return (id) => this.dropEndpoint(type, direction, id);
    return operation.startsWith("cancel-")
      ? (id) => this.cancel(type, direction, id)
      : (id, pointer, length) => this.transfer(type, direction, id, pointer, length);
  }

  getImports(module) {
    const interfaces = this.getInterfaces();
    const root = this.rootImports();
    const env = {
      "__wasm_get_stack_pointer": () => this.stackPointer,
      "__wasm_set_stack_pointer": (value) => {
        this.stackPointer = value;
      },
      "__wasm_get_tls_base": () => this.tlsBase,
      "__wasm_set_tls_base": (value) => {
        this.tlsBase = value;
      },
    };
    const imports = {};
    for (const item of WebAssembly.Module.imports(module)) {
      const [name, version] = item.module.split("@");
      let fn;
      if (name === "env") fn = env[item.name];
      else if (name === "$root") fn = root[item.name];
      else if (supportedVersion(version))
        fn = interfaces[name]?.[item.name] || this.intrinsic(name, item.name);
      else if (!name.startsWith("wasi") && name !== "[export]$root") continue;
      if (!fn) {
        if (name === "env") continue;
        throw new TypeError(`Unsupported Preview 3 import ${item.module}#${item.name}`);
      }
      imports[item.module] ||= {};
      imports[item.module][item.name] = fn;
    }
    return imports;
  }
}
