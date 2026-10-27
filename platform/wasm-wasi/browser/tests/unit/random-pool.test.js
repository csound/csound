import assert from "node:assert/strict";
import { once } from "node:events";
import { MessageChannel } from "node:worker_threads";
import {
  RandomPool,
  randomBytes,
  serveRandomBytes,
  workletRandomValues,
} from "../../src/utils/random-pool.js";

describe("AudioWorklet random pool", () => {
  it("consumes bytes once, refills between calls, and shares no used storage", async () => {
    const seed = new Uint8Array([1, 2, 3]);
    let requests = 0;
    const pool = new RandomPool(seed, async () => {
      requests++;
      return new Uint8Array([4, 5, 6]);
    });
    assert.deepEqual([...pool.fill(new Uint8Array(2))], [1, 2]);
    assert.deepEqual([...seed], [0, 0, 3]);
    assert.throws(() => pool.fill(new Uint8Array(2)), /exhausted/);
    await new Promise((resolve) => setImmediate(resolve));
    assert.equal(requests, 1);
    assert.deepEqual([...pool.fill(new Uint8Array(4))], [3, 4, 5, 6]);
  });

  it("reports a failed refill instead of reusing bytes", async () => {
    const pool = new RandomPool(new Uint8Array([1]), async () => {
      throw new Error("source closed");
    });
    pool.fill(new Uint8Array(1));
    await new Promise((resolve) => setImmediate(resolve));
    assert.throws(() => pool.fill(new Uint8Array(1)), /source closed/);
  });

  it("refills a direct AudioWorklet host through its message port", async () => {
    const { port1, port2 } = new MessageChannel();
    try {
      serveRandomBytes(port1);
      const fill = workletRandomValues(randomBytes(), port2);
      const response = once(port2, "message");
      fill(new Uint8Array(65537));
      await response;
      await new Promise((resolve) => setImmediate(resolve));
      assert(fill(new Uint8Array(131072)).some((byte) => byte !== 0));
    } finally {
      port1.close();
      port2.close();
    }
  });
});
