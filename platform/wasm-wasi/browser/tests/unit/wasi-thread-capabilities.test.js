import assert from "node:assert/strict";
import { canUseThreadedWasm, canUseSharedWasm } from "../../src/utils/wasi-thread-capabilities.js";

const supported = { workletSupport: true, sharedMemorySupport: true };

describe("browser thread host selection", () => {
  let isolation;
  let validate;
  let sharedArrayBuffer;
  beforeEach(() => {
    isolation = Object.getOwnPropertyDescriptor(globalThis, "crossOriginIsolated");
    Object.defineProperty(globalThis, "crossOriginIsolated", { configurable: true, value: true });
    validate = WebAssembly.validate;
    sharedArrayBuffer = Object.getOwnPropertyDescriptor(globalThis, "SharedArrayBuffer");
  });
  afterEach(() => {
    if (isolation) Object.defineProperty(globalThis, "crossOriginIsolated", isolation);
    else delete globalThis.crossOriginIsolated;
    WebAssembly.validate = validate;
    Object.defineProperty(globalThis, "SharedArrayBuffer", sharedArrayBuffer);
  });

  it("selects the shared host by default only when the required Wasm ABI is supported", () => {
    assert.equal(canUseThreadedWasm(supported), true);
    WebAssembly.validate = () => false;
    assert.equal(canUseThreadedWasm(supported), false);
  });

  it("allows a local shared heap without isolation or a visible SharedArrayBuffer constructor", () => {
    Object.defineProperty(globalThis, "crossOriginIsolated", { configurable: true, value: false });
    Object.defineProperty(globalThis, "SharedArrayBuffer", {
      configurable: true,
      value: undefined,
    });
    assert.equal(canUseSharedWasm(), true);
    assert.equal(canUseThreadedWasm(supported), false);
  });

  it("requires worklets and shared buffers to communicate between threads", () => {
    for (const overrides of [{ workletSupport: false }, { sharedMemorySupport: false }]) {
      assert.equal(canUseThreadedWasm({ ...supported, ...overrides }), false);
    }
    assert.equal(canUseSharedWasm(), true);
  });

  it("rejects host support without isolation or if feature probing throws", () => {
    Object.defineProperty(globalThis, "crossOriginIsolated", { configurable: true, value: false });
    assert.equal(canUseThreadedWasm(supported), false);
    Object.defineProperty(globalThis, "crossOriginIsolated", { configurable: true, value: true });
    WebAssembly.validate = () => {
      throw new Error("Unavailable Wasm engine");
    };
    assert.equal(canUseThreadedWasm(supported), false);
  });
});
