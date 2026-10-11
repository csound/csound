import assert from "node:assert/strict";

let WASI, constants, previousGoog;
const encode = (text) => new TextEncoder().encode(text);
const decode = (bytes) => new TextDecoder().decode(bytes);
const memory = () => new WebAssembly.Memory({ initial: 1 });
function open(wasi, path, flags = 0, rights = constants.RIGHTS_REGULAR_FILE_BASE) {
  const bytes = encode(path);
  new Uint8Array(wasi.memory.buffer).set(bytes, 128);
  assert.equal(wasi.path_open(3, 0, 128, bytes.length, flags, rights, 0n, 0, 8), 0);
  return wasi.getMemory().getUint32(8, true);
}
function io(wasi, fd, text) {
  const bytes = encode(text);
  new Uint8Array(wasi.memory.buffer).set(bytes, 256);
  wasi.getMemory().setUint32(32, 256, true);
  wasi.getMemory().setUint32(36, bytes.length, true);
  assert.equal(wasi.fd_write(fd, 32, 1, 16), 0);
}

describe("shared WASI filesystem", () => {
  before(async () => {
    previousGoog = Object.getOwnPropertyDescriptor(globalThis, "goog");
    globalThis.goog = { global: globalThis, define: (_name, value) => value };
    ({ WASI } = await import("../../src/filesystem/wasi.js"));
    constants = await import("../../src/filesystem/constants.js");
  });
  after(() => {
    if (previousGoog) Object.defineProperty(globalThis, "goog", previousGoog);
    else delete globalThis.goog;
  });
  let host;
  beforeEach(() => {
    host = new WASI();
    host.setMemory(memory());
  });

  it("shares writes while preserving each parent's and child's open position", () => {
    host.writeFile("score", encode("abcdef"));
    const parentFd = open(host, "score");
    assert.equal(host.fd_seek(parentFd, 4n, 0, 16), 0);
    const child = host.createCommand(["tool"]);
    child.setMemory(memory());
    const childFd = open(child, "score");
    io(child, childFd, "XY");
    assert.equal(decode(host.readFile("score")), "XYcdef");
    assert.equal(host.fd[parentFd].seekPos, 4n);
    assert.equal(child.fd[childFd].seekPos, 2n);
    child.fd_close(childFd);
    assert.equal(host.fd_tell(parentFd, 16), 0);
    assert.equal(host.getMemory().getBigUint64(16, true), 4n);
  });

  it("overwrites a file header without inserting bytes or losing its tail", () => {
    host.writeFile("audio", encode("HEADsamples"));
    const fd = open(host, "audio");
    io(host, fd, "RIFF");
    assert.equal(decode(host.readFile("audio")), "RIFFsamples");
    assert.equal(host.fd_seek(fd, 14n, 0, 16), 0);
    io(host, fd, "end");
    assert.deepEqual(
      [...host.readFile("audio")],
      [...encode("RIFFsamples"), 0, 0, 0, ...encode("end")],
    );
  });

  it("reads across chunks and iovecs and leaves bytes after EOF untouched", () => {
    host.writeFile("input", encode("abc"));
    host.appendFile("input", encode("def"));
    const fd = open(host, "input");
    const view = host.getMemory();
    view.setUint32(32, 256, true);
    view.setUint32(36, 2, true);
    view.setUint32(40, 300, true);
    view.setUint32(44, 8, true);
    new Uint8Array(host.memory.buffer, 300, 8).fill(99);
    assert.equal(host.fd_read(fd, 32, 2, 16), 0);
    assert.equal(view.getUint32(16, true), 6);
    assert.equal(decode(new Uint8Array(host.memory.buffer, 256, 2)), "ab");
    assert.equal(decode(new Uint8Array(host.memory.buffer, 300, 8)), "cdefcccc");
    assert.equal(host.fd_read(fd, 32, 2, 16), 0);
    assert.equal(view.getUint32(16, true), 0);
    assert.equal(decode(new Uint8Array(host.memory.buffer, 256, 2)), "ab");
  });

  it("preserves open files after unlink and gives recreated paths a new node", () => {
    host.writeFile("score", encode("old"));
    const fd = open(host, "score");
    assert.equal(host.path_unlink_file(3, 128, 5), 0);
    host.writeFile("score", encode("new"));
    io(host, fd, "OLD");
    assert.equal(decode(host.readFile("score")), "new");
    assert.equal(decode(host.fd[fd].node.buffers[0]), "OLD");
  });

  it("does not expose files above the command's mounted directory", () => {
    host.writeFile("secret", encode("private"));
    host.mkdir("project");
    host.chdir("project");
    const child = host.createCommand(["tool"]);
    child.setMemory(memory());
    assert.equal(child.readFile("../secret"), undefined);
    assert.equal(child.readFile("/secret"), undefined);
    new Uint8Array(child.memory.buffer).set(encode("../secret"), 128);
    assert.notEqual(child.path_open(3, 0, 128, 9, 0, 2n, 0n, 0, 8), 0);
    child.writeFile("result", encode("score"));
    assert.equal(decode(host.readFile("result")), "score");
  });

  it("writes UTF-8 argv sizes, pointers and terminators after memory growth", () => {
    const child = host.createCommand(["tool", "entrée", ""]);
    child.setMemory(memory());
    child.getMemory();
    child.memory.grow(1);
    assert.equal(child.args_sizes_get(0, 4), 0);
    assert.equal(child.getMemory().getUint32(0, true), 3);
    assert.equal(child.getMemory().getUint32(4, true), 14);
    assert.equal(child.args_get(16, 65536), 0);
    assert.deepEqual(
      [16, 20, 24].map((p) => child.getMemory().getUint32(p, true)),
      [65536, 65541, 65549],
    );
    assert.equal(decode(new Uint8Array(child.memory.buffer, 65536, 14)), "tool\0entrée\0\0");
  });

  it("returns ENOSYS for unsupported imports without exposing JS helper methods", () => {
    for (const name of ["poll_oneoff", "writeFile"]) {
      const string = (value) => [value.length, ...encode(value)];
      const section = (id, bytes) => [id, bytes.length, ...bytes];
      const module = new WebAssembly.Module(
        new Uint8Array([
          0,
          97,
          115,
          109,
          1,
          0,
          0,
          0,
          ...section(1, [1, 0x60, 0, 1, 0x7f]),
          ...section(2, [1, ...string("wasi_snapshot_preview1"), ...string(name), 0, 0]),
          ...section(7, [1, ...string("call"), 0, 0]),
        ]),
      );
      const instance = new WebAssembly.Instance(module, host.getImports(module));
      assert.equal(instance.exports.call(), constants.WASI_ENOSYS);
    }
  });

  it("uses the WASI fdstat and filestat layouts", () => {
    host.writeFile("file", encode("abc"));
    const fd = open(host, "file");
    assert.equal(host.fd_fdstat_get(fd, 400), 0);
    const view = host.getMemory();
    assert.equal(view.getUint8(400), constants.WASI_FILETYPE_REGULAR_FILE);
    assert.equal(view.getBigUint64(408, true), constants.RIGHTS_REGULAR_FILE_BASE);
    assert.equal(host.fd_filestat_get(fd, 440), 0);
    assert.equal(view.getUint8(456), constants.WASI_FILETYPE_REGULAR_FILE);
    assert.equal(view.getBigUint64(464, true), 1n);
    assert.equal(view.getBigUint64(472, true), 3n);
  });

  it("truncates shared storage without resetting other open positions", () => {
    host.writeFile("file", encode("abcdef"));
    const first = open(host, "file");
    host.fd_seek(first, 5n, 0, 16);
    const second = open(host, "file", constants.WASI_O_TRUNC);
    assert.equal(host.readFile("file").length, 0);
    io(host, second, "x");
    assert.equal(host.fd[first].seekPos, 5n);
    assert.equal(host.fd_filestat_set_size(second, 3n), 0);
    assert.deepEqual([...host.readFile("file")], [120, 0, 0]);
  });

  it("keeps positioned writes from changing the file position", () => {
    host.writeFile("file", encode("abcdef"));
    const fd = open(host, "file");
    host.fd_seek(fd, 5n, 0, 16);
    new Uint8Array(host.memory.buffer).set(encode("XY"), 256);
    host.getMemory().setUint32(32, 256, true);
    host.getMemory().setUint32(36, 2, true);
    assert.equal(host.fd_pwrite(fd, 32, 1, 1n, 16), 0);
    assert.equal(decode(host.readFile("file")), "aXYdef");
    assert.equal(host.fd[fd].seekPos, 5n);
  });
});
