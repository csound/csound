import assert from "node:assert/strict";
import fs from "node:fs";
import { pathToFileURL } from "node:url";
const { WASI } = await import(pathToFileURL(process.argv[4] + "/filesystem/preview3.js"));
const core = new WebAssembly.Module(fs.readFileSync(process.argv[2]));
const sections = WebAssembly.Module.customSections(core, "csound.wasi-bridge");
const module = sections.length ? new WebAssembly.Module(sections[0]) : core;
const memory = new WebAssembly.Memory({ address: "i64", initial: 65538n, maximum: 65540n });
const host = new WASI({ args: ["bridge-test"] });
const imports = host.getImports(module);
imports.env = { ...imports.env, memory };
const bridge = new WebAssembly.Instance(module, imports);
host.setInstance(bridge);
let heap = 4096n;
const allocations = new Set();
const engine = new WebAssembly.Instance(new WebAssembly.Module(fs.readFileSync(process.argv[3])), {
  test: {
    allocate(old, size, alignment, length) {
      assert.equal(old, 0n);
      assert.equal(size, 0n);
      heap = (heap + alignment - 1n) & -alignment;
      const pointer = heap;
      heap += length;
      if (length) allocations.add(pointer);
      return pointer;
    },
    free(pointer) {
      assert(allocations.delete(pointer), "adapter frees each temporary allocation once");
    },
  },
});
const functions = bridge.exports["engine-functions"];
functions.set(0, engine.exports.cabi_realloc);
functions.set(2, engine.exports.free);
functions.set(3, engine.exports.__wasm_task_hook);
const call = (module, name, ...args) => bridge.exports[module + "#" + name](...args);
const filesystem = "wasi:filesystem/types@0.3.2";
const stdin = "wasi:cli/stdin@0.3.2";
const bytes = new Uint8Array(memory.buffer, 0x100001000, 1024);
const pointer = 0x100001000n;
const intrinsic = (operation, ...args) =>
  call(
    filesystem,
    (operation === "read" || operation === "write" ? "[async-lower]" : "") +
      `[stream-${operation}-0][method]descriptor.read-via-stream`,
    ...args,
  );
const pair = () => {
  const packed = call(stdin, "[stream-new-0]read-via-stream");
  return { reader: Number(packed & 0xffffffffn), writer: Number(packed >> 32n) };
};
const set = call("$root", "[waitable-set-new]");
function poll(reader) {
  call("$root", "[waitable-join]", reader, set);
  const event = call("$root", "[waitable-set-poll]", set, pointer + 128n);
  call("$root", "[waitable-join]", reader, 0);
  const view = new DataView(memory.buffer, Number(pointer + 128n), 8);
  return [event, view.getUint32(0, true), view.getUint32(4, true)];
}
function close({ reader, writer }) {
  intrinsic("drop-readable", reader);
  intrinsic("drop-writable", writer);
}
// A later import must neither overwrite nor release a blocked read's storage.
let endpoints = pair();
bytes.fill(0xaa);
assert.equal(intrinsic("read", endpoints.reader, pointer, 3n) >>> 0, 0xffffffff);
call("wasi:cli/environment@0.3.2", "get-arguments", pointer + 256n);
// Guest owns returned arguments; clear those allocations before checking leaks.
allocations.clear();
bytes.set([4, 5, 6], 32);
assert.equal(intrinsic("write", endpoints.writer, pointer + 32n, 3n), 48);
assert.deepEqual(poll(endpoints.reader), [2, endpoints.reader, 48]);
assert.deepEqual([...bytes.slice(0, 4)], [4, 5, 6, 0xaa]);
close(endpoints);
// Cancellation can race with a partial transfer. Copy only the count returned.
endpoints = pair();
bytes.fill(0xaa);
assert.equal(intrinsic("read", endpoints.reader, pointer, 3n) >>> 0, 0xffffffff);
bytes[32] = 7;
assert.equal(intrinsic("write", endpoints.writer, pointer + 32n, 1n), 16);
assert.equal(intrinsic("cancel-read", endpoints.reader), 16);
assert.deepEqual([...bytes.slice(0, 3)], [7, 0xaa, 0xaa]);
close(endpoints);
// Cancelling before any transfer preserves the guest buffer and releases it.
endpoints = pair();
assert.equal(intrinsic("read", endpoints.reader, pointer, 3n) >>> 0, 0xffffffff);
assert.equal(intrinsic("cancel-read", endpoints.reader), 2);
close(endpoints);
// The pending writer retains its buffer until the reader reaches the file sink.
host.writeFile("output", new Uint8Array());
const descriptor = host.add("descriptor", { node: host.findEntry("output"), flags: 3 });
endpoints = pair();
bytes.set([8, 9, 10], 32);
assert.equal(intrinsic("write", endpoints.writer, pointer + 32n, 3n) >>> 0, 0xffffffff);
const future = call(
  filesystem,
  "[method]descriptor.write-via-stream",
  descriptor,
  endpoints.reader,
  0n,
);
assert.deepEqual(poll(endpoints.writer), [3, endpoints.writer, 48]);
assert.deepEqual([...host.readFile("output")], [8, 9, 10]);
intrinsic("drop-writable", endpoints.writer);
call(filesystem, "[future-drop-readable-1][method]descriptor.read-via-stream", future);
call(filesystem, "[resource-drop]descriptor", descriptor);
call("$root", "[waitable-set-drop]", set);
assert.equal(allocations.size, 0, "temporary argument records must not leak in memory64");
const size = bridge.exports.memory.buffer.byteLength;
for (let index = 0; index < 5000; index++) {
  endpoints = pair();
  assert.equal(intrinsic("read", endpoints.reader, pointer, 3n) >>> 0, 0xffffffff);
  assert.equal(intrinsic("cancel-read", endpoints.reader), 2);
  close(endpoints);
}
assert.equal(bridge.exports.memory.buffer.byteLength, size, "completed arenas must be reused");
console.log("Preview 3 memory64 delayed transfers, cancellation and allocation ownership passed");
