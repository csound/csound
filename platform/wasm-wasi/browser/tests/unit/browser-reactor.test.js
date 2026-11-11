import assert from "node:assert/strict";
import { assertBrowserReactor, assertBrowserPrecision } from "../../src/utils/browser-reactor.js";

const string = (value) => [value.length, ...new TextEncoder().encode(value)];
const section = (id, bytes) => [id, bytes.length, ...bytes];
const fixture = (functions, stack = true) =>
  new WebAssembly.Module(
    new Uint8Array([
      0,
      97,
      115,
      109,
      1,
      0,
      0,
      0,
      ...section(1, [1, 0x60, 0, 0]),
      ...section(3, [1, 0]),
      ...section(6, [1, 0x7f, 1, 0x41, 0, 0x0b]),
      ...section(7, [
        functions.length + Number(stack),
        ...functions.flatMap((name) => [...string(name), 0, 0]),
        ...(stack ? [...string("__stack_pointer"), 3, 0] : []),
      ]),
      ...section(10, [1, 2, 0, 0x0b]),
    ]),
  );

describe("browser reactor boundary", () => {
  it("requires both native float types to use four bytes", () => {
    assert.doesNotThrow(() =>
      assertBrowserPrecision({
        csoundGetSizeOfCsFloat: () => 4,
        csoundGetSizeOfCsDouble: () => 4,
      }),
    );
    for (const [sampleSize, scoreSize] of [
      [8, 8],
      [4, 8],
      [8, 4],
    ]) {
      assert.throws(
        () =>
          assertBrowserPrecision({
            csoundGetSizeOfCsFloat: () => sampleSize,
            csoundGetSizeOfCsDouble: () => scoreSize,
          }),
        /4-byte samples/,
      );
    }
    assert.throws(() => assertBrowserPrecision({}), /4-byte samples/);
  });
  it("accepts a reactor with the child entry and plugin stack export", () => {
    assert.doesNotThrow(() => assertBrowserReactor(fixture(["_initialize", "wasi_thread_start"])));
  });
  it("rejects command artifacts before the host can run their startup", () => {
    for (const names of [
      ["_start", "wasi_thread_start"],
      ["_start", "_initialize", "wasi_thread_start"],
    ])
      assert.throws(() => assertBrowserReactor(fixture(names)), /browser reactor/);
  });
  it("rejects missing thread or stack exports at load time", () => {
    assert.throws(() => assertBrowserReactor(fixture(["_initialize"])), /browser reactor/);
    assert.throws(
      () => assertBrowserReactor(fixture(["_initialize", "wasi_thread_start"], false)),
      /stack pointer/,
    );
  });
});
