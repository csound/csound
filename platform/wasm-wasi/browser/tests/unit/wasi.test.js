import assert from "node:assert/strict";
import { WASI } from "../../src/filesystem/preview2.js";
import { setupHost, importsFor } from "../preview2-host-fixture.js";
const encode = (s) => new TextEncoder().encode(s);
const decode = (b) => new TextDecoder().decode(b);
function open(f, path, flags = 0, access = 3) {
  f.host.bytes(128, encode(path).length).set(encode(path));
  f.call("open-at", f.root, 0, 128, encode(path).length, flags, access, 32);
  assert.equal(f.host.getMemory().getUint8(32), 0);
  return f.host.getMemory().getUint32(36, true);
}
function output(f, fd, position = 0n) {
  f.call("write-via-stream", fd, position, 32);
  assert.equal(f.host.getMemory().getUint8(32), 0);
  return f.host.getMemory().getUint32(36, true);
}
function write(f, stream, text) {
  const bytes = encode(text);
  f.host.bytes(256, bytes.length).set(bytes);
  f.host.writeStream(stream, 256, bytes.length, 32);
  assert.equal(f.host.getMemory().getUint8(32), 0);
}
describe("Preview 2 filesystem", () => {
  let f;
  beforeEach(() => {
    f = setupHost();
  });
  it("waits for a clock without the SharedArrayBuffer global", () => {
    const SharedBuffer = globalThis.SharedArrayBuffer;
    globalThis.SharedArrayBuffer = undefined;
    try {
      const deadline = performance.now() + 5;
      f.host.block(f.host.add("pollable", { deadline }));
      assert(performance.now() >= deadline);
    } finally {
      globalThis.SharedArrayBuffer = SharedBuffer;
    }
  });
  it("explains why a UI or AudioWorklet host cannot wait", () => {
    const wait = Atomics.wait;
    Atomics.wait = () => {
      throw new TypeError("Atomics.wait cannot be called in this context");
    };
    try {
      assert.throws(
        () => f.host.block(f.host.add("pollable", { deadline: performance.now() + 100 })),
        /require a worker/,
      );
    } finally {
      Atomics.wait = wait;
    }
  });
  it("opens UTF-8 paths and allocates large random lists across memory growth", () => {
    f.host.writeFile("音", encode("audio"));
    assert(open(f, "音"));
    const random = importsFor(f.host, "wasi:random/random@0.2.0", ["get-random-bytes"]);
    random["get-random-bytes"](140000n, 32);
    const view = f.host.getMemory(),
      data = view.getUint32(32, true);
    assert.equal(view.getUint32(36, true), 140000);
    assert(f.host.bytes(data + 65536, 70000).some((v) => v !== 0));
  });
  it("shares writes while keeping independent stream positions", () => {
    f.host.writeFile("score", encode("abcdef"));
    const parent = output(f, open(f, "score"), 4n);
    const child = setupHost(f.host.createCommand(["tool"]));
    const childStream = output(child, open(child, "score"));
    write(child, childStream, "XY");
    assert.equal(decode(f.host.readFile("score")), "XYcdef");
    write(f, parent, "Z");
    assert.equal(decode(f.host.readFile("score")), "XYcdZf");
  });
  it("overwrites headers and zero-fills gaps without losing the tail", () => {
    f.host.writeFile("audio", encode("HEADsamples"));
    const fd = open(f, "audio");
    write(f, output(f, fd), "RIFF");
    assert.equal(decode(f.host.readFile("audio")), "RIFFsamples");
    write(f, output(f, fd, 14n), "end");
    assert.deepEqual(
      [...f.host.readFile("audio")],
      [...encode("RIFFsamples"), 0, 0, 0, ...encode("end")],
    );
  });
  it("reads across storage chunks and reports a closed stream after EOF", () => {
    f.host.writeFile("input", encode("abc"));
    f.host.appendFile("input", encode("def"));
    f.call("read-via-stream", open(f, "input"), 0n, 32);
    const stream = f.host.getMemory().getUint32(36, true);
    f.host.readStream(stream, 20n, 32);
    const view = f.host.getMemory();
    assert.equal(view.getUint8(32), 0);
    assert.equal(
      decode(f.host.bytes(view.getUint32(36, true), view.getUint32(40, true))),
      "abcdef",
    );
    f.host.readStream(stream, 1n, 32);
    assert.equal(view.getUint8(32), 1);
    assert.equal(view.getUint8(36), 1);
  });
  it("does not extend a file on an empty write beyond EOF", () => {
    f.host.writeFile("audio", encode("head"));
    write(f, output(f, open(f, "audio"), 100n), "");
    assert.equal(decode(f.host.readFile("audio")), "head");
  });
  it("shares the supplied entropy source with child commands", () => {
    let next = 1;
    const parent = new WASI({ getRandomValues: (bytes) => bytes.fill(next++) });
    for (const host of [parent, parent.createCommand(["tool"])]) {
      setupHost(host);
      importsFor(host, "wasi:random/random@0.2.0", ["get-random-bytes"])["get-random-bytes"](
        8n,
        32,
      );
      const ptr = host.getMemory().getUint32(32, true);
      assert.equal(host.bytes(ptr, 8)[0], next - 1);
    }
    assert.equal(next, 3);
  });
  it("preserves open files after unlink and gives recreated paths new storage", () => {
    f.host.writeFile("score", encode("old"));
    const fd = open(f, "score"),
      stream = output(f, fd);
    f.call("unlink-file-at", f.root, 128, 5, 32);
    assert.equal(f.host.getMemory().getUint8(32), 0);
    f.host.writeFile("score", encode("new"));
    write(f, stream, "OLD");
    assert.equal(decode(f.host.readFile("score")), "new");
    assert.equal(decode(f.host.resource(stream, "output-stream").node.buffers[0]), "OLD");
  });
  it("rejects paths above a command's mount and writes within its root", () => {
    f.host.writeFile("secret", encode("private"));
    f.host.mkdir("project");
    f.host.chdir("project");
    const child = setupHost(f.host.createCommand(["tool"]));
    assert.equal(child.host.readFile("../secret"), undefined);
    assert.equal(child.host.readFile("/secret"), undefined);
    child.host.bytes(128, 9).set(encode("../secret"));
    child.call("open-at", child.root, 0, 128, 9, 0, 1, 32);
    assert.equal(child.host.getMemory().getUint8(32), 1);
    child.host.writeFile("result", encode("score"));
    assert.equal(decode(f.host.readFile("result")), "score");
  });
  it("uses canonical string lists after memory growth", () => {
    const child = setupHost(f.host.createCommand(["tool", "entrée", ""]));
    child.host.memory.grow(1);
    importsFor(child.host, "wasi:cli/environment@0.2.0", ["get-arguments"])["get-arguments"](32);
    const view = child.host.getMemory(),
      pointer = view.getUint32(32, true);
    assert.equal(view.getUint32(36, true), 3);
    assert.deepEqual(
      [0, 1, 2].map((i) =>
        decode(
          child.host.bytes(
            view.getUint32(pointer + i * 8, true),
            view.getUint32(pointer + i * 8 + 4, true),
          ),
        ),
      ),
      ["tool", "entrée", ""],
    );
  });
  it("rejects Preview 1 and unknown imports instead of exposing JS methods", () => {
    assert.throws(() => importsFor(f.host, "wasi_snapshot_preview1", ["proc_exit"]), /Preview 1/);
    assert.throws(
      () => importsFor(f.host, "wasi:filesystem/types@0.2.0", ["writeFile"]),
      /Unsupported/,
    );
    assert.throws(() => f.host.drop(999, "descriptor"), /Invalid/);
  });
  it("supports old and current Preview 2 command bindings", () => {
    const exports = f.host.instance.exports;
    for (const version of ["0.2.0", "0.2.12"]) {
      const cli = importsFor(f.host, `wasi:cli/exit@${version}`, ["exit"]);
      for (const status of [0, 1]) {
        const instance = {
          exports: {
            ...exports,
            [`wasi:cli/run@${version}#run`]: () => cli.exit(status),
          },
        };
        assert.equal(f.host.startCommand(instance), status);
      }
    }
  });
  it("preserves the exit code from newer Preview 2 commands", () => {
    const cli = importsFor(f.host, "wasi:cli/exit@0.2.12", ["exit-with-code"]);
    assert.equal(
      f.host.startCommand({
        exports: {
          ...f.host.instance.exports,
          "wasi:cli/run@0.2.12#run": () => cli["exit-with-code"](42),
        },
      }),
      42,
    );
  });
  it("rejects future WASI versions for imports and commands", () => {
    for (const version of ["0.2.13", "0.3.0", "0.2.12-extra"]) {
      assert.throws(
        () => importsFor(f.host, `wasi:cli/exit@${version}`, ["exit"]),
        /Unsupported Preview 2 import/,
      );
      assert.throws(
        () =>
          f.host.startCommand({
            exports: {
              ...f.host.instance.exports,
              [`wasi:cli/run@${version}#run`]: () => 0,
            },
          }),
        /Expected a WASI Preview 2 command/,
      );
    }
  });
  it("returns canonical descriptor metadata and respects read-only handles", () => {
    f.host.writeFile("file", encode("abc"));
    const fd = open(f, "file", 0, 1);
    f.call("get-flags", fd, 32);
    assert.equal(f.host.getMemory().getUint8(33), 1);
    f.call("stat", fd, 400);
    const view = f.host.getMemory();
    assert.equal(view.getUint8(400), 0);
    assert.equal(view.getUint8(408), 6);
    assert.equal(view.getBigUint64(416, true), 1n);
    assert.equal(view.getBigUint64(424, true), 3n);
    f.call("write-via-stream", fd, 0n, 32);
    assert.equal(view.getUint8(32), 1);
  });
  it("truncates shared storage without resetting existing streams", () => {
    f.host.writeFile("file", encode("abcdef"));
    const stream = output(f, open(f, "file"), 5n);
    const second = open(f, "file", 8);
    assert.equal(f.host.readFile("file").length, 0);
    f.call("set-size", second, 3n, 32);
    assert.deepEqual([...f.host.readFile("file")], [0, 0, 0]);
    write(f, stream, "x");
    assert.deepEqual([...f.host.readFile("file")], [0, 0, 0, 0, 0, 120]);
  });
  it("denies writable opens through a read-only directory before changing files", () => {
    f.host.writeFile("file", encode("keep"));
    const directory = open(f, ".", 2, 1);
    for (const [path, openFlags, flags] of [
      ["file", 0, 2],
      ["file", 8, 2],
      ["file", 1, 1],
      ["new", 1, 2],
      [".", 2, 33],
    ]) {
      f.host.bytes(128, encode(path).length).set(encode(path));
      f.call("open-at", directory, 0, 128, encode(path).length, openFlags, flags, 32);
      assert.equal(f.host.getMemory().getUint8(32), 1);
      assert.equal(f.host.getMemory().getUint8(36), 33); // read-only
      assert.equal(decode(f.host.readFile("file")), "keep");
      assert.deepEqual(f.host.readdir("/"), ["file"]);
    }
    assert(open({ ...f, root: directory }, "file", 0, 1));
    f.call("unlink-file-at", directory, 128, 4, 32);
    assert.equal(f.host.getMemory().getUint8(32), 1);
    assert.equal(f.host.getMemory().getUint8(33), 33);
    assert.equal(decode(f.host.readFile("file")), "keep");
  });
  it("keeps positioned writes from changing stream positions", () => {
    f.host.writeFile("file", encode("abcdef"));
    const fd = open(f, "file"),
      stream = output(f, fd, 5n);
    f.host.bytes(256, 2).set(encode("XY"));
    f.call("write", fd, 256, 2, 1n, 32);
    write(f, stream, "Z");
    assert.equal(decode(f.host.readFile("file")), "aXYdeZ");
  });
  it("does not create a file when truncation lacks write access", () => {
    f.host.bytes(128, 3).set(encode("new"));
    f.call("open-at", f.root, 0, 128, 3, 9, 1, 32);
    assert.equal(f.host.getMemory().getUint8(32), 1);
    assert.equal(f.host.getMemory().getUint8(36), 0); // access
    assert.deepEqual(f.host.readdir("/"), []);
  });
  it("checks path components before resolving dot segments", () => {
    f.host.writeFile("file", encode("keep"));
    f.host.mkdir("directory");
    for (const [path, error] of [
      ["file/", 24],
      ["file/.", 24],
      ["file/../file", 24],
      ["missing/../file", 20],
      ["", 20],
    ]) {
      f.host.bytes(128, encode(path).length).set(encode(path));
      f.call("open-at", f.root, 0, 128, encode(path).length, 0, 1, 32);
      assert.equal(f.host.getMemory().getUint8(32), 1, path);
      assert.equal(f.host.getMemory().getUint8(36), error, path);
    }
    assert(open(f, "directory/../file", 0, 1));
    assert(open(f, "directory//./", 2, 1));
    const create = importsFor(f.host, "wasi:filesystem/types@0.2.0", [
      "[method]descriptor.create-directory-at",
    ]);
    f.host.bytes(128, 5).set(encode("new//"));
    create["[method]descriptor.create-directory-at"](f.root, 128, 5, 32);
    assert.equal(f.host.getMemory().getUint8(32), 0);
    assert(open(f, "new/", 2, 1));
  });
  it("keeps removed directory handles separate from a replacement at the same path", () => {
    const methods = importsFor(f.host, "wasi:filesystem/types@0.2.0", [
      "[method]descriptor.remove-directory-at",
      "[method]descriptor.read-directory",
      "[method]descriptor.rename-at",
    ]);
    f.host.mkdir("old");
    const directory = open(f, "old", 2, 35);
    methods["[method]descriptor.remove-directory-at"](f.root, 128, 3, 32);
    assert.equal(f.host.getMemory().getUint8(32), 0);
    f.host.writeFile("old/new", encode("keep"));
    f.host.bytes(128, 3).set(encode("new"));
    f.call("open-at", directory, 0, 128, 3, 8, 3, 32);
    assert.equal(f.host.getMemory().getUint8(32), 1);
    assert.equal(f.host.getMemory().getUint8(36), 20);
    methods["[method]descriptor.read-directory"](directory, 32);
    assert.equal(f.host.getMemory().getUint8(32), 1);
    assert.equal(f.host.getMemory().getUint8(36), 20);
    assert.equal(decode(f.host.readFile("old/new")), "keep");
    // A live directory handle follows a rename.
    const live = open(f, "old", 2, 35);
    f.host.bytes(256, 5).set(encode("moved"));
    methods["[method]descriptor.rename-at"](f.root, 128, 3, f.root, 256, 5, 32);
    assert.equal(f.host.getMemory().getUint8(32), 0);
    assert(open({ ...f, root: live }, "new", 0, 1));
  });
});
