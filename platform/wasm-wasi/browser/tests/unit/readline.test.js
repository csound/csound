/* eslint-env mocha */
import assert from "node:assert/strict";
import { PublicEventAPI } from "../../src/events.js";
import { messageEventHandler } from "../../src/mains/messages.main.js";

describe("readline host events", () => {
  it("keeps empty prompts distinct from close events and out of log messages", () => {
    const events = new PublicEventAPI({});
    const api = events.decorateAPI({});
    const received = [];
    const logs = [];
    api.on("readline", (event) => received.push(event));
    api.on("message", (message) => logs.push(message));
    const deliver = messageEventHandler({ publicEvents: events });
    for (const prompt of ["", null, "名前> ", null]) {
      deliver({ data: { log: { readline: { csound: 42, requestId: 1, prompt } } } });
    }
    assert.deepEqual(received, [
      { requestId: 1, prompt: "" },
      { requestId: 1, prompt: null },
      { requestId: 1, prompt: "名前> " },
      { requestId: 1, prompt: null },
    ]);
    assert.deepEqual(logs, []);
  });

  for (const end of ["triggerRealtimePerformanceEnded", "triggerRenderEnded"]) {
    it(`closes the pending prompt once on ${end}`, () => {
      const events = new PublicEventAPI({});
      const received = [];
      events.decorateAPI({}).on("readline", (event) => received.push(event));
      events.triggerReadline({ requestId: 3, prompt: "input> " });
      events[end]();
      events.triggerReadline({ requestId: 4, prompt: "next> " });
      // A delayed close from cleanup must not close the next request.
      events.triggerReadline({ requestId: 3, prompt: null });
      events.triggerReadline({ requestId: 4, prompt: null });
      assert.deepEqual(received, [
        { requestId: 3, prompt: "input> " },
        { requestId: 3, prompt: null },
        { requestId: 4, prompt: "next> " },
        { requestId: 4, prompt: null },
      ]);
    });
  }
});

describe("readline submissions", () => {
  let submit;
  let originalGoogDescriptor;
  before(async () => {
    originalGoogDescriptor = Object.getOwnPropertyDescriptor(globalThis, "goog");
    Object.defineProperty(globalThis, "goog", {
      configurable: true,
      value: { define: (_name, value) => value },
    });
    ({ csoundReadlineSubmit: submit } = await import("../../src/modules/control-events.js"));
  });
  after(() => {
    if (originalGoogDescriptor) Object.defineProperty(globalThis, "goog", originalGoogDescriptor);
    else delete globalThis.goog;
  });

  it("rejects invalid IDs and embedded NULs before calling WASM", () => {
    for (const id of [0, -1, 1.5, 2 ** 32, NaN]) assert.equal(submit({})(1, id, "x"), -1);
    assert.equal(submit({})(1, 1, "one\0two"), -1);
  });

  it("copies UTF-8 input, preserves the unsigned ID and frees temporary memory", () => {
    const memory = new WebAssembly.Memory({ initial: 1 });
    let freed;
    const wasm = {
      wasi: { memory },
      exports: {
        allocStringMem: () => 16,
        freeStringMem: (ptr) => {
          freed = ptr;
        },
        csoundReadlineSubmit: (csound, id, ptr) => {
          assert.equal(csound, 123);
          assert.equal(id, 0xffff_ffff);
          assert.equal(ptr, 16);
          assert.deepEqual([...new Uint8Array(memory.buffer, ptr, 3)], [195, 169, 0]);
          return -1;
        },
      },
    };
    assert.equal(submit(wasm)(123, 0xffff_ffff, "é"), -1);
    assert.equal(freed, 16);
  });
});
