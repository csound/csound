import assert from "node:assert/strict";
import { mkdtempSync, mkdirSync, writeFileSync, rmSync } from "node:fs";
import { deflateSync } from "node:zlib";
import { tmpdir } from "node:os";
import path from "node:path";
import { browserBinaries } from "../../goog/browser-binaries.js";
import { inlineArraybuffer } from "../../goog/inline-arraybuffer.js";

const preview3 = "wasi:random/random@0.3.2";
const preview1 = "wasi_snapshot_preview1";
const moduleWithImports = (...names) => {
  const imports = names.flatMap((name) => [name.length, ...Buffer.from(name), 1, 102, 0, 0]);
  return Buffer.from([
    0,
    97,
    115,
    109,
    1,
    0,
    0,
    0,
    1,
    4,
    1,
    96,
    0,
    0,
    2,
    imports.length + 1,
    names.length,
    ...imports,
  ]);
};
const reactor = deflateSync(moduleWithImports(preview3));

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
  const put = (directory, filename, bytes = reactor) => {
    const filename_ = path.join(directory, filename);
    writeFileSync(filename_, bytes);
    return filename_;
  };

  it("builds one baseline entry when the installed package has no companion", () => {
    const baseline = put(installed, "csound.wasm.z");
    const selected = browserBinaries(root);
    assert.deepEqual(selected, {
      baseline,
      threaded: undefined,
    });
    assert.ok(inlineArraybuffer(selected.baseline).includes(reactor.toString("base64")));
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
    const fixture = put(directory, "probe.wasm", moduleWithImports(preview3));
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

  it("rejects older installed engines and skips stale adjacent companions", () => {
    const oldReactor = deflateSync(moduleWithImports(preview1));
    put(installed, "csound.wasm.z", oldReactor);
    assert.throws(() => browserBinaries(root), /Expected a WASI Preview 3/);

    const baseline = put(local, "csound.wasm.z");
    const stale = put(local, "csound-browser-realtime.wasm.z", oldReactor);
    assert.deepEqual(browserBinaries(root), { baseline, threaded: undefined });
    assert.throws(() => browserBinaries(root, stale), /Expected a WASI Preview 3/);

    const mixed = put(directory, "mixed.wasm", moduleWithImports(preview3, "wasi"));
    assert.throws(() => browserBinaries(root, mixed), /Expected a WASI Preview 3/);
  });

  it("does not fall back to wasm64 when the default engine is missing", () => {
    put(local, "csound-wasm64.wasm.z");
    assert.throws(() => browserBinaries(root), /binary is missing/);
    assert.throws(() => browserBinaries(root, undefined, "unknown"), /Unknown browser target/);
  });
});
