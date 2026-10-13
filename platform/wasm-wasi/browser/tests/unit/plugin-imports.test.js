import assert from "node:assert/strict";
import { preparePluginImports } from "../../src/utils/plugin-imports.js";

describe("WebAssembly plugin imports", () => {
  const prepare = (overrides = {}) => {
    const options = { env: {} };
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
    });
    link({ host: () => -1, local: () => 42 });
    assert.equal(options.env.host(), 12);
    assert.equal(options.env.local(), 42);
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
