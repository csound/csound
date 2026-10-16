import assert from "node:assert/strict";
import { assertRealtimeCallAllowed } from "../../src/utils/realtime-api.js";

describe("active realtime API", () => {
  it("admits notes and short end events, but keeps table generation stopped", () => {
    assert.doesNotThrow(() => assertRealtimeCallAllowed("csoundEvent", [0, [1, 0, 0.1]]));
    for (const fields of [[], [0], [0, 0.1]]) {
      assert.doesNotThrow(() => assertRealtimeCallAllowed("csoundEvent", [2, fields]));
      assert.throws(
        () => assertRealtimeCallAllowed("csoundEvent", [1, fields]),
        /Table generation requires stopped realtime performance/,
      );
    }
    for (const [type, fields] of [
      [0, [1, 0]],
      [2, [NaN]],
      [2, Array(1)],
      [2, Array(65).fill(0)],
      [3, []],
    ])
      assert.throws(() => assertRealtimeCallAllowed("csoundEvent", [type, fields]), TypeError);
  });

  it("allows numeric control access without opening string or compiler APIs", () => {
    assert.doesNotThrow(() => assertRealtimeCallAllowed("csoundGetControlChannel", ["gain"]));
    assert.doesNotThrow(() => assertRealtimeCallAllowed("csoundSetControlChannel", ["gain", 0.5]));
    for (const name of ["", "bad\0name", 12, undefined])
      assert.throws(() => assertRealtimeCallAllowed("csoundGetControlChannel", [name]), TypeError);
    for (const value of [NaN, Infinity, "1", undefined])
      assert.throws(
        () => assertRealtimeCallAllowed("csoundSetControlChannel", ["gain", value]),
        TypeError,
      );
    for (const name of [
      "csoundSetStringChannel",
      "csoundGetStringChannel",
      "csoundCompileOrc",
      "csoundInputMessageAsync",
    ])
      assert.throws(() => assertRealtimeCallAllowed(name, []), /requires stopped/);
  });
});
