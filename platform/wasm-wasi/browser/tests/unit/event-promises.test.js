import assert from "node:assert/strict";
import { EventPromises } from "../../src/utils/event-promises.js";

describe("transport deadlines", () => {
  let originalSetTimeout;
  let originalClearTimeout;
  let timers;
  let delays;
  beforeEach(() => {
    originalSetTimeout = globalThis.setTimeout;
    originalClearTimeout = globalThis.clearTimeout;
    timers = [];
    delays = [];
    globalThis.setTimeout = (callback, delay) => {
      timers.push(callback);
      delays.push(delay);
      return timers.length;
    };
    globalThis.clearTimeout = () => {};
  });

  it("allows worker preparation more time without changing the default start deadline", () => {
    const events = new EventPromises();
    events.createStartPromise(35000);
    assert.deepEqual(delays, [35000]);
    events.releaseStartPromise();
    events.createStartPromise();
    assert.deepEqual(delays, [35000, 2000]);
    events.releaseStartPromise();
  });
  afterEach(() => {
    globalThis.setTimeout = originalSetTimeout;
    globalThis.clearTimeout = originalClearTimeout;
  });

  for (const event of ["start", "stop", "pause", "resume"]) {
    it(`rejects a guarded ${event} before cleanup resolves its pending acknowledgement`, async () => {
      let rejectFailure;
      const failure = new Promise((_resolve, reject) => {
        rejectFailure = reject;
      });
      const error = new Error(`${event} timed out`);
      const suffix = event[0].toUpperCase() + event.slice(1);
      const events = new EventPromises((expired) => {
        assert.equal(expired, event);
        assert.ok(events[`${event}Promise`]);
        rejectFailure(error);
        events[`release${suffix}Promise`]();
      });
      events[`create${suffix}Promise`]();
      const guarded = Promise.race([events[`waitFor${suffix}`](), failure]);
      timers[0]();
      await assert.rejects(guarded, (received) => received === error);
      assert.equal(events[`${event}Promise`], undefined);
    });
  }

  it("keeps the existing timeout result when no handler was requested", async () => {
    const events = new EventPromises();
    events.createStopPromise();
    const stopped = events.waitForStop();
    const warn = console.warn;
    try {
      console.warn = () => {};
      timers[0]();
    } finally {
      console.warn = warn;
    }
    await stopped;
    assert.equal(events.isWaitingToStop(), false);
  });
});
