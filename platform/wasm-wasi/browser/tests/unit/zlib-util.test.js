import assert from "node:assert/strict";
import { Util } from "../../src/zlib/util.js";

describe("zlib byte strings", () => {
  it("preserves every byte, including NUL and high bytes", () => {
    const bytes = Uint8Array.from({ length: 256 }, (_, index) => index);
    assert.deepEqual(Util.stringToByteArray(String.fromCharCode(...bytes)), bytes);
  });

  it("returns an empty byte array for an empty string", () => {
    assert.deepEqual(Util.stringToByteArray(""), new Uint8Array());
  });
});
