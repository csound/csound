/* eslint-env mocha */
import assert from "node:assert/strict";

// AudioWorklet lacks TextDecoder; compile.js selects this fallback there.
describe("AudioWorklet UTF-8 decoder", () => {
  let decoder;
  let originalGoogDescriptor;
  before(async () => {
    originalGoogDescriptor = Object.getOwnPropertyDescriptor(globalThis, "goog");
    Object.defineProperty(globalThis, "goog", {
      configurable: true,
      value: { define: () => true },
    });
    ({ decoder } = await import("../../src/utils/text-encoders.js?worklet-test"));
  });
  after(() => {
    if (originalGoogDescriptor) Object.defineProperty(globalThis, "goog", originalGoogDescriptor);
    else delete globalThis.goog;
  });

  it("decodes ASCII and two-, three-, and four-byte characters", () => {
    for (const text of ["", "name> ", "héllo", "名前> ", "🎵", "aé名🎵z"]) {
      assert.equal(decoder.decode(new TextEncoder().encode(text)), text);
    }
  });

  it("matches TextDecoder replacement behavior for malformed UTF-8", () => {
    for (const bytes of [
      [0x80],
      [0xc0, 0xaf],
      [0xc2],
      [0xe0, 0x80, 0x80],
      [0xed, 0xa0, 0x80],
      [0xf4, 0x90, 0x80, 0x80],
      [0xf5, 0x80, 0x80, 0x80],
      [0xe2, 0x82],
      [0xe2, 0x82, 0x41],
      [0xc2, 0x41],
    ]) {
      const view = Uint8Array.from(bytes);
      assert.equal(decoder.decode(view), new TextDecoder().decode(view));
    }
  });

  it("respects view bounds and the existing NUL terminator behavior", () => {
    const bytes = new TextEncoder().encode("X名前\0ignored");
    assert.equal(decoder.decode(bytes.subarray(1)), "名前");
  });
});
