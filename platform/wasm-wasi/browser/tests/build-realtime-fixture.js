import { copyFileSync, existsSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";
const cwd = fileURLToPath(new URL("..", import.meta.url));
const probe = fileURLToPath(
  new URL("../../test-artifacts/csound-browser-realtime.wasm.z", import.meta.url),
);
if (!existsSync(probe)) throw new Error("Run scripts/compile-test-fixtures.sh first");
const env = { ...process.env };
delete env.CSOUND_THREADED_WASM;
try {
  execFileSync("npm", ["run", "build:prod"], {
    cwd,
    stdio: "inherit",
    env: { ...env, CSOUND_THREADED_WASM: probe },
  });
  copyFileSync(
    new URL("../dist/csound.js", import.meta.url),
    new URL("../../test-artifacts/csound-probe.js", import.meta.url),
  );
} finally {
  // Leave the local package and linked apps on the production engine.
  execFileSync("npm", ["run", "build:prod"], { cwd, stdio: "inherit", env });
}
