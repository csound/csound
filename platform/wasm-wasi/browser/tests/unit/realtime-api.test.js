import assert from "node:assert/strict";
import { assertRealtimeCallAllowed } from "../../src/utils/realtime-api.js";

describe("active realtime API", () => {
  it("admits numeric notes, but rejects table and end events before field validation", () => {
    assert.doesNotThrow(() => assertRealtimeCallAllowed("csoundEvent", [0, [1, 0, 0.1]]));
    for (const type of [1, 2]) {
      for (const fields of [[], [0], [1, 0, 0.1]])
        assert.throws(
          () => assertRealtimeCallAllowed("csoundEvent", [type, fields]),
          /requires stopped realtime performance/,
        );
    }
    assert.throws(() => assertRealtimeCallAllowed("csoundEvent", [0, [1, 0]]), /3 to 64/);
  });
});
