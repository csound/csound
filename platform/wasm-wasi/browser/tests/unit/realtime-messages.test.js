import assert from "node:assert/strict";
import { createWasiThreadEnvironment } from "../../src/utils/wasi-threads-imports.js";

function logger() {
  const memory = new WebAssembly.Memory({ initial: 1, maximum: 1, shared: true });
  const messages = [];
  const env = createWasiThreadEnvironment({
    memory,
    postMessage: (data) => messages.push(data.log),
  });
  const write = (text) => {
    const bytes = new TextEncoder().encode(text);
    new Uint8Array(memory.buffer, 128, bytes.length).set(bytes);
    env.csoundWasiJsMessageCallback(0, 0, bytes.length, 128);
  };
  return { messages, write };
}

describe("realtime Csound log lines", () => {
  it("keeps amplitude columns together until their newline", () => {
    const { messages, write } = logger();
    // Csound writes the heading and each channel in separate callback calls.
    write("M:");
    write("\t0.25000");
    write("\t0.50000");
    assert.deepEqual(messages, []);
    write("\n");
    assert.deepEqual(messages, ["M:\t0.25000\t0.50000"]);
  });

  it("handles progress updates, CRLF, and a trailing partial line", () => {
    const { messages, write } = logger();
    write("progress\r");
    write("\nfirst\nsecond\r\nlast ");
    write("λ\n");
    assert.deepEqual(messages, ["progress", "first", "second", "last λ"]);
  });

  it("keeps unfinished lines local to each worker", () => {
    const first = logger();
    const second = logger();
    first.write("first ");
    second.write("second ");
    first.write("line\n");
    second.write("line\n");
    assert.deepEqual(first.messages, ["first line"]);
    assert.deepEqual(second.messages, ["second line"]);
  });
});
