import { existsSync, readFileSync } from "node:fs";
import path from "node:path";
import { inflateSync } from "node:zlib";

function isPreview3Reactor(file) {
  const bytes = readFileSync(file);
  const wasm = bytes.subarray(0, 4).equals(Buffer.from("\0asm")) ? bytes : inflateSync(bytes);
  const wasi = WebAssembly.Module.imports(new WebAssembly.Module(wasm))
    .map(({ module }) => module)
    .filter((name) => name === "wasi" || name.startsWith("wasi_") || name.startsWith("wasi:"));
  return wasi.length > 0 && wasi.every((name) => /^wasi:[^@]+@0\.3\.[0-2]$/.test(name));
}

export function browserBinaries(rootDir, threadedOverride, target = "wasm32") {
  if (!["wasm32", "wasm64"].includes(target)) throw new Error(`Unknown browser target: ${target}`);
  const name = target === "wasm64" ? "csound-wasm64" : "csound";
  // Keep the baseline and its companion in one directory so local builds
  // cannot pick up a threaded engine from a different installed version.
  const localBinaryDir = path.resolve(rootDir, "../lib");
  const binaryDir = existsSync(path.join(localBinaryDir, `${name}.wasm.z`))
    ? localBinaryDir
    : path.join(rootDir, "node_modules/@csound/wasm-bin/lib");
  const baseline = path.join(binaryDir, `${name}.wasm.z`);
  if (!existsSync(baseline)) throw new Error(`Csound browser binary is missing: ${baseline}`);
  if (!isPreview3Reactor(baseline))
    throw new Error(`Expected a WASI Preview 3 browser reactor: ${baseline}`);
  const threaded = threadedOverride || path.join(binaryDir, `${name}-browser-realtime.wasm.z`);
  if (threadedOverride && !existsSync(threaded))
    throw new Error(`CSOUND_THREADED_WASM does not exist: ${threaded}`);
  const hasThreadedReactor = existsSync(threaded) && isPreview3Reactor(threaded);
  if (threadedOverride && !hasThreadedReactor)
    throw new Error(`Expected a WASI Preview 3 browser reactor: ${threaded}`);
  return {
    baseline,
    threaded: hasThreadedReactor ? threaded : undefined,
  };
}
