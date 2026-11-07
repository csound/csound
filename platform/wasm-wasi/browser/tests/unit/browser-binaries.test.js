import assert from "node:assert/strict";
import { mkdtempSync, mkdirSync, writeFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import path from "node:path";
import { browserBinaries } from "../../goog/browser-binaries.js";
import { inlineArraybuffer } from "../../goog/inline-arraybuffer.js";

describe("browser binary packaging", () => {
  let directory;
  let root;
  let local;
  let installed;
  beforeEach(() => {
    directory = mkdtempSync(path.join(tmpdir(), "csound-browser-binaries-"));
    root = path.join(directory, "browser");
    local = path.join(directory, "lib");
    installed = path.join(root, "node_modules/@csound/wasm-bin/lib");
    mkdirSync(local, { recursive: true });
    mkdirSync(installed, { recursive: true });
  });
  afterEach(() => rmSync(directory, { recursive: true, force: true }));
  const put = (directory, filename, bytes = "fixture") => {
    const filename_ = path.join(directory, filename);
    writeFileSync(filename_, bytes);
    return filename_;
  };

  it("builds one baseline entry when the installed package has no companion", () => {
    const baseline = put(installed, "csound.wasm.z", "baseline-only");
    const selected = browserBinaries(root);
    assert.deepEqual(selected, {
      baseline,
      threaded: undefined,
    });
    assert.ok(
      inlineArraybuffer(selected.baseline).includes(
        Buffer.from("baseline-only").toString("base64"),
      ),
    );
  });

  it("does not mix a local baseline-only build with a package companion", () => {
    put(installed, "csound.wasm.z");
    put(installed, "csound-browser-realtime.wasm.z");
    const baseline = put(local, "csound.wasm.z");
    assert.deepEqual(browserBinaries(root), {
      baseline,
      threaded: undefined,
    });
  });

  it("selects matching local artifacts and the threaded main entry", () => {
    put(installed, "csound.wasm.z");
    const baseline = put(local, "csound.wasm.z");
    const threaded = put(local, "csound-browser-realtime.wasm.z");
    assert.deepEqual(browserBinaries(root), { baseline, threaded });
  });

  it("accepts an explicit raw fixture but rejects a missing override", () => {
    put(local, "csound.wasm.z");
    const fixture = put(directory, "probe.wasm");
    assert.equal(browserBinaries(root, fixture).threaded, fixture);
    assert.throws(
      () => browserBinaries(root, path.join(directory, "missing.wasm")),
      /CSOUND_THREADED_WASM/,
    );
  });
  it("requires an explicit target for wasm64 and keeps its companion separate", () => {
    const baseline = put(local, "csound.wasm.z");
    const wide = put(local, "csound-wasm64.wasm.z");
    const threaded = put(local, "csound-wasm64-browser-realtime.wasm.z");
    assert.deepEqual(browserBinaries(root), { baseline, threaded: undefined });
    assert.deepEqual(browserBinaries(root, undefined, "wasm64"), { baseline: wide, threaded });
  });

  it("does not fall back to wasm64 when the default engine is missing", () => {
    put(local, "csound-wasm64.wasm.z");
    assert.throws(() => browserBinaries(root), /binary is missing/);
    assert.throws(() => browserBinaries(root, undefined, "unknown"), /Unknown browser target/);
  });
});
