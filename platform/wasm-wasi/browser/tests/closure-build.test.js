import assert from "node:assert/strict";
import { mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import path from "node:path";
import { test } from "node:test";
import { runClosureCompiler } from "../goog/closure-compiler.js";

async function compile(source, options, config = {}) {
  const directory = mkdtempSync(path.join(tmpdir(), "csound-closure-"));
  const input = path.join(directory, "input.js");
  writeFileSync(input, source);
  try {
    await runClosureCompiler(
      {
        js: input,
        js_output_file: path.join(directory, "output.js"),
        compilation_level: "ADVANCED",
        ...config,
      },
      options,
    );
  } finally {
    rmSync(directory, { recursive: true, force: true });
  }
}

const typeWarning = '/** @type {number} */ const value = "text"; console.log(value);';

test("production rejects type warnings even when Closure exits successfully", async () => {
  await assert.rejects(compile(typeWarning), /warnings are fatal/);
});

test("production rejects unknown defines", async () => {
  await assert.rejects(
    compile('console.log("ok");', undefined, { define: ["UNKNOWN_BUILD_DEFINE=1"] }),
    /warnings are fatal/,
  );
});

test("production accepts a clean compilation", async () => {
  await compile("/** @type {number} */ const value = 1; console.log(value);");
});

test("development can report warnings without failing", async () => {
  await compile(typeWarning, { failOnWarnings: false });
});

test("compiler errors still fail in development", async () => {
  await assert.rejects(
    compile("const = ;", { failOnWarnings: false }),
    /Closure compilation failed/,
  );
});
