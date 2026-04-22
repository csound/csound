const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const { spawnSync } = require("node:child_process");

const packageRoot = path.resolve(__dirname, "..", "..");
const tempRoot = fs.mkdtempSync(path.join(os.tmpdir(), "csound-browser-types-"));
const fixturePackageDir = path.join(tempRoot, "node_modules", "@csound", "browser");
const fixturePackageJson = path.join(fixturePackageDir, "package.json");
const fixtureSourceFile = path.join(__dirname, "exports.fixture.txt");
const testFile = path.join(tempRoot, "exports.ts");
const tscPath = path.join(
  packageRoot,
  "node_modules",
  ".bin",
  process.platform === "win32" ? "tsc.cmd" : "tsc",
);

fs.mkdirSync(fixturePackageDir, { recursive: true });
fs.writeFileSync(
  fixturePackageJson,
  JSON.stringify(
    {
      name: "@csound/browser",
      private: true,
      types: path.relative(fixturePackageDir, path.join(packageRoot, "index.d.ts")),
    },
    null,
    2,
  ),
);
fs.writeFileSync(testFile, fs.readFileSync(fixtureSourceFile, "utf8"));

const result = spawnSync(
  tscPath,
  ["--noEmit", "--module", "esnext", "--moduleResolution", "node", "--skipLibCheck", testFile],
  {
    cwd: tempRoot,
    stdio: "inherit",
  },
);

fs.rmSync(tempRoot, { force: true, recursive: true });

if (result.error) {
  throw result.error;
}

process.exit(result.status ?? 1);
