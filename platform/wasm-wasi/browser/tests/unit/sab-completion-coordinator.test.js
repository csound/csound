/* eslint-env mocha */

import assert from "node:assert/strict";

import { SABCompletionCoordinator } from "../../src/utils/sab-completion-coordinator.js";

const createCoordinator = (generation = 1) => {
  const completedStates = [];
  const coordinator = new SABCompletionCoordinator((endState) => completedStates.push(endState));
  coordinator.begin(generation);

  return { completedStates, coordinator };
};

describe("SABCompletionCoordinator", () => {
  it("completes after the end state followed by the stop release", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markEnd("renderEnded", 1);
    assert.deepEqual(completedStates, []);

    coordinator.markStopReleased(1);
    assert.deepEqual(completedStates, ["renderEnded"]);
  });

  it("completes after the stop release followed by the end state", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markStopReleased(1);
    assert.deepEqual(completedStates, []);

    coordinator.markEnd("realtimePerformanceEnded", 1);
    assert.deepEqual(completedStates, ["realtimePerformanceEnded"]);
  });

  it("ignores stale generations", () => {
    const { completedStates, coordinator } = createCoordinator(2);

    coordinator.markEnd("renderEnded", 1);
    coordinator.markStopReleased(1);
    assert.deepEqual(completedStates, []);

    coordinator.markEnd("renderEnded", 2);
    assert.deepEqual(completedStates, []);

    coordinator.markStopReleased(2);
    assert.deepEqual(completedStates, ["renderEnded"]);
  });

  it("ignores undefined generations", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markEnd("renderEnded");
    coordinator.markStopReleased();
    assert.deepEqual(completedStates, []);

    coordinator.markEnd("renderEnded", 1);
    assert.deepEqual(completedStates, []);

    coordinator.markStopReleased(1);
    assert.deepEqual(completedStates, ["renderEnded"]);
  });

  it("discards a partial end state when a new generation begins", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markEnd("renderEnded", 1);
    coordinator.begin(2);
    coordinator.markStopReleased(2);
    assert.deepEqual(completedStates, []);

    coordinator.markEnd("renderEnded", 2);
    assert.deepEqual(completedStates, ["renderEnded"]);
  });

  it("discards a partial stop release when a new generation begins", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markStopReleased(1);
    coordinator.begin(2);
    coordinator.markEnd("renderEnded", 2);
    assert.deepEqual(completedStates, []);

    coordinator.markStopReleased(2);
    assert.deepEqual(completedStates, ["renderEnded"]);
  });

  it("completes only once when a full signal pair is replayed", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markEnd("renderEnded", 1);
    coordinator.markStopReleased(1);
    coordinator.markEnd("renderEnded", 1);
    coordinator.markStopReleased(1);

    assert.deepEqual(completedStates, ["renderEnded"]);
    assert.equal(coordinator.canAccept(1), false);
  });

  it("clears the completion latch for a new generation", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markEnd("renderEnded", 1);
    coordinator.markStopReleased(1);
    coordinator.begin(2);
    coordinator.markStopReleased(2);
    coordinator.markEnd("realtimePerformanceEnded", 2);

    assert.deepEqual(completedStates, ["renderEnded", "realtimePerformanceEnded"]);
  });

  it("latches completion before invoking a re-entrant callback", () => {
    let completionCount = 0;

    function onComplete() {
      completionCount += 1;
      coordinator.markEnd("renderEnded", 1);
      coordinator.markStopReleased(1);
    }

    const coordinator = new SABCompletionCoordinator(onComplete);
    coordinator.begin(1);

    coordinator.markEnd("renderEnded", 1);
    coordinator.markStopReleased(1);

    assert.equal(completionCount, 1);
  });

  it("does not complete early or twice for duplicate end signals", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markEnd("renderEnded", 1);
    coordinator.markEnd("renderEnded", 1);
    assert.deepEqual(completedStates, []);

    coordinator.markStopReleased(1);
    assert.deepEqual(completedStates, ["renderEnded"]);
  });

  it("does not complete early or twice for duplicate stop-release signals", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markStopReleased(1);
    coordinator.markStopReleased(1);
    assert.deepEqual(completedStates, []);

    coordinator.markEnd("renderEnded", 1);
    assert.deepEqual(completedStates, ["renderEnded"]);
  });

  for (const endState of ["renderEnded", "realtimePerformanceEnded"]) {
    it(`reports the ${endState} terminal state`, () => {
      const { completedStates, coordinator } = createCoordinator();

      coordinator.markEnd(endState, 1);
      coordinator.markStopReleased(1);

      assert.deepEqual(completedStates, [endState]);
    });
  }

  it("ignores an old end handler that resumes after a new generation begins", () => {
    const { completedStates, coordinator } = createCoordinator();

    coordinator.markStopReleased(1);
    coordinator.begin(2);
    coordinator.markEnd("renderEnded", 1);
    coordinator.markStopReleased(2);
    assert.deepEqual(completedStates, []);

    coordinator.markEnd("renderEnded", 2);
    assert.deepEqual(completedStates, ["renderEnded"]);
  });
});
