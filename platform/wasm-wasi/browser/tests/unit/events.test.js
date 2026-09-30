/* eslint-env mocha */

import assert from "node:assert/strict";
import { PublicEventAPI } from "../../src/events.js";

describe("public events", () => {
  it("removes every listener when the instance terminates", () => {
    const events = new PublicEventAPI({});
    const api = events.decorateAPI({});
    let calls = 0;
    const names = [
      "play",
      "pause",
      "stop",
      "realtimePerformanceStarted",
      "realtimePerformancePaused",
      "realtimePerformanceResumed",
      "realtimePerformanceEnded",
      "renderStarted",
      "renderEnded",
      "onAudioNodeCreated",
      "message",
      "debugCallback",
      Symbol("custom"),
    ];
    for (const name of names) {
      api.on(name, () => calls++);
      api.once(name, () => calls++);
    }

    events.terminateInstance();

    assert.deepEqual(api.eventNames(), []);
    for (const name of names) {
      assert.equal(api.listenerCount(name), 0);
      assert.equal(events.eventEmitter.emit(name), false);
    }
    assert.equal(calls, 0);
    events.terminateInstance();
    assert.deepEqual(api.eventNames(), []);
  });

  it("delivers payloads, honors context and removes one-time listeners", () => {
    const events = new PublicEventAPI({});
    const api = events.decorateAPI({});
    const context = { messages: [] };
    function listener(message) {
      this.messages.push(message);
    }
    const emitter = api.on("message", listener, context);
    assert.equal(emitter, events.eventEmitter);
    assert.equal(api.once("message", listener, context), emitter);
    assert.equal(api.listenerCount("message"), 2);
    assert.deepEqual(api.listeners("message"), [listener, listener]);

    events.triggerMessage({ log: "first" });
    events.triggerMessage({ log: "second" });
    assert.deepEqual(context.messages, ["first", "first", "second"]);
    assert.equal(api.listenerCount("message"), 1);
    assert.equal(api.off("message", listener, context), emitter);
    assert.equal(api.listenerCount("message"), 0);

    const node = {};
    let received;
    api.addListener("onAudioNodeCreated", (value) => {
      received = value;
    });
    events.triggerOnAudioNodeCreated(node);
    assert.equal(received, node);
    api.removeListener("onAudioNodeCreated");
    assert.deepEqual(api.eventNames(), []);
  });

  it("emits lifecycle events and only emits derived state changes once", () => {
    const events = new PublicEventAPI({});
    const api = events.decorateAPI({});
    const received = [];
    const names = [
      "play",
      "pause",
      "stop",
      "realtimePerformanceStarted",
      "realtimePerformancePaused",
      "realtimePerformanceResumed",
      "realtimePerformanceEnded",
      "renderStarted",
      "renderEnded",
      "debugCallback",
    ];
    for (const name of names) api.on(name, () => received.push(name));

    events.triggerRealtimePerformanceStarted();
    events.triggerRealtimePerformanceStarted();
    events.triggerRealtimePerformancePaused();
    events.triggerRealtimePerformanceResumed();
    events.triggerRealtimePerformanceEnded();
    events.triggerRenderStarted();
    events.triggerRenderEnded();
    events.triggerDebugCallback();

    assert.deepEqual(received, [
      "realtimePerformanceStarted",
      "play",
      "realtimePerformanceStarted",
      "realtimePerformancePaused",
      "pause",
      "realtimePerformanceResumed",
      "play",
      "realtimePerformanceEnded",
      "stop",
      "renderStarted",
      "renderEnded",
      "debugCallback",
    ]);
    api.removeAllListeners("play");
    assert.equal(api.listenerCount("play"), 0);
    assert.equal(api.listenerCount("pause"), 1);
    api.removeAllListeners();
    assert.deepEqual(api.eventNames(), []);
  });
});
