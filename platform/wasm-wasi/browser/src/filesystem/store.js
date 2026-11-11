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

import { encoder, decoder } from "../utils/utf8.js";

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

export function sliceBuffers(buffers, start, length) {
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

export function concatenate(buffers, size) {
  const result = new Uint8Array(size);
  let offset = 0;
  for (const buffer of buffers) {
    result.set(buffer, offset);
    offset += buffer.length;
  }
  return result;
}

export function writeBytes(node, offset, bytes) {
  if (bytes.length === 0) return;
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

/** @unrestricted */
export class MemoryFilesystem {
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
    // Keep standard streams in the file store when resource handles close.
    this.fd = [0, 1, 2].map((fd) => ({
      node: { type: "stream", buffers: [], size: 0, inode: fd },
    }));
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
    if (!entry) return 44;
    if (entry.type !== "dir") return 54;
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
}
