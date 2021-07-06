import goog from "google-closure-compiler";
import JarPath from "google-closure-compiler-java";
import prettier from "prettier";
import { Readable } from "stream";
import { fileURLToPath } from "url";
import fs from "fs";
import path from "path";
import * as R from "ramda";
const { compiler: ClosureCompiler } = goog;
import { inlineWebworker } from "./goog/generate-webworker-module.js";
import { inlineArraybuffer } from "./goog/inline-arraybuffer.js";
// const ClosureCompiler = goog();

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = __dirname; //path.resolve(__dirname, ".");
const nodeModulesDir = path.join(rootDir, "node_modules");
const externsDir = path.join(rootDir, "externs");
const srcDir = path.join(rootDir, "src");
const googDir = path.join(rootDir, "goog");

function trimString(a) {
  return a
    .split("\n")
    .map((line) => line.trim())
    .filter((line) => Boolean(line))
    .join("\n");
}

// console.log(Object.keys(ClosureCompiler), ClosureCompiler, typeof ClosureCompiler);
// npx google-closure-compiler --js=dist/csound.esm.js --platform=java --compilation_level=SIMPLE_OPTIMIZATIONS --language_in=ECMASCRIPT_NEXT --assume_function_wrapper=true --transform_amd_modules=true --module_resolution=NODE --js='node_modules/buffer-es6/index.js' > out.js

const monkeyPatches = (code) => {
  return R.pipe(R.replace("cwd = process.cwd();", "cwd = '/';"))(code);
};

// const prettyCode = prettier.format(monkeyPatches(code), {
//   trailingComma: "es5",
//   arrowParens: "always",
//   parser: "babel",
// });
//
// const tmpInFileName = path.join(rootDir, "dist", chunk.fileName.replace(/\.js$/g, ".nomin.js"));
const inputFile = path.join(srcDir, "workers/sab.worker.js");
const tmpOutFileName = path.join(rootDir, "dist", "test.min.js");

if (!fs.existsSync(path.join(rootDir, "dist"))) {
  fs.mkdirSync(path.join(rootDir, "dist"));
}

if (!fs.existsSync(path.join(rootDir, "dist", "package.json"))) {
  const fakePackageJson = `{
    "name": "dist",
    "version": "0.0.0",
    "description": "this file is only a temporary transient file to fool closure compiler, plz ignore",
  }`;
  fs.writeFileSync(path.join(rootDir, "dist", "package.json"), fakePackageJson);
}

fs.writeFileSync(
  path.join(rootDir, "dist", "__csound_wasm.inline.js"),
  inlineArraybuffer("./node_modules/@csound/wasm-bin/lib/csound.dylib.wasm.z", "binary.wasm"),
);

fs.writeFileSync(
  path.join(rootDir, "dist", "__wasmer_wasm_transformer.inline.js"),
  inlineArraybuffer(
    "./node_modules/@wasmer/wasm-transformer/lib/wasm-transformer.wasm",
    "transformer.wasm",
  ),
);

const polyfills = {
  fetch_noop: fs.readFileSync("polyfills/fetch-noop.js", "utf-8"),
  set_timeout_noop: fs.readFileSync("polyfills/set-timeout-noop.js", "utf-8"),
  text_encoding: fs.readFileSync("polyfills/text-encoding.js", "utf-8"),
  performance: fs.readFileSync("polyfills/performance.js", "utf-8"),
};

const compilationSequence = [
  {
    entry_point: "./src/workers/sab.worker",
    js_output_file: path.join(rootDir, "dist", "__compiled.sab.worker.js"),
    create_source_map: path.join(rootDir, "dist", "__compiled.sab.worker.js.map"),
    source_map_location_mapping: "./src|/dist/src",
    postbuild: () => inlineWebworker("sab.worker.js", "worker.sab", false, []),
    output_wrapper: trimString(`(function(){%output%}).call(this);
                    //# sourceMappingURL=/dist/__compiled.sab.worker.js.map`),
  },
  {
    entry_point: "./src/workers/vanilla.worker.js",
    js_output_file: path.join(rootDir, "dist", "__compiled.vanilla.worker.js"),
    create_source_map: path.join(rootDir, "dist", "__compiled.vanilla.worker.js.map"),
    source_map_location_mapping: "./src|/dist/src",
    postbuild: () => inlineWebworker("vanilla.worker.js", "worker.vanilla", false, []),
    output_wrapper: trimString(`(function(){%output%}).call(this);
                    //# sourceMappingURL=/dist/__compiled.vanilla.worker.js.map`),
  },
  {
    entry_point: "./src/workers/worklet.worker.js",
    js_output_file: path.join(rootDir, "dist", "__compiled.worklet.worker.js"),
    create_source_map: path.join(rootDir, "dist", "__compiled.worklet.worker.js.map"),
    source_map_location_mapping: "./src|/dist/src",
    // define: ["WITH_TEXT_ENCODER_POLYFILL"],
    output_wrapper: trimString(`
    let self = AudioWorkletGlobalScope;
    %output%
    //# sourceMappingURL=/dist/__compiled.worklet.worker.js.map`),
    postbuild: () => inlineWebworker("worklet.worker.js", "worker.worklet", true, []),
    // postbuild: () => {
    //   // monkey patch away non existing self
    //   const loc = path.join(rootDir, "dist", "__compiled.worklet.worker.js");
    //   fs.writeFileSync(
    //     loc,
    //     fs
    //       .readFileSync(loc)
    //       .toString()
    //       .replace("global=this||self", "global=AudioWorkletGlobalScope")
    //       .replace(/TextDecoder/g, "this.TextDecoder")
    //       .replace(/TextEncoder/g, "this.TextEncoder")
    //   );
    //   inlineWebworker("worklet.worker.js", "worker.worklet", true, []);
    // },
  },
  {
    entry_point: "./src/workers/worklet.singlethread.worker.js",
    js_output_file: path.join(rootDir, "dist", "__compiled.worklet.singlethread.worker.js"),
    create_source_map: path.join(rootDir, "dist", "__compiled.worklet.singlethread.worker.js.map"),
    source_map_location_mapping:
      path.join(rootDir, "dist", "__compiled.worklet.singlethread.worker.js") +
      "|" +
      "dist/src/workers/worklet.singlethread.worker.js",
    define: ["WITH_TEXT_ENCODER_POLYFILL"],
    output_wrapper: trimString(`
    let self = AudioWorkletGlobalScope;
    %output%
    //# sourceMappingURL=/dist/__compiled.worklet.singlethread.worker.js.map`),
    postbuild: () =>
      inlineWebworker("worklet.singlethread.worker.js", "worklet.singlethread.worker", true, []),
    // postbuild: () => {
    //   // monkey patch away non existing self
    //   const loc = path.join(rootDir, "dist", "__compiled.worklet.singlethread.worker.js");
    //   fs.writeFileSync(
    //     loc,
    //     fs
    //       .readFileSync(loc)
    //       .toString()
    //       // .replace("global=this||self", "global=AudioWorkletGlobalScope")
    //       .replace(/TextDecoder/g, "goog.global.TextDecoder")
    //       .replace(/TextEncoder/g, "goog.global.TextEncoder")
    //   );
    //   inlineWebworker("worklet.singlethread.worker.js", "worklet.singlethread.worker", true, []);
    // },
  },
  {
    entry_point: "./src/workers/old-spn.worker.js",
    js_output_file: path.join(rootDir, "dist", "__compiled.old-spn.worker.js"),
    create_source_map: path.join(rootDir, "dist", "__compiled.old-spn.worker.js.map"),
    source_map_location_mapping: "./src|/dist/src",
    // source_map_input: [`./src/index.js|/dist/__compiled.old-spn.worker.sourcemaps.js`],
    // isolation_mode: "IIFE",
    output_wrapper: trimString(`(function(){%output%}).call(this);
                    //# sourceMappingURL=/dist/__compiled.old-spn.worker.js.map`),
    postbuild: () => inlineWebworker("old-spn.worker.js", "worker.old_spn", false, []),
  },
  {
    entry_point: "./src/index.js",
    js_output_file: path.join(rootDir, "dist", "csound.js"),
    create_source_map: path.join(rootDir, "dist", "csound.js.map"),
    // source_map_location_mapping: "./src|./dist",
    // source_map_location_mapping: "./dist/dist|./dist",
    // js: [path.join(rootDir, "dist", "package.json")],
    output_manifest: "output.manifest.txt",
    output_wrapper: trimString(`%output%
      const Csound = Csound$$module$src$index;
      Csound.toString = () => 'async (options) => CsoundObj;';
      export { Csound }
      export default Csound
      //# sourceMappingURL=/dist/csound.js.map`),
  },
];

function deepMerge(v1, v2) {
  if (Array.isArray(v1) && Array.isArray(v2)) {
    return R.uniq(R.concat(v1, v2));
  } else if (typeof v1 === "object" && typeof v2 === "object" && !R.isNil(v1) && !R.isNil(v2)) {
    return R.mergeWith(deepMerge, v1, v2);
  } else {
    return v2;
  }
}

const compile = async (config) => {
  console.log("Compiling", config.entry_point, "\n");
  const defaultConfig = {
    browser_featureset_year: 2020,
    externs: [path.join(rootDir, "externs.js")],
    js: [
      "./src/*.js",
      "./src/**/*.js",
      "./dist/*inline.js",
      "./goog/mocks/package.json",
      "./goog/mocks/index.js",
      "./node_modules/lines-logger/package.json",
      "./node_modules/lines-logger/lib/browser.js",
      "./node_modules/lines-logger/lib/index.js",
      "./node_modules/rambda/package.json",
      "./node_modules/rambda/dist/rambda.esm.js",
      "./node_modules/path-browserify/package.json",
      "./node_modules/path-browserify/index.js",
      // "./node_modules/@wasmer/wasi/package.json",
      // "./node_modules/@wasmer/wasi/lib/index.esm.js",
      "./node_modules/@wasmer/wasm-transformer/package.json",
      "./node_modules/@wasmer/wasm-transformer/lib/unoptimized/wasm-transformer.esm.js",
      "./node_modules/comlink/package.json",
      "./node_modules/comlink/dist/esm/comlink.mjs",
      "./node_modules/zlibjs/bin/inflate.min.js",
      "./node_modules/unmute-ios-audio/package.json",
      "./node_modules/unmute-ios-audio/index.js",
      "./node_modules/eventemitter3/umd/eventemitter3.min.js",
      "./node_modules/google-closure-library/package.json",
      "./node_modules/google-closure-library/closure/goog/base.js",
      "./node_modules/google-closure-library/closure/goog/array/array.js",
      "./node_modules/google-closure-library/closure/goog/asserts/asserts.js",
      "./node_modules/google-closure-library/closure/goog/debug/*.js",
      "./node_modules/google-closure-library/closure/goog/disposable/*.js",
      "./node_modules/google-closure-library/closure/goog/useragent/*.js",
      "./node_modules/google-closure-library/closure/goog/reflect/*.js",
      "./node_modules/google-closure-library/closure/goog/dom/*.js",
      "./node_modules/google-closure-library/closure/goog/html/*.js",
      "./node_modules/google-closure-library/closure/goog/i18n/*.js",
      "./node_modules/google-closure-library/closure/goog/fs/*.js",
      "./node_modules/google-closure-library/closure/goog/functions/*.js",
      "./node_modules/google-closure-library/closure/goog/labs/*.js",
      "./node_modules/google-closure-library/closure/goog/labs/useragent/*.js",
      "./node_modules/google-closure-library/closure/goog/object/*.js",
      "./node_modules/google-closure-library/closure/goog/string/*.js",
      "./node_modules/google-closure-library/closure/goog/async/*.js",
      "./node_modules/google-closure-library/closure/goog/promise/*.js",
      "./node_modules/google-closure-library/closure/goog/math/*.js",
      "./node_modules/google-closure-library/closure/goog/db/*.js",
      "./node_modules/google-closure-library/third_party/closure/goog/mochikit/async/*.js",
      "./node_modules/google-closure-library/closure/goog/events/*.js",
      // "./node_modules/buffer-es6/package.json",
      // "./node_modules/buffer-es6/index.js",
      // "./node_modules/buffer-es6/base64.js",
      // "./node_modules/buffer-es6/ieee754.js",
      // "./node_modules/buffer-es6/isArray.js",
    ],
    hide_warnings_for: [
      // "./node_modules/@wasmer/wasi/lib/index.esm.js",
      // "./node_modules/buffer-es6/index.js",
    ],
    // entry_point: "./src/workers/sab.worker",
    // entry_point: "sab.worker",
    assume_function_wrapper: true,
    // isolation_mode: "NONE",
    // compilation_level: "ADVANCED",
    compilation_level: "SIMPLE_OPTIMIZATIONS",
    // compilation_level: "WHITESPACE_ONLY",
    // compilation_level: "BUNDLE",
    language_in: "ECMASCRIPT_2020",
    language_out: "ECMASCRIPT_2020",
    process_common_js_modules: true,
    rewrite_polyfills: false,
    module_resolution: "NODE",
    dependency_mode: "PRUNE",
    // force_inject_library: "base",
    // js_module_root: "./goog/mocks",
    // js_output_file: tmpOutFileName,
    // define: "\"__LOGGER__=export * from './logger.production';\"",
    debug: false,
    source_map_include_content: true,
  };
  const closureCompiler = new ClosureCompiler(deepMerge(defaultConfig, config));

  closureCompiler.javaPath = `${process.env.JAVA_HOME}/bin/java`;
  closureCompiler.JAR_PATH = JarPath;
  await new Promise((resolve, reject) => {
    const javaProcess = closureCompiler.run((exitCode, inputString, stderr) => {
      console.log(stderr);
      if (exitCode === 0) {
        resolve();
      } else {
        reject();
        process.exit(0);
      }
    });
  });
};

(async () => {
  for (const compileInfo of compilationSequence) {
    const postbuild = R.prop("postbuild", compileInfo);
    const closureData = R.dissoc("postbuild", compileInfo);
    await compile(closureData);
    postbuild && postbuild();
  }
})();

// closureCompiler.spawnOptions = { stdio: "inherit" };

// let outputResolver, outputRejecter;
// const codePromise = new Promise((resolve, reject) => {
//   outputResolver = resolve;
//   outputRejecter = reject;
// });
//

// const closureCompiler = new ClosureCompiler({
//   js: [
//     path.join(rootDir, "dist/csound.cjs.js"),
//     // path.join(nodeModulesDir, "assert/build/assert.js"),
//     path.join(nodeModulesDir, "buffer-es6/index.js"),
//     path.join(nodeModulesDir, "buffer-es6/base64.js"),
//     path.join(nodeModulesDir, "buffer-es6/ieee754.js"),
//     path.join(nodeModulesDir, "buffer-es6/isArray.js"),
//   ],
//   externs: [path.join(rootDir, "externs.js")],
//   // entry_point: path.join(rootDir, "dist/csound.esm.js"),
//
//   compilation_level: "ADVANCED",
//   // compilation_level: "SIMPLE_OPTIMIZATIONS",
//   transform_amd_modules: true,
//   module_resolution: "NODE",
//   js_output_file: "out.js",
//   debug: true,
// });
//
// console.log(closureCompiler, Object.keys(closureCompiler));

// closureCompiler.javaPath = `${process.env.JAVA_HOME}/bin/java`;
// closureCompiler.JAR_PATH = JarPath;
// closureCompiler.spawnOptions = { stdio: "inherit" };
//
// closureCompiler.run((exitCode, outputCode, stderr) => {
//   console.log(stdout);
//   console.log(stderr);
//   process.exitCode = exitCode;
// });
