/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import fs from "fs";
import { browserBinaries } from "./goog/browser-binaries.js";
import path from "path";
import { fileURLToPath } from "url";
import { inlineArraybuffer } from "./goog/inline-arraybuffer.js";
import { runClosureCompiler } from "./goog/closure-compiler.js";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = __dirname;

fs.mkdirSync(path.join(rootDir, "dist"), { recursive: true });
fs.writeFileSync(
  path.join(rootDir, "dist", "__csound_wasm_static_tools.inline.js"),
  inlineArraybuffer(browserBinaries(rootDir).baseline, "binary.wasm"),
);

(async () => {
  const closureConfig = {
    entry_point: "./tools/csound-no-audio.js",
    js_output_file: path.join(rootDir, "dist", "csound-no-audio.js"),
    browser_featureset_year: 2020,
    externs: [path.join(rootDir, "externs.js")],
    js: [
      "./tools/csound-no-audio.js",
      "./src/*.js",
      "./src/**/*.js",
      "./dist/*tools.inline.js",
      "./goog/mocks/package.json",
      "./goog/mocks/index.js",
      "./node_modules/google-closure-library/package.json",
      "./node_modules/google-closure-library/**/*.js",
    ],
    // Match the main bundle's exclusion for upstream Closure-library JSDoc.
    hide_warnings_for: ["./node_modules/google-closure-library/closure/goog/base.js"],
    jscomp_off: ["accessControls"],
    jscomp_error: ["globalThis"],
    assume_function_wrapper: false,
    compilation_level: "SIMPLE_OPTIMIZATIONS",
    language_in: "ECMASCRIPT_2021",
    process_common_js_modules: true,
    rewrite_polyfills: false,
    module_resolution: "NODE",
    dependency_mode: "PRUNE",
    force_inject_library: "base",
    debug: false,
    source_map_include_content: true,
    output_wrapper: `%output% const Csound = Csound$$module$tools$csound_no_audio; export { Csound };`,
  };

  await runClosureCompiler(closureConfig);
})();
