import fs from "fs";
import path from "path";
import goog from "google-closure-compiler";
import JarPath from "google-closure-compiler-java";
import { fileURLToPath } from "url";
import { inlineArraybuffer } from "./goog/inline-arraybuffer.js";

const { compiler: ClosureCompiler } = goog;

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = __dirname;

fs.writeFileSync(
  path.join(rootDir, "dist", "__csound_wasm_static_tools.inline.js"),
  inlineArraybuffer("./node_modules/@csound/wasm-bin/lib/csound.static.wasm.z", "binary.wasm"),
);

(async () => {
  const closureConfig = {
    entry_point: "./tools/csound-no-audio.js",
    js_output_file: path.join(rootDir, "dist", "csound-no-audio.js"),
    browser_featureset_year: 2020,
    js: [
      "./tools/csound-no-audio.js",
      "./src/*.js",
      "./src/**/*.js",
      "./dist/*tools.inline.js",
      "./goog/mocks/package.json",
      "./goog/mocks/index.js",
      "./node_modules/google-closure-library/package.json",
      "./node_modules/google-closure-library/**/*.js",
      "./node_modules/pako/package.json",
      "./node_modules/pako/dist/*.js",
    ],
    hide_warnings_for: ["./node_modules/pako/dist/pako.min.js"],

    jscomp_off: ["accessControls"],
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

  const closureCompiler = new ClosureCompiler(closureConfig);

  closureCompiler.javaPath = `${process.env.JAVA_HOME}/bin/java`;
  closureCompiler.JAR_PATH = JarPath;
  await new Promise((resolve, reject) => {
    const javaProcess = closureCompiler.run((exitCode, inputString, stderr) => {
      console.log(stderr);
      if (exitCode === 0) {
        resolve();
      } else {
        reject();
        process.exit(1);
      }
    });
  });
})();
