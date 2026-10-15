import assert from "node:assert/strict";

// Separate module instances model the two independently compiled browser files.
const main = await import("../../src/utils/worklet-context.js?main");
const compatibility = await import("../../src/utils/worklet-context.js?compatibility");

describe("worklet contexts shared by main and compatibility bundles", () => {
  it("shares pending and completed registration and allocates distinct node IDs", async () => {
    const context = {};
    let registrations = 0;
    let finish;
    const register = () => {
      registrations++;
      return new Promise((resolve) => {
        finish = resolve;
      });
    };
    const first = main.registerWorkletModule(context, "processor", register);
    const second = compatibility.registerWorkletModule(context, "processor", register);
    assert.equal(first, second);
    await Promise.resolve();
    assert.equal(registrations, 1);
    finish();
    await Promise.all([first, second]);
    await compatibility.registerWorkletModule(context, "processor", register);
    assert.equal(registrations, 1);
    assert.equal(main.nextWorkletNodeId(context), "audioWorklet0");
    assert.equal(compatibility.nextWorkletNodeId(context), "audioWorklet1");
  });

  it("retries a failed registration from the other bundle", async () => {
    const context = {};
    await assert.rejects(
      main.registerWorkletModule(context, "processor", () => {
        throw new Error("load failed");
      }),
      /load failed/,
    );
    let registrations = 0;
    await compatibility.registerWorkletModule(context, "processor", () => {
      registrations++;
    });
    assert.equal(registrations, 1);
  });

  it("keeps different processors and contexts separate", async () => {
    const first = {};
    const second = {};
    let registrations = 0;
    const register = () => {
      registrations++;
    };
    await Promise.all([
      main.registerWorkletModule(first, "worker", register),
      compatibility.registerWorkletModule(first, "singlethread", register),
      compatibility.registerWorkletModule(second, "worker", register),
    ]);
    assert.equal(registrations, 3);
    assert.equal(main.nextWorkletNodeId(first), "audioWorklet0");
    assert.equal(compatibility.nextWorkletNodeId(second), "audioWorklet0");
  });
});
