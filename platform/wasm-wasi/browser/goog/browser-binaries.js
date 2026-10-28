import { existsSync } from "node:fs";
import path from "node:path";

export function browserBinaries(rootDir, threadedOverride) {
  // Use one directory for both memory ABIs; a local baseline-only build must
  // not pick up a threaded companion from a different installed version.
  const localBinaryDir = path.resolve(rootDir, "../lib");
  const binaryDir = existsSync(path.join(localBinaryDir, "csound-wasm64.wasm.z"))
    ? localBinaryDir
    : path.join(rootDir, "node_modules/@csound/wasm-bin/lib");
  const baseline = path.join(binaryDir, "csound-wasm64.wasm.z");
  if (!existsSync(baseline)) throw new Error(`Csound browser binary is missing: ${baseline}`);
  const threaded = threadedOverride || path.join(binaryDir, "csound-wasm64-browser-realtime.wasm.z");
  if (threadedOverride && !existsSync(threaded))
    throw new Error(`CSOUND_THREADED_WASM does not exist: ${threaded}`);
  return {
    baseline,
    threaded: existsSync(threaded) ? threaded : undefined,
  };
}
