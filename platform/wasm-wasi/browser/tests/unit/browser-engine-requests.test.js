import assert from "node:assert/strict";
import { extraEngineRequests } from "../browser-engine-requests.js";
describe("one engine download check", () => {
  it("rejects extra JS or engine artifacts while allowing the selected bundle and plugins", () => {
    const baseline = [
      "/",
      "/dist/csound.js",
      "/plugin_example_shared.wasm",
      "/plugin_example_cpp_shared.wasm",
      "/missing-plugin.wasm",
    ];
    assert.deepEqual(extraEngineRequests(baseline, "/dist/csound.js"), []);
    for (const extra of [
      "/csound.wasm.z",
      "/csound-browser-realtime.wasm",
      "/csound-browser-realtime.wasm.z",
      "/another-engine.js",
      "/probe/csound.js",
    ])
      assert.deepEqual(extraEngineRequests([...baseline, extra], "/dist/csound.js"), [extra]);
  });
});
