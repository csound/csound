/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { encoder, decoder } from "../utils/text-encoders.js";
import { getGlobalScope } from "../utils/global-scope.js";
import * as constants from "./constants.js";

const ZERO = BigInt(0);
const MAX_FILE_SIZE = 0x7fffffff;
const OUTPUT_LIMIT = 16000;
// Keep the WASI import surface separate from the JavaScript filesystem API.
const SYSCALLS = new Set([
  "args_sizes_get",
  "args_get",
  "environ_sizes_get",
  "environ_get",
  "proc_exit",
  "clock_time_get",
  "clock_res_get",
  "random_get",
  "fd_prestat_get",
  "fd_prestat_dir_name",
  "fd_close",
  "fd_fdstat_get",
  "fd_fdstat_set_flags",
  "fd_filestat_get",
  "fd_filestat_set_size",
  "fd_read",
  "fd_write",
  "fd_seek",
  "fd_tell",
  "fd_pread",
  "fd_pwrite",
  "fd_sync",
  "fd_datasync",
  "path_open",
  "path_filestat_get",
  "path_create_directory",
  "path_unlink_file",
  "path_remove_directory",
  "path_rename",
  "fd_readdir",
]);

/**
 * @param {bigint} rights
 * @param {bigint} right
 * @return {boolean}
 */
function hasRight(rights, right) {
  return (rights & right) !== ZERO;
}

function absolutePath(base, path) {
  const parts = path.startsWith("/") ? [] : base.split("/").filter(Boolean);
  for (const part of path.split("/")) {
    if (part === "..") parts.pop();
    else if (part && part !== ".") parts.push(part);
  }
  return `/${parts.join("/")}`;
}

function parentPath(path) {
  return path.slice(0, path.lastIndexOf("/")) || "/";
}

function nodeType(node) {
  return node.type === "dir"
    ? constants.WASI_FILETYPE_DIRECTORY
    : node.type === "stream"
      ? constants.WASI_FILETYPE_CHARACTER_DEVICE
      : constants.WASI_FILETYPE_REGULAR_FILE;
}

function sliceBuffers(buffers, start, length) {
  const result = [];
  for (const buffer of buffers) {
    if (length <= 0) break;
    if (start >= buffer.length) start -= buffer.length;
    else {
      const chunk = buffer.subarray(start, start + length);
      result.push(chunk);
      length -= chunk.length;
      start = 0;
    }
  }
  return result;
}

function concatenate(buffers, size) {
  const result = new Uint8Array(size);
  let offset = 0;
  for (const buffer of buffers) {
    result.set(buffer, offset);
    offset += buffer.length;
  }
  return result;
}

function writeBytes(node, offset, bytes) {
  if (offset === node.size) node.buffers.push(bytes);
  else {
    const before = sliceBuffers(node.buffers, 0, offset);
    if (offset > node.size) before.push(new Uint8Array(offset - node.size));
    node.buffers = [
      ...before,
      bytes,
      ...sliceBuffers(node.buffers, offset + bytes.length, node.size),
    ];
  }
  node.size = Math.max(node.size, offset + bytes.length);
}

class WasiExit extends Error {
  constructor(code) {
    super(`WASI command exited with status ${code}`);
    this.code = code;
  }
}

// A filesystem node owns the bytes; a descriptor owns its position and rights.
// Reactors and commands use the same callbacks without sharing open handles.
// WASI imports select syscall methods by name.
/** @unrestricted */
export class WASI {
  constructor({ filesystem, root = "/", args, preopens } = {}) {
    this.filesystem = filesystem || { nodes: new Map(), nextInode: 1 };
    this.root = root;
    this.cwd = "/";
    this.command = args !== undefined;
    this.args = (args || []).map((arg) => encoder.encode(arg));
    this.preopens = preopens || {};
    this.CPUTIME_START = Date.now();
    /** @type {WebAssembly.Memory|undefined} */
    this.memory = undefined;
    /** @type {DataView|undefined} */
    this.view = undefined;
    this.output = {};
    if (!this.filesystem.nodes.has("/")) this.makeNode("/", "dir");
    // WASI passes rights and file offsets as 64-bit integers, exposed as BigInt.
    /** @type {!Array<{node: ?, seekPos: bigint, flags: number, rights: bigint}>} */
    this.fd = [0, 1, 2].map((fd) => ({
      node: { type: "stream", buffers: [], size: 0, inode: fd },
      seekPos: ZERO,
      flags: 0,
      rights: fd === 0 ? constants.WASI_RIGHT_FD_READ : constants.WASI_RIGHT_FD_WRITE,
    }));
    this.fd[3] = {
      node: this.filesystem.nodes.get(root),
      seekPos: ZERO,
      flags: 0,
      rights: constants.RIGHTS_DIRECTORY_BASE,
    };
  }

  createCommand(args) {
    return new WASI({ filesystem: this.filesystem, root: this.storagePath(this.cwd), args });
  }

  start(instance) {
    const initialize = instance["exports"]["_initialize"];
    if (typeof initialize !== "function")
      throw new TypeError("Browser WASI module does not export _initialize");
    initialize();
  }

  startCommand(instance) {
    const exports = instance["exports"];
    if (
      !(exports["memory"] instanceof WebAssembly.Memory) ||
      typeof exports["_start"] !== "function"
    ) {
      throw new TypeError("Expected a WASI Preview 1 command with memory and _start exports");
    }
    this.setMemory(exports["memory"]);
    try {
      exports["_start"]();
      return 0;
    } catch (error) {
      if (error instanceof WasiExit) return error.code;
      throw error;
    }
  }

  getImports(module) {
    const options = {};
    for (const item of WebAssembly.Module.imports(module)) {
      if (item.kind !== "function" || item.module !== "wasi_snapshot_preview1") continue;
      options[item.module] ||= {};
      // An unimplemented syscall must fail, not claim it performed an operation.
      options[item.module][item.name] = SYSCALLS.has(item.name)
        ? this[item.name].bind(this)
        : () => constants.WASI_ENOSYS;
    }
    return options;
  }

  setMemory(memory) {
    this.memory = memory;
    this.view = undefined;
  }

  getMemory() {
    if (!this.view || this.view.buffer !== this.memory.buffer)
      this.view = new DataView(this.memory.buffer);
    return this.view;
  }

  resolvePath(path) {
    return absolutePath(this.cwd, path);
  }

  storagePath(path) {
    const absolute = this.resolvePath(path);
    return this.root === "/" ? absolute : this.root + (absolute === "/" ? "" : absolute);
  }

  findEntry(path) {
    return this.filesystem.nodes.get(this.storagePath(path));
  }

  pathExists(path) {
    return !!this.findEntry(path);
  }

  makeNode(path, type) {
    const node = { path, type, buffers: [], size: 0, inode: this.filesystem.nextInode++ };
    this.filesystem.nodes.set(path, node);
    return node;
  }

  chdir(path) {
    const entry = this.findEntry(path);
    if (!entry) return constants.WASI_ENOENT;
    if (entry.type !== "dir") return constants.WASI_ENOTDIR;
    this.cwd = this.resolvePath(path);
    return 0;
  }

  // JS filesystem methods use guest paths. A command's root is the directory
  // selected by its parent, so even absolute paths cannot escape that mount.
  readdir(path) {
    const prefix = this.storagePath(path).replace(/\/$/, "") + "/";
    return [...this.filesystem.nodes.keys()]
      .filter((name) => name.startsWith(prefix))
      .map((name) => name.slice(prefix.length))
      .filter((name) => name && !name.includes("/"));
  }

  mkdir(path) {
    const full = this.storagePath(path);
    let current = "";
    for (const part of full.split("/").filter(Boolean)) {
      current += `/${part}`;
      const node = this.filesystem.nodes.get(current);
      if (node && node.type !== "dir") throw new Error(`Not a directory: ${path}`);
      if (!node) this.makeNode(current, "dir");
    }
  }

  writeFile(path, data) {
    const full = this.storagePath(path);
    let node = this.filesystem.nodes.get(full);
    if (node?.type === "dir") throw new Error(`Cannot write a directory: ${path}`);
    if (!node) {
      this.mkdir(parentPath(this.resolvePath(path)));
      node = this.makeNode(full, "file");
    }
    node.buffers = [data];
    node.size = data.length;
  }

  appendFile(path, data) {
    const node = this.findEntry(path);
    if (!node || node.type !== "file") throw new Error(`File not found: ${path}`);
    writeBytes(node, node.size, data);
  }

  readFile(path) {
    const node = this.findEntry(path);
    return node?.type === "file" ? concatenate(node.buffers, node.size) : undefined;
  }

  unlink(path) {
    this.filesystem.nodes.delete(this.storagePath(path));
  }

  readStdOut() {
    const node = this.fd[1]?.node;
    return node && concatenate(node.buffers, node.size);
  }

  readOutput(fd) {
    const node = this.output?.[fd] || this.fd[fd]?.node;
    return node ? decoder.decode(concatenate(node.buffers, node.size)) : "";
  }

  stat(path) {
    const node = this.findEntry(path);
    if (!node) return undefined;
    const directory = node.type === "dir";
    return {
      dev: 0,
      ino: node.inode,
      mode: directory ? 16877 : 33188,
      nlink: 1,
      uid: 0,
      gid: 0,
      rdev: 0,
      size: node.size,
      blksize: 4096,
      blocks: Math.ceil(node.size / 512),
      atimeMs: this.CPUTIME_START,
      mtimeMs: this.CPUTIME_START,
      ctimeMs: this.CPUTIME_START,
      birthtimeMs: this.CPUTIME_START,
      atime: new Date(this.CPUTIME_START),
      mtime: new Date(this.CPUTIME_START),
      ctime: new Date(this.CPUTIME_START),
      birthtime: new Date(this.CPUTIME_START),
      isFile: !directory,
      isDirectory: directory,
      isBlockDevice: false,
      isCharacterDevice: false,
      isSymbolicLink: false,
      isFIFO: false,
      isSocket: false,
    };
  }

  args_sizes_get(argc, size) {
    const view = this.getMemory();
    view.setUint32(argc, this.args.length, true);
    view.setUint32(
      size,
      this.args.reduce((sum, arg) => sum + arg.length + 1, 0),
      true,
    );
    return 0;
  }

  args_get(argv, buffer) {
    const view = this.getMemory();
    const bytes = new Uint8Array(this.memory.buffer);
    for (const arg of this.args) {
      view.setUint32(argv, buffer, true);
      argv += 4;
      bytes.set(arg, buffer);
      bytes[buffer + arg.length] = 0;
      buffer += arg.length + 1;
    }
    return 0;
  }

  environ_sizes_get(count, size) {
    const view = this.getMemory();
    view.setUint32(count, 0, true);
    view.setUint32(size, 0, true);
    return 0;
  }

  environ_get() {
    return 0;
  }

  proc_exit(code) {
    throw new WasiExit(code);
  }

  clock_time_get(id, precision, pointer) {
    let ms;
    switch (id) {
      case constants.WASI_CLOCK_REALTIME: {
        ms = Date.now();
        break;
      }
      case constants.WASI_CLOCK_MONOTONIC: {
        ms = getGlobalScope().performance?.now() ?? Date.now();
        break;
      }
      case constants.WASI_CLOCK_PROCESS_CPUTIME_ID:
      case constants.WASI_CLOCK_THREAD_CPUTIME_ID: {
        ms = Date.now() - this.CPUTIME_START;
        break;
      }
      default: {
        return constants.WASI_EINVAL;
      }
    }
    this.getMemory().setBigUint64(pointer, BigInt(Math.floor(ms * 1000)) * BigInt(1000), true);
    return 0;
  }

  clock_res_get(id, pointer) {
    if (id < 0 || id > 3) return constants.WASI_EINVAL;
    this.getMemory().setBigUint64(pointer, BigInt(1000000), true);
    return 0;
  }

  random_get(pointer, length) {
    const crypto = getGlobalScope().crypto;
    if (!crypto?.getRandomValues) return constants.WASI_ENOSYS;
    for (let offset = 0; offset < length; offset += 65536) {
      crypto.getRandomValues(
        new Uint8Array(this.memory.buffer, pointer + offset, Math.min(65536, length - offset)),
      );
    }
    return 0;
  }

  fd_prestat_get(fd, pointer) {
    if (fd !== 3 || !this.fd[fd]) return constants.WASI_EBADF;
    const view = this.getMemory();
    view.setUint32(pointer, 0, true);
    view.setUint32(pointer + 4, 1, true);
    return 0;
  }

  fd_prestat_dir_name(fd, pointer, length) {
    if (fd !== 3 || !this.fd[fd]) return constants.WASI_EBADF;
    if (length < 1) return constants.WASI_ENAMETOOLONG;
    this.getMemory().setUint8(pointer, 47);
    return 0;
  }

  fd_close(fd) {
    if (!this.fd[fd]) return constants.WASI_EBADF;
    if (fd === 1 || fd === 2) {
      this.output ||= {};
      this.output[fd] = this.fd[fd].node;
    }
    delete this.fd[fd];
    return 0;
  }

  fd_fdstat_get(fd, pointer) {
    const handle = this.fd[fd];
    if (!handle) return constants.WASI_EBADF;
    const view = this.getMemory();
    new Uint8Array(this.memory.buffer, pointer, 24).fill(0);
    view.setUint8(pointer, nodeType(handle.node));
    view.setUint16(pointer + 2, handle.flags, true);
    view.setBigUint64(pointer + 8, handle.rights, true);
    view.setBigUint64(
      pointer + 16,
      constants.RIGHTS_REGULAR_FILE_BASE | constants.RIGHTS_DIRECTORY_BASE,
      true,
    );
    return 0;
  }

  fd_fdstat_set_flags(fd, flags) {
    if (!this.fd[fd]) return constants.WASI_EBADF;
    this.fd[fd].flags = flags;
    return 0;
  }

  writeStat(node, pointer) {
    const view = this.getMemory();
    new Uint8Array(this.memory.buffer, pointer, 64).fill(0);
    view.setBigUint64(pointer + 8, BigInt(node.inode), true);
    view.setUint8(pointer + 16, nodeType(node));
    view.setBigUint64(pointer + 24, BigInt(1), true);
    view.setBigUint64(pointer + 32, BigInt(node.size), true);
    return 0;
  }

  fd_filestat_get(fd, pointer) {
    return this.fd[fd] ? this.writeStat(this.fd[fd].node, pointer) : constants.WASI_EBADF;
  }

  fd_filestat_set_size(fd, size) {
    const handle = this.fd[fd];
    if (!handle) return constants.WASI_EBADF;
    if (!hasRight(handle.rights, constants.WASI_RIGHT_FD_WRITE)) return constants.WASI_ENOTCAPABLE;
    if (size < ZERO || size > BigInt(MAX_FILE_SIZE)) return constants.WASI_EFBIG;
    const node = handle.node;
    const length = Number(size);
    if (node.type !== "file") return constants.WASI_EINVAL;
    if (length > node.size) node.buffers.push(new Uint8Array(length - node.size));
    else node.buffers = sliceBuffers(node.buffers, 0, length);
    node.size = length;
    return 0;
  }

  fd_read(fd, iovs, count, readPointer) {
    const handle = this.fd[fd];
    if (!handle) return constants.WASI_EBADF;
    if (!hasRight(handle.rights, constants.WASI_RIGHT_FD_READ)) return constants.WASI_ENOTCAPABLE;
    if (handle.node.type === "dir") return constants.WASI_EISDIR;
    const view = this.getMemory();
    let read = 0;
    for (let i = 0; i < count; i++) {
      const pointer = view.getUint32(iovs + i * 8, true);
      const length = view.getUint32(iovs + i * 8 + 4, true);
      const chunks = sliceBuffers(handle.node.buffers, Number(handle.seekPos), length);
      let offset = pointer;
      for (const chunk of chunks) {
        new Uint8Array(this.memory.buffer, offset, chunk.length).set(chunk);
        offset += chunk.length;
      }
      const copied = offset - pointer;
      read += copied;
      handle.seekPos += BigInt(copied);
      if (copied < length) break;
    }
    view.setUint32(readPointer, read, true);
    return 0;
  }

  fd_write(fd, iovs, count, writtenPointer) {
    const handle = this.fd[fd];
    if (!handle) return constants.WASI_EBADF;
    if (!hasRight(handle.rights, constants.WASI_RIGHT_FD_WRITE)) return constants.WASI_ENOTCAPABLE;
    if (handle.node.type === "dir") return constants.WASI_EISDIR;
    const view = this.getMemory();
    const node = handle.node;
    let written = 0;
    for (let i = 0; i < count; i++) {
      const pointer = view.getUint32(iovs + i * 8, true);
      const length = view.getUint32(iovs + i * 8 + 4, true);
      const offset =
        handle.flags & 1 || node.type === "stream" ? node.size : Number(handle.seekPos);
      if (offset + length > MAX_FILE_SIZE) return constants.WASI_EFBIG;
      const bytes = new Uint8Array(this.memory.buffer, pointer, length).slice();
      if (length) writeBytes(node, offset, bytes);
      handle.seekPos = BigInt(offset + length);
      written += length;
      if (node.type === "stream") {
        if (!this.command && bytes.length > 0) console.log(decoder.decode(bytes));
        if (node.size > OUTPUT_LIMIT) {
          node.buffers = sliceBuffers(node.buffers, node.size - OUTPUT_LIMIT, OUTPUT_LIMIT);
          node.size = OUTPUT_LIMIT;
        }
      }
    }
    view.setUint32(writtenPointer, written, true);
    return 0;
  }

  /** @param {bigint} offset */
  fd_seek(fd, offset, whence, pointer) {
    const handle = this.fd[fd];
    if (!handle) return constants.WASI_EBADF;
    if (handle.node.type !== "file") return constants.WASI_ESPIPE;
    let position;
    switch (whence) {
      case constants.WASI_WHENCE_SET: {
        position = offset;
        break;
      }
      case constants.WASI_WHENCE_CUR: {
        position = handle.seekPos + offset;
        break;
      }
      case constants.WASI_WHENCE_END: {
        position = BigInt(handle.node.size) + offset;
        break;
      }
      default: {
        return constants.WASI_EINVAL;
      }
    }
    if (position < ZERO || position > BigInt(MAX_FILE_SIZE)) return constants.WASI_EINVAL;
    handle.seekPos = position;
    this.getMemory().setBigUint64(pointer, position, true);
    return 0;
  }

  fd_tell(fd, pointer) {
    if (!this.fd[fd]) return constants.WASI_EBADF;
    this.getMemory().setBigUint64(pointer, this.fd[fd].seekPos, true);
    return 0;
  }

  fd_pread(fd, iovs, count, offset, result) {
    return this.positionedIO(fd, iovs, count, offset, result, false);
  }

  fd_pwrite(fd, iovs, count, offset, result) {
    return this.positionedIO(fd, iovs, count, offset, result, true);
  }

  positionedIO(fd, iovs, count, offset, result, writing) {
    const handle = this.fd[fd];
    if (!handle) return constants.WASI_EBADF;
    if (handle.node.type !== "file") return constants.WASI_ESPIPE;
    if (offset < ZERO || offset > BigInt(MAX_FILE_SIZE)) return constants.WASI_EINVAL;
    const previous = handle.seekPos;
    handle.seekPos = offset;
    try {
      return writing
        ? this.fd_write(fd, iovs, count, result)
        : this.fd_read(fd, iovs, count, result);
    } finally {
      handle.seekPos = previous;
    }
  }

  fd_sync(fd) {
    return this.fd[fd] ? 0 : constants.WASI_EBADF;
  }

  fd_datasync(fd) {
    return this.fd_sync(fd);
  }

  // Resolve a WASI path relative to an open directory. The root descriptor
  // follows the wrapper's cwd, since browser fs.chdir() does not call libc.
  pathAt(fd, pointer, length) {
    const handle = this.fd[fd];
    if (!handle) return { error: constants.WASI_EBADF };
    if (handle.node.type !== "dir") return { error: constants.WASI_ENOTDIR };
    const text = decoder.decode(new Uint8Array(this.memory.buffer, pointer, length));
    if (text.includes("\0")) return { error: constants.WASI_EINVAL };
    if (text.startsWith("/")) return { error: constants.WASI_ENOTCAPABLE };
    const guest =
      this.root === "/" ? handle.node.path : handle.node.path.slice(this.root.length) || "/";
    const path = this.storagePath(absolutePath(fd === 3 ? this.cwd : guest, text));
    return { path };
  }

  /** @param {bigint} rights */
  path_open(fd, dirflags, pointer, length, oflags, rights, inheriting, flags, result) {
    const target = this.pathAt(fd, pointer, length);
    if (target.error) return target.error;
    const path = target.path;
    let node = this.filesystem.nodes.get(path);
    if (!node) {
      if (!(oflags & constants.WASI_O_CREAT)) return constants.WASI_ENOENT;
      const parent = this.filesystem.nodes.get(parentPath(path));
      if (!parent) return constants.WASI_ENOENT;
      if (parent.type !== "dir") return constants.WASI_ENOTDIR;
      if (oflags & constants.WASI_O_DIRECTORY) return constants.WASI_ENOTDIR;
      node = this.makeNode(path, "file");
    } else if (oflags & constants.WASI_O_CREAT && oflags & constants.WASI_O_EXCL)
      return constants.WASI_EEXIST;
    if (oflags & constants.WASI_O_DIRECTORY && node.type !== "dir") return constants.WASI_ENOTDIR;
    if (oflags & constants.WASI_O_TRUNC) {
      if (node.type === "dir") return constants.WASI_EISDIR;
      if (!hasRight(rights, constants.WASI_RIGHT_FD_WRITE)) return constants.WASI_ENOTCAPABLE;
      node.buffers = [];
      node.size = 0;
    }
    let next = 4;
    while (this.fd[next]) next++;
    this.fd[next] = { node, seekPos: ZERO, flags, rights };
    this.getMemory().setUint32(result, next, true);
    return 0;
  }

  path_filestat_get(fd, flags, pointer, length, result) {
    const target = this.pathAt(fd, pointer, length);
    if (target.error) return target.error;
    const node = this.filesystem.nodes.get(target.path);
    return node ? this.writeStat(node, result) : constants.WASI_ENOENT;
  }

  path_create_directory(fd, pointer, length) {
    const target = this.pathAt(fd, pointer, length);
    if (target.error) return target.error;
    if (this.filesystem.nodes.has(target.path)) return constants.WASI_EEXIST;
    const parent = this.filesystem.nodes.get(parentPath(target.path));
    if (!parent) return constants.WASI_ENOENT;
    if (parent.type !== "dir") return constants.WASI_ENOTDIR;
    this.makeNode(target.path, "dir");
    return 0;
  }

  path_unlink_file(fd, pointer, length) {
    const target = this.pathAt(fd, pointer, length);
    if (target.error) return target.error;
    const node = this.filesystem.nodes.get(target.path);
    if (!node) return constants.WASI_ENOENT;
    if (node.type === "dir") return constants.WASI_EISDIR;
    this.filesystem.nodes.delete(target.path);
    return 0;
  }

  path_remove_directory(fd, pointer, length) {
    const target = this.pathAt(fd, pointer, length);
    if (target.error) return target.error;
    const node = this.filesystem.nodes.get(target.path);
    if (!node) return constants.WASI_ENOENT;
    if (node.type !== "dir") return constants.WASI_ENOTDIR;
    if ([...this.filesystem.nodes.keys()].some((path) => path.startsWith(target.path + "/")))
      return constants.WASI_ENOTEMPTY;
    if (target.path === this.root) return constants.WASI_ENOTCAPABLE;
    this.filesystem.nodes.delete(target.path);
    return 0;
  }

  path_rename(oldFd, oldPointer, oldLength, newFd, newPointer, newLength) {
    const from = this.pathAt(oldFd, oldPointer, oldLength);
    if (from.error) return from.error;
    const to = this.pathAt(newFd, newPointer, newLength);
    if (to.error) return to.error;
    const node = this.filesystem.nodes.get(from.path);
    if (!node) return constants.WASI_ENOENT;
    if (from.path === to.path) return 0;
    if (from.path === this.root || to.path === this.root) return constants.WASI_ENOTCAPABLE;
    const parent = this.filesystem.nodes.get(parentPath(to.path));
    if (!parent) return constants.WASI_ENOENT;
    if (parent.type !== "dir") return constants.WASI_ENOTDIR;
    const existing = this.filesystem.nodes.get(to.path);
    if (existing && existing.type !== node.type)
      return existing.type === "dir" ? constants.WASI_EISDIR : constants.WASI_ENOTDIR;
    if (node.type === "dir" && to.path.startsWith(from.path + "/")) return constants.WASI_EINVAL;
    if (
      existing?.type === "dir" &&
      [...this.filesystem.nodes.keys()].some((path) => path.startsWith(to.path + "/"))
    )
      return constants.WASI_ENOTEMPTY;
    const moving = [...this.filesystem.nodes].filter(
      ([path]) => path === from.path || path.startsWith(from.path + "/"),
    );
    for (const [path] of moving) this.filesystem.nodes.delete(path);
    for (const [path, entry] of moving) {
      entry.path = to.path + path.slice(from.path.length);
      this.filesystem.nodes.set(entry.path, entry);
    }
    return 0;
  }

  fd_readdir(fd, pointer, length, cookie, used) {
    const handle = this.fd[fd];
    if (!handle) return constants.WASI_EBADF;
    if (handle.node.type !== "dir") return constants.WASI_ENOTDIR;
    if (cookie < ZERO) return constants.WASI_EINVAL;
    const entries = [...this.filesystem.nodes.values()].filter(
      (node) => node.path !== handle.node.path && parentPath(node.path) === handle.node.path,
    );
    const bytes = new Uint8Array(this.memory.buffer, pointer, length);
    let written = 0;
    for (let i = Number(cookie); i < entries.length && written < length; i++) {
      const node = entries[i];
      const name = encoder.encode(node.path.slice(node.path.lastIndexOf("/") + 1));
      const record = new Uint8Array(24 + name.length);
      const view = new DataView(record.buffer);
      view.setBigUint64(0, BigInt(i + 1), true);
      view.setBigUint64(8, BigInt(node.inode), true);
      view.setUint32(16, name.length, true);
      view.setUint8(20, nodeType(node));
      record.set(name, 24);
      const amount = Math.min(record.length, length - written);
      bytes.set(record.subarray(0, amount), written);
      written += amount;
    }
    this.getMemory().setUint32(used, written, true);
    return 0;
  }
}
