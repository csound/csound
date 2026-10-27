import { MemoryFilesystem, writeBytes, sliceBuffers, concatenate } from "./store.js";
import { getGlobalScope } from "../utils/global-scope.js";
import { encoder, decoder } from "../utils/utf8.js";

const monotonic = () => getGlobalScope().performance?.now() ?? Date.now();
const MAX_FILE_SIZE = 0x7fffffff;
// wasi:filesystem/types@0.2.0 error-code discriminants.
const E = {
  access: 0,
  badDescriptor: 3,
  exist: 7,
  tooLarge: 8,
  invalid: 12,
  isDirectory: 14,
  noEntry: 20,
  notDirectory: 24,
  notEmpty: 25,
  unsupported: 27,
  notPermitted: 31,
};
class FilesystemError extends Error {
  constructor(code) {
    super(`WASI filesystem error ${code}`);
    this.code = code;
  }
}
class CommandExit extends Error {
  constructor(code) {
    super(`WASI command exited with status ${code}`);
    this.code = code;
  }
}
const fail = (code) => {
  throw new FilesystemError(code);
};
const parent = (path) => path.slice(0, path.lastIndexOf("/")) || "/";
const fileType = (node) => (node.type === "dir" ? 3 : node.type === "stream" ? 2 : 6);

// Host the canonical ABI of wasm32-wasip2 core modules. The browser keeps the
// Csound C exports, memory and plugin table; standalone commands are packaged
// as components too. Resource handles belong to this instance, not to libc's
// internal POSIX descriptor table.
/** @unrestricted */
export class WASI extends MemoryFilesystem {
  constructor(options = {}) {
    super(options);
    this.resources = new Map();
    this.nextResource = 1;
    this.instance = undefined;
    this.waitControl = undefined;
    this.getRandomValues =
      options.getRandomValues || ((bytes) => getGlobalScope().crypto.getRandomValues(bytes));
  }

  createCommand(args) {
    return new WASI({
      filesystem: this.filesystem,
      root: this.storagePath(this.cwd),
      args,
      getRandomValues: this.getRandomValues,
    });
  }

  chdir(path) {
    const previous = this.cwd;
    const status = super.chdir(path);
    if (status !== 0) return status;
    const nativeExports = this.instance?.["exports"];
    if (!nativeExports || typeof nativeExports["chdir"] !== "function") return 0;
    // libc resolves relative paths before passing them to the preopen. Keep
    // its cwd in sync with the JS filesystem instead of rebasing host paths.
    const bytes = encoder.encode(this.cwd + "\0");
    let pointer;
    try {
      pointer = this.allocate(bytes.length);
      this.bytes(pointer, bytes.length).set(bytes);
      const result = nativeExports["chdir"](pointer);
      if (result !== 0) this.cwd = previous;
      return result;
    } catch (error) {
      this.cwd = previous;
      throw error;
    } finally {
      if (pointer !== undefined)
        nativeExports["cabi_realloc"](pointer, bytes.length, 1, 0);
    }
  }

  setInstance(instance) {
    this.instance = instance;
    if (instance["exports"]["memory"]) this.setMemory(instance["exports"]["memory"]);
    if (typeof instance["exports"]["cabi_realloc"] !== "function")
      throw new TypeError("Preview 2 requires an exported cabi_realloc");
  }

  start(instance) {
    this.setInstance(instance);
    const initialize = instance["exports"]["_initialize"];
    if (typeof initialize !== "function") throw new TypeError("Expected a Preview 2 reactor");
    initialize();
  }

  startCommand(instance) {
    this.setInstance(instance);
    const run = instance["exports"]["wasi:cli/run@0.2.0#run"];
    if (typeof run !== "function") throw new TypeError("Expected a WASI Preview 2 command");
    try {
      return run() === 0 ? 0 : 1;
    } catch (error) {
      if (error instanceof CommandExit) return error.code;
      throw error;
    }
  }

  allocate(length, alignment = 1) {
    if (!length) return 0;
    if (!Number.isSafeInteger(length) || length < 0 || length > MAX_FILE_SIZE) fail(E.tooLarge);
    return this.instance["exports"]["cabi_realloc"](0, 0, alignment, length) >>> 0;
  }

  u8(pointer, value) {
    this.getMemory().setUint8(pointer, value);
  }

  u32(pointer, value) {
    this.getMemory().setUint32(pointer, value, true);
  }

  u64(pointer, value) {
    this.getMemory().setBigUint64(pointer, BigInt(value), true);
  }

  bytes(pointer, length) {
    return new Uint8Array(this.memory.buffer, pointer, length);
  }

  text(pointer, length) {
    return decoder.decode(this.bytes(pointer, length));
  }

  list(pointer, bytes) {
    const data = this.allocate(bytes.length);
    this.bytes(data, bytes.length).set(bytes);
    this.u32(pointer, data);
    this.u32(pointer + 4, bytes.length);
  }

  add(kind, value = {}) {
    const id = this.nextResource++;
    this.resources.set(id, { kind, ...value });
    return id;
  }

  resource(id, kind) {
    const value = this.resources.get(id);
    if (!value || value.kind !== kind) throw new TypeError(`Invalid WASI ${kind} resource ${id}`);
    return value;
  }

  drop(id, kind) {
    this.resource(id, kind);
    this.resources.delete(id);
  }

  result(pointer, alignment, action) {
    try {
      action(pointer + alignment);
      this.u8(pointer, 0);
    } catch (error) {
      if (!(error instanceof FilesystemError)) throw error;
      this.u8(pointer, 1);
      this.u8(pointer + alignment, error.code);
    }
  }

  streamResult(pointer, alignment, action) {
    try {
      action(pointer + alignment);
      this.u8(pointer, 0);
    } catch (error) {
      if (!(error instanceof FilesystemError)) throw error;
      this.u8(pointer, 1);
      this.u8(pointer + alignment, 0);
      this.u32(pointer + alignment + 4, this.add("error", { code: error.code }));
    }
  }

  descriptor(id) {
    return this.resource(id, "descriptor");
  }

  requireFile(handle, flag) {
    if (handle.node.type === "dir") fail(E.isDirectory);
    if (!(handle.flags & flag)) fail(E.access);
    return handle.node;
  }

  offset(value) {
    if (value < BigInt(0) || value > BigInt(MAX_FILE_SIZE)) fail(E.tooLarge);
    return Number(value);
  }

  pathAt(id, pointer, length) {
    const handle = this.descriptor(id);
    if (handle.node.type !== "dir") fail(E.notDirectory);
    if (this.bytes(pointer, length).includes(0)) fail(E.invalid);
    const path = this.text(pointer, length);
    if (path.startsWith("/") || path.includes("\0")) fail(E.notPermitted);
    const boundary = handle.node.path.split("/").filter(Boolean);
    const parts = [...boundary];
    for (const part of path.split("/")) {
      if (part === "..") {
        if (parts.length <= boundary.length) fail(E.notPermitted);
        parts.pop();
      } else if (part && part !== ".") parts.push(part);
    }
    return "/" + parts.join("/");
  }

  nodeAt(id, pointer, length) {
    const node = this.filesystem.nodes.get(this.pathAt(id, pointer, length));
    if (!node) fail(E.noEntry);
    return node;
  }

  requireParent(path) {
    const node = this.filesystem.nodes.get(parent(path));
    if (!node) fail(E.noEntry);
    if (node.type !== "dir") fail(E.notDirectory);
  }

  mutable(id) {
    if (!(this.descriptor(id).flags & 32)) fail(E.access);
  }

  statResult(pointer, node) {
    this.bytes(pointer, 96).fill(0);
    this.u8(pointer, fileType(node));
    this.u64(pointer + 8, 1);
    this.u64(pointer + 16, node.size);
  }

  hashResult(pointer, node) {
    this.u64(pointer, node.inode);
    this.u64(pointer + 8, 0);
  }

  readBytes(node, position, length) {
    const chunks = sliceBuffers(node.buffers, position, Math.min(length, node.size - position));
    return concatenate(
      chunks,
      chunks.reduce((sum, chunk) => sum + chunk.length, 0),
    );
  }

  writeStream(id, pointer, length, ret) {
    this.streamResult(ret, 4, () => {
      const stream = this.resource(id, "output-stream");
      const offset = stream.append ? stream.node.size : stream.position;
      if (offset + length > MAX_FILE_SIZE) fail(E.tooLarge);
      const bytes = new Uint8Array(this.bytes(pointer, length));
      writeBytes(stream.node, offset, bytes);
      stream.position = offset + length;
      if (stream.node.type === "stream") {
        if (!this.command && bytes.length > 0) console.log(decoder.decode(bytes));
        if (stream.node.size > 16000) {
          stream.node.buffers = sliceBuffers(stream.node.buffers, stream.node.size - 16000, 16000);
          stream.node.size = 16000;
        }
      }
    });
  }

  readStream(id, length, ret) {
    const stream = this.resource(id, "input-stream");
    if (stream.position >= stream.node.size && length !== BigInt(0)) {
      this.u8(ret, 1);
      this.u8(ret + 4, 1);
      return;
    }
    this.streamResult(ret, 4, (payload) => {
      const bytes = this.readBytes(stream.node, stream.position, this.offset(length));
      stream.position += bytes.length;
      this.list(payload, bytes);
    });
  }

  openAt(id, pathFlags, pointer, length, openFlags, flags, ret) {
    this.result(ret, 4, (payload) => {
      const path = this.pathAt(id, pointer, length);
      let node = this.filesystem.nodes.get(path);
      if (openFlags & ~15 || flags & ~63 || pathFlags & ~1) fail(E.invalid);
      if (flags & 28) fail(E.unsupported);
      if (!node) {
        if (!(openFlags & 1)) fail(E.noEntry);
        this.mutable(id);
        this.requireParent(path);
        if (openFlags & 2) fail(E.notDirectory);
        node = this.makeNode(path, "file");
      } else if (openFlags & 1 && openFlags & 4) fail(E.exist);
      if (openFlags & 2 && node.type !== "dir") fail(E.notDirectory);
      if (flags & 32 && !(this.descriptor(id).flags & 32)) fail(E.access);
      if (openFlags & 8) {
        if (!(flags & 2)) fail(E.access);
        if (node.type === "dir") fail(E.isDirectory);
        node.buffers = [];
        node.size = 0;
      }
      this.u32(payload, this.add("descriptor", { node, flags }));
    });
  }

  removeAt(id, pointer, length, directory, ret) {
    this.result(ret, 1, () => {
      this.mutable(id);
      const path = this.pathAt(id, pointer, length);
      const node = this.nodeAt(id, pointer, length);
      if (path === this.root) fail(E.notPermitted);
      if (directory && node.type !== "dir") fail(E.notDirectory);
      if (!directory && node.type === "dir") fail(E.isDirectory);
      if (directory && [...this.filesystem.nodes.keys()].some((key) => key.startsWith(path + "/")))
        fail(E.notEmpty);
      this.filesystem.nodes.delete(path);
    });
  }

  renameAt(id, pointer, length, other, target, targetLength, ret) {
    this.result(ret, 1, () => {
      this.mutable(id);
      this.mutable(other);
      const from = this.pathAt(id, pointer, length);
      const to = this.pathAt(other, target, targetLength);
      const node = this.nodeAt(id, pointer, length);
      const existing = this.filesystem.nodes.get(to);
      if (from === to) return;
      if (from === this.root || to === this.root) fail(E.notPermitted);
      this.requireParent(to);
      if (existing && existing.type !== node.type)
        fail(existing.type === "dir" ? E.isDirectory : E.notDirectory);
      if (node.type === "dir" && to.startsWith(from + "/")) fail(E.invalid);
      if (
        existing?.type === "dir" &&
        [...this.filesystem.nodes.keys()].some((key) => key.startsWith(to + "/"))
      )
        fail(E.notEmpty);
      const moving = [...this.filesystem.nodes].filter(
        ([key]) => key === from || key.startsWith(from + "/"),
      );
      for (const [key] of moving) this.filesystem.nodes.delete(key);
      for (const [key, entry] of moving) {
        entry.path = to + key.slice(from.length);
        this.filesystem.nodes.set(entry.path, entry);
      }
    });
  }

  block(id) {
    const pollable = this.resource(id, "pollable");
    let remaining = (pollable.deadline || 0) - monotonic();
    while (remaining > 0) {
      // Local shared Wasm memory works even when cross-origin isolation hides
      // the SharedArrayBuffer constructor. Only worker threads may wait.
      this.waitControl ||= new Int32Array(
        new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true }).buffer,
      );
      try {
        Atomics.wait(this.waitControl, 0, 0, remaining);
      } catch {
        throw new Error("Preview 2 clock waits require a worker that supports Atomics.wait");
      }
      remaining = (pollable.deadline || 0) - monotonic();
    }
  }

  getImports(module) {
    const clock = (ret) => {
      const ms = Date.now();
      this.u64(ret, Math.floor(ms / 1000));
      this.u32(ret + 8, (ms % 1000) * 1000000);
    };
    const stream = (fd, kind) =>
      this.add(kind, { node: this.fd[fd].node, position: 0, append: fd !== 0 });
    const streamFrom =
      (kind, flag, append = false) =>
      (id, offset, ret) =>
        this.result(ret, 4, (payload) => {
          const handle = this.descriptor(id);
          const node = this.requireFile(handle, flag);
          this.u32(payload, this.add(kind, { node, position: this.offset(offset), append }));
        });
    const readViaStream = streamFrom("input-stream", 1);
    const writeViaStream = streamFrom("output-stream", 2);
    const appendViaStream = streamFrom("output-stream", 2, true);
    const flush = (id, ret) => {
      this.resource(id, "output-stream");
      this.u8(ret, 0);
    };
    // WASI import names must survive Closure property renaming.
    // prettier-ignore
    const interfaces = {
      "wasi:cli/environment": {
        "get-environment": (ret) => {
          this.u32(ret, 0);
          this.u32(ret + 4, 0);
        },
        "get-arguments": (ret) => {
          const data = this.allocate(this.args.length * 8, 4);
          for (const [index, bytes] of this.args.entries()) this.list(data + index * 8, bytes);
          this.u32(ret, data);
          this.u32(ret + 4, this.args.length);
        },
        "initial-cwd": (ret) => {
          this.list(ret + 4, encoder.encode(this.cwd));
          this.u8(ret, 1);
        },
      },
      "wasi:cli/exit": {
        "exit": (status) => {
          throw new CommandExit(status ? 1 : 0);
        },
      },
      "wasi:cli/stdin": { "get-stdin": () => stream(0, "input-stream") },
      "wasi:cli/stdout": { "get-stdout": () => stream(1, "output-stream") },
      "wasi:cli/stderr": { "get-stderr": () => stream(2, "output-stream") },
      "wasi:cli/terminal-stdin": { "get-terminal-stdin": (ret) => this.u8(ret, 0) },
      "wasi:cli/terminal-stdout": { "get-terminal-stdout": (ret) => this.u8(ret, 0) },
      "wasi:cli/terminal-stderr": { "get-terminal-stderr": (ret) => this.u8(ret, 0) },
      "wasi:clocks/wall-clock": {
        "now": clock,
        "resolution": (ret) => {
          this.u64(ret, 0);
          this.u32(ret + 8, 1000000);
        },
      },
      "wasi:clocks/monotonic-clock": {
        "now": () => BigInt(Math.floor(monotonic() * 1000000)),
        "resolution": () => BigInt(1000000),
        "subscribe-instant": (when) => this.add("pollable", { deadline: Number(when) / 1000000 }),
        "subscribe-duration": (duration) =>
          this.add("pollable", { deadline: monotonic() + Number(duration) / 1000000 }),
      },
      "wasi:io/poll": {
        "[method]pollable.ready": (id) => {
          const p = this.resource(id, "pollable");
          return !p.deadline || monotonic() >= p.deadline ? 1 : 0;
        },
        "[method]pollable.block": (id) => this.block(id),
        "poll": (pointer, length, ret) => {
          if (!length) throw new TypeError("Cannot poll an empty list");
          const handles = [...new Uint32Array(this.memory.buffer, pointer, length)];
          const deadline = (id) => this.resource(id, "pollable").deadline || 0;
          this.block(handles.reduce((a, b) => (deadline(a) < deadline(b) ? a : b)));
          const ready = handles.flatMap((id, index) =>
            deadline(id) <= monotonic() ? [index] : [],
          );
          const data = this.allocate(ready.length * 4, 4);
          ready.forEach((value, index) => this.u32(data + index * 4, value));
          this.u32(ret, data);
          this.u32(ret + 4, ready.length);
        },
      },
      "wasi:io/error": {
        "[method]error.to-debug-string": (id, ret) =>
          this.list(
            ret,
            encoder.encode(`WASI filesystem error ${this.resource(id, "error").code}`),
          ),
      },
      "wasi:io/streams": {
        "[method]input-stream.read": (id, length, ret) => this.readStream(id, length, ret),
        "[method]input-stream.blocking-read": (id, length, ret) => this.readStream(id, length, ret),
        "[method]input-stream.subscribe": (id) => {
          this.resource(id, "input-stream");
          return this.add("pollable");
        },
        "[method]output-stream.check-write": (id, ret) => {
          this.resource(id, "output-stream");
          this.u8(ret, 0);
          this.u64(ret + 8, 65536);
        },
        "[method]output-stream.write": (id, pointer, length, ret) =>
          this.writeStream(id, pointer, length, ret),
        "[method]output-stream.blocking-write-and-flush": (id, pointer, length, ret) =>
          this.writeStream(id, pointer, length, ret),
        "[method]output-stream.flush": flush,
        "[method]output-stream.blocking-flush": flush,
        "[method]output-stream.subscribe": (id) => {
          this.resource(id, "output-stream");
          return this.add("pollable");
        },
      },
      "wasi:filesystem/preopens": {
        "get-directories": (ret) => {
          const data = this.allocate(12, 4);
          this.u32(
            data,
            this.add("descriptor", {
              node: this.filesystem.nodes.get(this.root),
              flags: 35,
            }),
          );
          this.list(data + 4, encoder.encode("/"));
          this.u32(ret, data);
          this.u32(ret + 4, 1);
        },
      },
      "wasi:filesystem/types": {
        "filesystem-error-code": (id, ret) => {
          const error = this.resource(id, "error");
          this.u8(ret, 1);
          this.u8(ret + 1, error.code);
        },
        "[method]descriptor.open-at": (...args) => this.openAt(...args),
        "[method]descriptor.read-via-stream": readViaStream,
        "[method]descriptor.write-via-stream": writeViaStream,
        "[method]descriptor.append-via-stream": (id, ret) => appendViaStream(id, BigInt(0), ret),
        "[method]descriptor.get-flags": (id, ret) =>
          this.result(ret, 1, (p) => this.u8(p, this.descriptor(id).flags)),
        "[method]descriptor.get-type": (id, ret) =>
          this.result(ret, 1, (p) => this.u8(p, fileType(this.descriptor(id).node))),
        "[method]descriptor.stat": (id, ret) =>
          this.result(ret, 8, (p) => this.statResult(p, this.descriptor(id).node)),
        "[method]descriptor.stat-at": (id, flags, ptr, len, ret) =>
          this.result(ret, 8, (p) => this.statResult(p, this.nodeAt(id, ptr, len))),
        "[method]descriptor.metadata-hash": (id, ret) =>
          this.result(ret, 8, (p) => this.hashResult(p, this.descriptor(id).node)),
        "[method]descriptor.metadata-hash-at": (id, flags, ptr, len, ret) =>
          this.result(ret, 8, (p) => this.hashResult(p, this.nodeAt(id, ptr, len))),
        "[method]descriptor.is-same-object": (id, other) =>
          this.descriptor(id).node === this.descriptor(other).node ? 1 : 0,
        "[method]descriptor.set-size": (id, size, ret) =>
          this.result(ret, 1, () => {
            const node = this.requireFile(this.descriptor(id), 2);
            const length = this.offset(size);
            if (length > node.size) node.buffers.push(new Uint8Array(length - node.size));
            else node.buffers = sliceBuffers(node.buffers, 0, length);
            node.size = length;
          }),
        "[method]descriptor.read": (id, length, offset, ret) =>
          this.result(ret, 4, (p) => {
            const node = this.requireFile(this.descriptor(id), 1);
            const position = this.offset(offset);
            const bytes = this.readBytes(node, position, this.offset(length));
            this.list(p, bytes);
            this.u8(p + 8, position + bytes.length >= node.size ? 1 : 0);
          }),
        "[method]descriptor.write": (id, ptr, len, offset, ret) =>
          this.result(ret, 8, (p) => {
            const node = this.requireFile(this.descriptor(id), 2);
            const position = this.offset(offset);
            if (position + len > MAX_FILE_SIZE) fail(E.tooLarge);
            writeBytes(node, position, new Uint8Array(this.bytes(ptr, len)));
            this.u64(p, len);
          }),
        "[method]descriptor.read-directory": (id, ret) =>
          this.result(ret, 4, (p) => {
            const node = this.descriptor(id).node;
            if (node.type !== "dir") fail(E.notDirectory);
            const entries = [...this.filesystem.nodes.values()].filter(
              (entry) => entry !== node && parent(entry.path) === node.path,
            );
            this.u32(p, this.add("directory-entry-stream", { entries, position: 0 }));
          }),
        "[method]directory-entry-stream.read-directory-entry": (id, ret) =>
          this.result(ret, 4, (p) => {
            const entries = this.resource(id, "directory-entry-stream");
            const node = entries.entries[entries.position++];
            this.u8(p, node ? 1 : 0);
            if (node) {
              this.u8(p + 4, fileType(node));
              this.list(p + 8, encoder.encode(node.path.slice(node.path.lastIndexOf("/") + 1)));
            }
          }),
        "[method]descriptor.create-directory-at": (id, ptr, len, ret) =>
          this.result(ret, 1, () => {
            this.mutable(id);
            const path = this.pathAt(id, ptr, len);
            if (this.filesystem.nodes.has(path)) fail(E.exist);
            this.requireParent(path);
            this.makeNode(path, "dir");
          }),
        "[method]descriptor.unlink-file-at": (id, ptr, len, ret) =>
          this.removeAt(id, ptr, len, false, ret),
        "[method]descriptor.remove-directory-at": (id, ptr, len, ret) =>
          this.removeAt(id, ptr, len, true, ret),
        "[method]descriptor.rename-at": (...args) => this.renameAt(...args),
        "[method]descriptor.sync": (id, ret) => {
          this.descriptor(id);
          this.u8(ret, 0);
        },
        "[method]descriptor.sync-data": (id, ret) => {
          this.descriptor(id);
          this.u8(ret, 0);
        },
        "[method]descriptor.advise": (id, offset, length, advice, ret) => {
          this.descriptor(id);
          this.u8(ret, 0);
        },
      },
    };
    const randomBytes = (length) => {
      const bytes = new Uint8Array(this.offset(length));
      for (let start = 0; start < bytes.length; start += 65536)
        this.getRandomValues(bytes.subarray(start, start + 65536));
      return bytes;
    };
    const randomU64 = () => new DataView(randomBytes(BigInt(8)).buffer).getBigUint64(0, true);
    interfaces["wasi:random/random"] = {
      "get-random-bytes": (length, ret) => this.list(ret, randomBytes(length)),
      "get-random-u64": randomU64,
    };
    interfaces["wasi:random/insecure"] = {
      "get-insecure-random-bytes": (length, ret) => this.list(ret, randomBytes(length)),
      "get-insecure-random-u64": randomU64,
    };
    interfaces["wasi:random/insecure-seed"] = {
      "insecure-seed": (ret) => {
        this.u64(ret, randomU64());
        this.u64(ret + 8, randomU64());
      },
    };
    const resourceTypes = {
      "wasi:io/error": ["error"],
      "wasi:io/poll": ["pollable"],
      "wasi:io/streams": ["input-stream", "output-stream"],
      "wasi:filesystem/types": ["descriptor", "directory-entry-stream"],
      "wasi:cli/terminal-input": ["terminal-input"],
      "wasi:cli/terminal-output": ["terminal-output"],
    };
    for (const [name, kinds] of Object.entries(resourceTypes)) {
      interfaces[name] ||= {};
      for (const kind of kinds)
        interfaces[name][`[resource-drop]${kind}`] = (id) => this.drop(id, kind);
    }
    const imports = {};
    for (const item of WebAssembly.Module.imports(module)) {
      if (item.module === "wasi_snapshot_preview1" || item.module === "wasi")
        throw new TypeError("Preview 1 modules cannot run in the Preview 2 host");
      if (!item.module.startsWith("wasi:")) continue;
      const [name, version] = item.module.split("@");
      const fn = version === "0.2.0" && item.kind === "function" && interfaces[name]?.[item.name];
      if (!fn) throw new TypeError(`Unsupported Preview 2 import ${item.module}#${item.name}`);
      imports[item.module] ||= {};
      imports[item.module][item.name] = fn;
    }
    return imports;
  }
}
