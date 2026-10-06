import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { exitCommand, swapCommand } from "../score-command-fixtures.js";

let WASI, createSystem, commandArguments, previousGoog;
const encode = (text) => new TextEncoder().encode(text);
const decode = (bytes) => new TextDecoder().decode(bytes);
const csbeats = readFileSync(new URL("csbeats.wasm", import.meta.resolve("@csound/wasm-bin")));

function setup() {
  const host = new WASI({});
  host.setMemory(new WebAssembly.Memory({ initial: 1 }));
  const logs = [];
  const system = createSystem(host, (message) => logs.push(message));
  return {
    host,
    logs,
    run(command) {
      new Uint8Array(host.memory.buffer).set(encode(command + "\0"), 16);
      return system(16);
    },
  };
}

describe("WASI filesystem commands", () => {
  before(async () => {
    previousGoog = Object.getOwnPropertyDescriptor(globalThis, "goog");
    globalThis.goog = {
      define: (_name, value) => value,
      global: globalThis,
      require: () => ({ join: (a, b) => `${a}/${b}` }),
    };
    ({ WASI } = await import("../../src/filesystem/wasi.js"));
    ({ createSystem, commandArguments } = await import("../../src/filesystem/system.js"));
  });
  after(() => {
    if (previousGoog) Object.defineProperty(globalThis, "goog", previousGoog);
    else delete globalThis.goog;
  });

  it("parses quoted and escaped arguments without running a shell", () => {
    assert.deepEqual(commandArguments("'my tool' one\\ two ''"), ["my tool", "one two", ""]);
    for (const text of ["", "tool | other", "tool; other", "tool\nother", "'tool", "tool\\"]) {
      assert.throws(() => commandArguments(text));
    }
  });

  it("runs uploaded csbeats under another name, handles UTF-8 paths, and truncates output", () => {
    const { host, run } = setup();
    host.mkdir("/project");
    host.chdir("/project");
    host.writeFile("uploaded.wasm", csbeats);
    host.writeFile("entrée.ext", encode("i1 m1 b1 C4 q mf\n"));
    host.writeFile("result.sco", encode("old data".repeat(100)));
    assert.equal(run("uploaded entrée.ext result.sco"), 0);
    assert.match(decode(host.readFile("result.sco")), /i1 0\.000000 1\.000000 261\.625565 -3/);
    assert.ok(!decode(host.readFile("result.sco")).includes("old data"));
    host.writeFile("entrée.ext", encode("i1 m1 b1 D4 q mf\n"));
    assert.equal(run("uploaded.wasm entrée.ext result.sco"), 0);
    assert.match(decode(host.readFile("result.sco")), /293\.664768/);
  });

  it("encodes proc_exit status and invalidates the cache after a file replacement", () => {
    const { host, run } = setup();
    host.writeFile("custom.wasm", exitCommand(7));
    assert.equal(run("custom"), 7 << 8);
    host.writeFile("custom.wasm", exitCommand(0));
    assert.equal(run("custom"), 0);
    host.writeFile("custom", exitCommand(3));
    assert.equal(run("custom"), 3 << 8, "exact path wins over .wasm suffix");
  });

  it("reports missing commands and malformed modules as failures", () => {
    const { host, run, logs } = setup();
    assert.equal(run("missing"), -1);
    assert.match(logs.at(-1), /command not found/);
    host.writeFile("bad.wasm", encode("not a WASM file"));
    assert.equal(run("bad"), -1);
    assert.ok(!host.pathExists("result.sco"));
  });

  it("does not read or replace unrelated project files", () => {
    const { host, run } = setup();
    host.writeFile("custom.wasm", exitCommand(0));
    host.writeFile("large.wav", new Uint8Array(1024));
    const sample = host.findEntry("/large.wav");
    new Uint8Array(host.memory.buffer).set(encode("large.wav"), 256);
    assert.equal(host.path_open(3, 0, 256, 9, 0, 2n, 0n, 0, 8), 0);
    const fd = host.getMemory().getUint32(8, true);
    assert.equal(host.fd_seek(fd, 17n, 0, 8), 0);
    const readFile = host.readFile.bind(host);
    host.readFile = (path) => {
      assert.notEqual(path, "/large.wav", "the command never opens this file");
      return readFile(path);
    };
    assert.equal(run("custom"), 0);
    assert.equal(host.findEntry("/large.wav"), sample);
    assert.equal(host.fd[fd].seekPos, 17n);
  });

  it("preserves both files when an uploaded command swaps their paths", () => {
    const { host, run } = setup();
    host.writeFile("swap.wasm", swapCommand());
    host.writeFile("a", encode("first"));
    host.writeFile("b", encode("second"));
    assert.equal(run("swap"), 0);
    assert.equal(decode(host.readFile("a")), "second");
    assert.equal(decode(host.readFile("b")), "first");
    assert.equal(host.pathExists("temp"), false);
  });

  it("unlinks temporary files through the real WASI callback", () => {
    const { host } = setup();
    host.writeFile("/temporary", encode("score"));
    new Uint8Array(host.memory.buffer).set(encode("temporary"), 16);
    assert.equal(host.path_unlink_file(3, 16, 9), 0);
    assert.equal(host.pathExists("/temporary"), false);
    assert.notEqual(host.path_unlink_file(3, 16, 9), 0);
  });
});
