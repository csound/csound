import assert from "node:assert/strict";
import fs from "node:fs";
import { pathToFileURL } from "node:url";
globalThis.goog = { define: (_name, value) => value, global: globalThis };
const { WASI } = await import(pathToFileURL(process.argv[3] + "/filesystem/preview3.js"));
const module = new WebAssembly.Module(fs.readFileSync(process.argv[2]));
const imports = WebAssembly.Module.imports(module);
assert(imports.length > 0);
assert(
  imports.every(
    ({ module, kind }) =>
      (module === "env" && (kind === "memory" || kind === "function")) ||
      module === "$root" ||
      (module.startsWith("wasi:") && module.endsWith("@0.3.2")),
  ),
);
const { instantiateCommand } = await import(pathToFileURL(process.argv[3] + "/utils/wasm64.js"));
const wasi = new WASI({ args: ["probe", "preview3"] });
wasi.mkdir("/project");
assert.equal(wasi.chdir("/project"), 0);
wasi.fd[0].node.buffers = [new TextEncoder().encode("abc")];
wasi.fd[0].node.size = 3;
const instance = instantiateCommand(module, wasi);
assert.equal(wasi.startCommand(instance), 0, wasi.readOutput(2));
assert.match(wasi.readOutput(1), /Preview 3 arguments, files, streams, clocks and random passed/);
assert.deepEqual(wasi.readdir("/"), ["project"]);
assert.deepEqual(wasi.readdir("/project"), []);
console.log(wasi.readOutput(1));
