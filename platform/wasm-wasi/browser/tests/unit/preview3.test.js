import assert from "node:assert/strict";
import { WASI } from "../../src/filesystem/preview3.js";
import { importsFor } from "../preview2-host-fixture.js";
const encode = (text) => new TextEncoder().encode(text);
function setup() {
  const host = new WASI({ args: ["test"] });
  const memory = new WebAssembly.Memory({ initial: 2 });
  let heap = 4096;
  host.setInstance({
    exports: {
      memory,
      cabi_realloc: (_old, _size, alignment, length) => {
        heap = Math.ceil(heap / alignment) * alignment;
        const pointer = heap;
        heap += length;
        return pointer;
      },
    },
  });
  const module = "wasi:filesystem/types@0.3.2";
  const root = host.add("descriptor", { node: host.findEntry("/"), flags: 35 });
  const call = (name, ...args) => importsFor(host, module, [name])[name](...args);
  return { host, root, call };
}
describe("Preview 3 core host", () => {
  it("denies create and truncate through read-only directories before mutation", () => {
    const { host, root, call } = setup();
    host.descriptor(root).flags = 1;
    host.writeFile("audio", encode("unchanged"));
    for (const [path, flags] of [
      ["new", 1],
      ["audio", 8],
    ]) {
      host.bytes(128, path.length).set(encode(path));
      assert.equal(
        call("[async-lower][method]descriptor.open-at", root, 0, 128, path.length, flags, 3, 32),
        2,
      );
      assert.equal(host.getMemory().getUint8(32), 1);
      assert.equal(host.getMemory().getUint8(36), 32);
    }
    assert(!host.pathExists("new"));
    assert.equal(new TextDecoder().decode(host.readFile("audio")), "unchanged");
  });
  it("writes asynchronous scalar results to the caller's result area", () => {
    const { host, root, call } = setup();
    assert.equal(call("[async-lower][method]descriptor.is-same-object", root, root, 32), 2);
    assert.equal(host.getMemory().getUint8(32), 1);
  });
  it("keeps stream and future destructors distinct", () => {
    const { host } = setup();
    const cli = importsFor(host, "wasi:cli/stdin@0.3.2", [
      "read-via-stream",
      "[stream-drop-readable-0]read-via-stream",
      "[future-drop-readable-1]read-via-stream",
    ]);
    cli["read-via-stream"](32);
    const stream = host.getMemory().getUint32(32, true);
    const future = host.getMemory().getUint32(36, true);
    assert.throws(() => cli["[stream-drop-readable-0]read-via-stream"](future), /Invalid WASI/);
    cli["[stream-drop-readable-0]read-via-stream"](stream);
    cli["[future-drop-readable-1]read-via-stream"](future);
  });
  it("returns filesystem errors before the caller drops a failed writer", () => {
    for (const offset of [0x7fffffffn, 0x80000000n]) {
      const { host } = setup();
      host.writeFile("file", encode("original"));
      const descriptor = host.add("descriptor", { node: host.findEntry("file"), flags: 3 });
      const { reader, writer } = host.pair("bytes");
      const future = host.writeViaStream(descriptor, reader, offset);
      assert.equal(host.transfer("bytes", "write", writer, 128, 1), 1);
      assert.equal(host.transfer("fs-result", "read", future, 32), 0);
      assert.equal(host.getMemory().getUint8(32), 1);
      assert.equal(host.getMemory().getUint8(36), 7);
      assert.equal(new TextDecoder().decode(host.readFile("file")), "original");
      host.dropEndpoint("bytes", "write", writer);
      host.dropEndpoint("fs-result", "read", future);
    }
  });
  it("returns an empty directory stream and error future for a non-directory", () => {
    const { host, call } = setup();
    host.writeFile("file", encode("x"));
    const descriptor = host.add("descriptor", { node: host.findEntry("file"), flags: 1 });
    call("[method]descriptor.read-directory", descriptor, 32);
    const stream = host.getMemory().getUint32(32, true);
    const future = host.getMemory().getUint32(36, true);
    assert.equal(host.transfer("directory", "read", stream, 128, 1), 1);
    assert.equal(host.transfer("fs-result", "read", future, 64), 0);
    assert.equal(host.getMemory().getUint8(68), 23);
    host.dropEndpoint("directory", "read", stream);
  });
  it("delivers a timer's completion once", () => {
    const { host } = setup();
    const root = host.rootImports();
    const task = host.add("subtask", { deadline: 0 });
    const set = root["[waitable-set-new]"]();
    root["[waitable-join]"](task, set);
    assert.equal(root["[waitable-set-poll]"](set, 32), 1);
    assert.equal(root["[waitable-set-poll]"](set, 32), 0);
    host.block = () => {
      throw new Error("A delivered timer must not schedule another wait");
    };
    assert.throws(() => root["[waitable-set-wait]"](set, 32), /No synchronous host event/);
    root["[subtask-drop]"](task);
    root["[waitable-set-drop]"](set);
  });
  it("returns a started timer's cancellation exactly once", () => {
    const { host } = setup();
    const root = host.rootImports();
    const status = host.clockTask(Infinity);
    assert.equal(status & 15, 1);
    const task = status >>> 4;
    assert.equal(root["[subtask-cancel]"](task), 4);
    assert.throws(() => root["[subtask-cancel]"](task), /Cannot cancel/);
    const set = root["[waitable-set-new]"]();
    root["[waitable-join]"](task, set);
    assert.equal(root["[waitable-set-poll]"](set, 32), 0);
    root["[subtask-drop]"](task);
    root["[waitable-set-drop]"](set);
  });
  it("keeps a completed stream transfer pending until its event is read", () => {
    const { host } = setup();
    const root = host.rootImports();
    const { reader, writer } = host.pair("bytes");
    const set = root["[waitable-set-new]"]();
    root["[waitable-join]"](reader, set);
    assert.equal(host.transfer("bytes", "read", reader, 128, 1), 0xffffffff);
    host.u8(256, 42);
    assert.equal(host.transfer("bytes", "write", writer, 256, 1), 16);
    assert.throws(() => host.transfer("bytes", "read", reader, 128, 1), /already pending/);
    assert.equal(root["[waitable-set-poll]"](set, 32), 2);
    assert.equal(host.getMemory().getUint8(128), 42);
    host.dropEndpoint("bytes", "read", reader);
    assert.equal(root["[waitable-set-poll]"](set, 32), 0);
    host.dropEndpoint("bytes", "write", writer);
    root["[waitable-set-drop]"](set);
  });
  it("restores the task's stack after an exception before releasing it", () => {
    const { host } = setup();
    host.instance.exports.__wasm_task_hook = (code) => {
      if (code === 0) host.setStackPointer(8192);
      else {
        assert.equal(host.getStackPointer(), 8192);
        host.setStackPointer(0);
      }
    };
    const error = new Error("host call failed");
    assert.throws(
      () =>
        host.task(() => {
          host.setStackPointer(8000);
          throw error;
        }),
      (caught) => caught === error,
    );
    assert.equal(
      host.task(() => 42),
      42,
    );
    assert.equal(host.getStackPointer(), 0);
  });
});
