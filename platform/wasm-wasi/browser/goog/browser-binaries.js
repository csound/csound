import { existsSync } from "node:fs";
import path from "node:path";

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
  const threaded = threadedOverride || path.join(binaryDir, `${name}-browser-realtime.wasm.z`);
  if (threadedOverride && !existsSync(threaded))
    throw new Error(`CSOUND_THREADED_WASM does not exist: ${threaded}`);
  return {
    baseline,
    threaded: existsSync(threaded) ? threaded : undefined,
  };
}
