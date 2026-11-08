import assert from "node:assert/strict";
import { preparePluginImports } from "../../src/utils/plugin-imports.js";

describe("WebAssembly plugin imports", () => {
  const prepare = (overrides = {}) => {
    const options = overrides.options || { env: {} };
    const link = preparePluginImports({
      imports: [],
      exports: [],
      hostExports: {},
      memoryBase: 1024,
      ...overrides,
      options,
    });
    return { options, link };
  };

  it("binds weak definitions to the host or the plugin's own export", () => {
    const { options, link } = prepare({
      imports: ["host", "local"].map((name) => ({ module: "env", name, kind: "function" })),
      exports: ["host", "local"].map((name) => ({ name, kind: "function" })),
      hostExports: { host: () => 12 },
      importedSymbols: ["host", "local"].map((name) => ({ module: "env", name, flags: 1 })),
    });
    link({ host: () => -1, local: () => 42 });
    assert.equal(options.env.host(), 12);
    assert.equal(options.env.local(), 42);
  });

  it("keeps strong static runtime definitions for direct calls and function pointers", () => {
    const names = [
      "longjmp",
      "__wasm_longjmp",
      "saveSetjmp",
      "testSetjmp",
      "getTempRet0",
      "setTempRet0",
    ];
    const localFunctions = Object.fromEntries(names.map((name) => [name, () => name]));
    const { options, link } = prepare({
      imports: names.flatMap((name) => [
        { module: "env", name, kind: "function" },
        { module: "GOT.func", name, kind: "global" },
      ]),
      exports: names.map((name) => ({ name, kind: "function" })),
      hostExports: Object.fromEntries(
        names.map((name) => [name, () => assert.fail("host runtime used")]),
      ),
    });
    const functions = [];
    link(localFunctions, (func) => {
      functions.push(func);
      return functions.length;
    });
    names.forEach((name, index) => {
      assert.equal(options.env[name](), name);
      assert.equal(functions[index], localFunctions[name]);
      assert.equal(options["GOT.func"][name].value, index + 1);
    });
  });

  it("uses the host for weak function pointers, including env metadata", () => {
    const host = () => 12;
    const { options, link } = prepare({
      imports: ["env", "GOT.func"].map((module) => ({
        module,
        name: "weak",
        kind: module === "env" ? "function" : "global",
      })),
      exports: [{ name: "weak", kind: "function" }],
      hostExports: { weak: host },
      importedSymbols: [{ module: "env", name: "weak", flags: 1 }],
    });
    link({ weak: () => assert.fail("local weak definition used") }, (func) => {
      assert.equal(func, host);
      return 5;
    });
    assert.equal(options.env.weak, host);
    assert.equal(options["GOT.func"].weak.value, 5);
  });

  it("uses shared host functions even when a plugin exports strong definitions", () => {
    const names = [
      "malloc",
      "calloc",
      "realloc",
      "free",
      "aligned_alloc",
      "posix_memalign",
      "memcpy",
      "memmove",
      "memset",
    ];
    const hostExports = Object.fromEntries(names.map((name) => [name, () => name]));
    const { options, link } = prepare({
      imports: names.flatMap((name) => [
        { module: "env", name, kind: "function" },
        { module: "GOT.func", name, kind: "global" },
      ]),
      exports: names.map((name) => ({ name, kind: "function" })),
      hostExports,
    });
    const functions = [];
    link(
      Object.fromEntries(names.map((name) => [name, () => assert.fail("local heap used")])),
      (func) => {
        functions.push(func);
        return functions.length;
      },
    );
    names.forEach((name, index) => {
      assert.equal(options.env[name], hostExports[name]);
      assert.equal(functions[index], hostExports[name]);
    });
  });

  it("maps private libc aliases to the public host functions", () => {
    const names = ["malloc", "calloc", "free"];
    const hostExports = Object.fromEntries(names.map((name) => [name, () => name]));
    const { options, link } = prepare({
      imports: names.flatMap((name) => [
        { module: "env", name: `__libc_${name}`, kind: "function" },
        { module: "GOT.func", name: `__libc_${name}`, kind: "global" },
      ]),
      exports: names.map((name) => ({ name: `__libc_${name}`, kind: "function" })),
      hostExports,
    });
    const functions = [];
    link(
      Object.fromEntries(
        names.map((name) => [`__libc_${name}`, () => assert.fail("local heap used")]),
      ),
      (func) => {
        functions.push(func);
        return functions.length;
      },
    );
    names.forEach((name, index) => {
      assert.equal(options.env[`__libc_${name}`], hostExports[name]);
      assert.equal(functions[index], hostExports[name]);
    });
  });

  it("preserves env functions supplied by the caller", () => {
    const supplied = () => 42;
    const { options } = prepare({
      options: { env: { supplied } },
      imports: [{ module: "env", name: "supplied", kind: "function" }],
      hostExports: { supplied: () => assert.fail("replaced supplied import") },
    });
    assert.equal(options.env.supplied, supplied);
  });

  it("keeps API wrappers for direct calls and raw exports for function addresses", () => {
    const direct = () => 42;
    const raw = () => 7;
    const { options, link } = prepare({
      imports: [
        { module: "env", name: "csoundDestroy", kind: "function" },
        { module: "GOT.func", name: "csoundDestroy", kind: "global" },
      ],
      hostExports: { csoundDestroy: direct },
      hostTableExports: { csoundDestroy: raw },
    });
    link({}, (func) => {
      assert.equal(func, raw);
      return 5;
    });
    assert.equal(options.env.csoundDestroy, direct);
    assert.equal(options["GOT.func"].csoundDestroy.value, 5);
  });

  it("does not treat inherited functions as host exports or supplied imports", () => {
    assert.throws(
      () => prepare({ imports: [{ module: "env", name: "toString", kind: "function" }] }),
      /Missing WebAssembly host function/,
    );
  });

  it("relocates host and local function and data addresses", () => {
    const hostFunction = () => 1;
    const localFunction = () => 2;
    const { options, link } = prepare({
      imports: [
        ...["hostFunction", "localFunction"].map((name) => ({
          module: "GOT.func",
          name,
          kind: "global",
        })),
        ...["hostData", "localData"].map((name) => ({ module: "GOT.mem", name, kind: "global" })),
      ],
      hostExports: { hostFunction, hostData: new WebAssembly.Global({ value: "i32" }, 64) },
    });
    const functions = [];
    link({ localFunction, localData: new WebAssembly.Global({ value: "i32" }, 32) }, (func) => {
      functions.push(func);
      return functions.length + 9;
    });
    assert.deepEqual(functions, [hostFunction, localFunction]);
    assert.equal(options["GOT.func"].hostFunction.value, 10);
    assert.equal(options["GOT.func"].localFunction.value, 11);
    assert.equal(options["GOT.mem"].hostData.value, 64);
    assert.equal(options["GOT.mem"].localData.value, 1056);
  });

  it("keeps strong local data and resolves weak data to the host", () => {
    const names = ["strong", "weak"];
    const { options, link } = prepare({
      imports: names.map((name) => ({ module: "GOT.mem", name, kind: "global" })),
      hostExports: Object.fromEntries(
        names.map((name) => [name, new WebAssembly.Global({ value: "i32" }, 64)]),
      ),
      importedSymbols: [{ module: "env", name: "weak", flags: 1 }],
    });
    link(
      Object.fromEntries(names.map((name) => [name, new WebAssembly.Global({ value: "i32" }, 32)])),
    );
    assert.equal(options["GOT.mem"].strong.value, 1056);
    assert.equal(options["GOT.mem"].weak.value, 64);
  });

  it("leaves missing weak addresses null and traps direct calls", () => {
    const imports = [
      { module: "env", name: "optional", kind: "function" },
      { module: "GOT.func", name: "optional", kind: "global" },
      { module: "GOT.mem", name: "optionalData", kind: "global" },
    ];
    const { options, link } = prepare({
      imports,
      importedSymbols: imports.map((entry) => ({ ...entry, module: "env", flags: 17 })),
    });
    link({}, () => assert.fail("must not allocate a table slot for a missing weak symbol"));
    assert.equal(options["GOT.func"].optional.value, 0);
    assert.equal(options["GOT.mem"].optionalData.value, 0);
    assert.throws(() => options.env.optional(), WebAssembly.RuntimeError);
  });

  it("rejects missing strong symbols and invalid address imports", () => {
    assert.throws(
      () => prepare({ imports: [{ module: "env", name: "absent", kind: "function" }] }),
      /Missing WebAssembly host function/,
    );
    for (const module of ["GOT.func", "GOT.mem"]) {
      const { link } = prepare({ imports: [{ module, name: "absent", kind: "global" }] });
      assert.throws(() => link({}), /Missing WebAssembly/);
      assert.throws(
        () => prepare({ imports: [{ module, name: "absent", kind: "function" }] }),
        /Invalid WebAssembly address import/,
      );
    }
  });

  it("gives each static exception runtime its own tags", () => {
    const imports = ["__c_longjmp", "__cpp_exception"].map((name) => ({
      module: "env",
      name,
      kind: "tag",
    }));
    const one = prepare({ imports }).options.env;
    const two = prepare({ imports }).options.env;
    for (const { name } of imports) {
      const exception = new WebAssembly.Exception(one[name], [42]);
      assert.equal(exception.getArg(one[name], 0), 42);
      assert.equal(exception.is(two[name]), false);
    }
  });
});
