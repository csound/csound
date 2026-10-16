import assert from "node:assert/strict";
let createThreadedPluginRegistry;
let instantiateThreadedPlugins;

// Compiled from fixtures/threaded-plugin.wat with wasm-tools parse.
const compiled = Buffer.from(
  "AGFzbQEAAAABDANgAABgAX8AYAABfwJbBANlbnYGbWVtb3J5AgMBAgNlbnYZX19pbmRpcmVjdF9mdW5jdGlvbl90YWJsZQFwAAEDZW52DV9fbWVtb3J5X2Jhc2UDfwADZW52DF9fdGFibGVfYmFzZQN/AAMGBQAAAQICBhADfwFBAAt/AEEEC38AQQQLB1cFCl9fdGxzX3NpemUDAwtfX3Rsc19hbGlnbgMEEV9fd2FzbV9jYWxsX2N0b3JzAAEPX193YXNtX2luaXRfdGxzAAISY3NvdW5kX29wY29kZV9pbml0AAQIAQAJBwEAIwELAQMKVAUXACMAQQBBAf5IAgBFBEAjAEENNgIICwsPACMAIwAoAgRBAWo2AgQLDQAgACQCIABBETYCAAsUACMCIwIoAgBBAWo2AgAjAigCAAsHACMAKAIICwA4BG5hbWUBDgIABXN0YXJ0AwRzdGVwAgwBAgEAB3BvaW50ZXIHEwMABGJhc2UBBXRhYmxlAgN0bHM=",
  "base64",
);
const pluginBytes = (tableAlign = 0) =>
  new Uint8Array([
    ...compiled.subarray(0, 8),
    // dylink.0: 16 data bytes aligned to4, one table element.
    0,
    15,
    8,
    ...new TextEncoder().encode("dylink.0"),
    1,
    4,
    16,
    2,
    1,
    tableAlign,
    ...compiled.subarray(8),
  ]);

const setup = (memory = new WebAssembly.Memory({ initial: 1, maximum: 2, shared: true })) => {
  let next = 1024;
  const allocations = [];
  const freed = [];
  const registered = [];
  const table = new WebAssembly.Table({ initial: 4, element: "anyfunc" });
  const hostExports = {
    __indirect_function_table: table,
    allocByteMem: (size) => {
      const pointer = next;
      next += size;
      allocations.push(pointer);
      return pointer;
    },
    freeByteMem: (pointer) => freed.push(pointer),
    csoundWasiLoadOpcodeLibrary: (csound, fgen, opcode) => registered.push(Number(opcode.value)),
  };
  const importsForModule = () => ({ env: {} });
  const registry = createThreadedPluginRegistry({
    memory,
    hostExports,
    importsForModule,
    poolSize: 2,
    wasi: { readFile: () => pluginBytes() },
  });
  return { memory, table, hostExports, importsForModule, registry, allocations, freed, registered };
};

describe("shared-memory plugin linking", () => {
  before(async () => {
    const previous = Object.getOwnPropertyDescriptor(globalThis, "goog");
    globalThis.goog = { define: (_name, value) => value };
    try {
      ({ createThreadedPluginRegistry, instantiateThreadedPlugins } =
        await import("../../src/utils/threaded-plugins.js"));
    } finally {
      if (previous) Object.defineProperty(globalThis, "goog", previous);
      else delete globalThis.goog;
    }
  });
  it("mirrors function addresses without rerunning constructors or sharing TLS", () => {
    const parent = setup();
    parent.registry.addInitial(pluginBytes());
    assert.equal(parent.registry.loadModules(1), 0);
    const [plan] = parent.registry.snapshot();
    const view = new DataView(parent.memory.buffer);
    assert.equal(view.getInt32(plan.memoryBase + 4, true), 1);
    assert.equal(parent.table.get(plan.tableBase)(), 18);
    view.setInt32(plan.memoryBase + 8, 99, true);

    const child = setup(parent.memory);
    const { initializeTLS } = instantiateThreadedPlugins({
      plugins: parent.registry.snapshot(),
      memory: parent.memory,
      hostExports: child.hostExports,
      importsForModule: child.importsForModule,
      index: 0,
    });
    initializeTLS();
    assert.equal(view.getInt32(plan.memoryBase + 4, true), 1, "constructor ran once");
    assert.equal(child.table.get(parent.registered[0])(), 99, "shared data survived child startup");
    assert.equal(child.table.get(plan.tableBase)(), 18, "child has its own TLS");
    assert.equal(parent.table.get(plan.tableBase)(), 19, "parent TLS did not change");
    initializeTLS();
    assert.equal(child.table.get(plan.tableBase)(), 18, "new pthread gets fresh TLS");
  });

  it("reuses registration slots after reset while rejecting a changed layout", () => {
    const { registry, table, registered } = setup();
    const bytes = pluginBytes();
    registry.addInitial(bytes);
    registry.loadModules(1);
    const length = table.length;
    registry.freeze();
    registry.addInitial(bytes);
    registry.loadModules(1);
    assert.equal(registered[0], registered[1]);
    assert.equal(table.length, length);
    const different = new Uint8Array([...bytes, 0, 2, 1, 120]);
    assert.throws(() => registry.addInitial(different), /before the first threaded performance/);
  });

  it("clears failed table slots and frees data and TLS allocations", () => {
    const { registry, table, allocations, freed } = setup();
    const bad = pluginBytes();
    const ctor = Buffer.from(bad).indexOf(Buffer.from([15, 0, 0x23, 0, 0x23, 0, 0x28]));
    assert.ok(ctor > 0);
    bad[ctor + 2] = 0; // Trap in the constructor, after data and TLS are allocated.
    assert.ok(WebAssembly.validate(bad));
    assert.throws(() => registry.addInitial(bad), WebAssembly.RuntimeError);
    const length = table.length;
    assert.throws(() => registry.addInitial(bad), WebAssembly.RuntimeError);
    assert.equal(table.length, length);
    assert.deepEqual(freed, allocations);
    assert.equal(table.get(length - 1), null);
    assert.deepEqual(registry.snapshot(), []);
  });

  it("rejects unsafe table growth and missing Csound entries", () => {
    const { registry, table } = setup();
    assert.throws(() => registry.addInitial(pluginBytes(30)), /function table is too large/);
    assert.equal(table.length, 4);
    assert.throws(() => registry.addInitial(new Uint8Array()), /PIC modules/);
  });

  it("rejects TLS export names with the wrong types and releases the allocation", () => {
    const { registry, allocations, freed } = setup();
    const bad = pluginBytes();
    const kind = Buffer.from(bad).indexOf("__tls_size") + "__tls_size".length;
    bad[kind] = 0; // Export a function in place of the TLS size global.
    assert.ok(WebAssembly.validate(bad));
    assert.throws(() => registry.addInitial(bad), /Invalid threaded plugin TLS metadata/);
    assert.deepEqual(freed, allocations);
  });

  it("rejects a TLS offset imported as an ordinary data address", () => {
    const { registry, allocations } = setup();
    const name = [...new TextEncoder().encode("external_tls")];
    const flags = [1, name.length, ...name, 0x80, 2]; // WASM_SYM_TLS = 0x100.
    const exportInfo = [3, flags.length, ...flags];
    const original = pluginBytes();
    const metadata = [...original.subarray(0, 25), ...exportInfo, ...original.subarray(25)];
    metadata[9] += exportInfo.length;
    const imports = Buffer.from(metadata).indexOf(Buffer.from([2, 91, 4]));
    assert.ok(imports > 0);
    const addressImport = [
      7,
      ...new TextEncoder().encode("GOT.mem"),
      name.length,
      ...name,
      3,
      0x7f,
      1,
    ];
    const end = imports + 2 + metadata[imports + 1];
    const binary = new Uint8Array([
      ...metadata.slice(0, end),
      ...addressImport,
      ...metadata.slice(end),
    ]);
    binary[imports + 1] += addressImport.length;
    binary[imports + 2] += 1;
    assert.ok(WebAssembly.validate(binary));
    assert.throws(() => registry.addInitial(binary), /cross-module TLS addresses/);
    assert.deepEqual(allocations, []);
  });

  it("needs no reactor table when no plugins were loaded", () => {
    assert.doesNotThrow(() => instantiateThreadedPlugins({ plugins: [] }).initializeTLS());
  });
});
