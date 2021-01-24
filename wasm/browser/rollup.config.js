import alias from "@rollup/plugin-alias";
import commonjs from "@rollup/plugin-commonjs";
import nodejsResolve from "@rollup/plugin-node-resolve";
import arraybufferPlugin from "./script/rollup-arraybuffer";
import inlineWebWorkerPlugin from "./script/inline-webworker";
import nodePolyfills from "rollup-plugin-node-polyfills";
import { terser } from "rollup-plugin-terser";
import babel from "@rollup/plugin-babel";
import pluginJson from "@rollup/plugin-json";
import replace from "@rollup/plugin-replace";
import { resolve } from "path";
import { readFileSync } from "fs";
import * as R from "ramda";

const RELEASE = process.env.BUILD_TARGET && process.env.BUILD_TARGET.startsWith("release");
const RELEASE_DEV = process.env.BUILD_TARGET === "release_dev";
const RELEASE_PROD = process.env.BUILD_TARGET === "release_prod";

const PROD = RELEASE_PROD || process.env.BUILD_TARGET === "production";

const DEV = RELEASE_DEV || process.env.BUILD_TARGET === "development";

const globals = {
  comlink: "Comlink",
  buffer: "buffer-es6",
  global: "window",
};

const pluginsCommon = [
  alias({
    entries: [
      { find: "@root", replacement: resolve("./src") },
      { find: "@module", replacement: resolve("./src/modules") },
      { find: "@utils", replacement: resolve("./src/utils") },
      { find: "path", replacement: require.resolve("path-browserify") },
      { find: "buffer", replacement: require.resolve("buffer-es6") },
      { find: "global", replacement: "window" },
    ],
  }),
  replace({
    __PROD__: PROD,
    __DEV__: DEV,
    __LOGGER__: DEV
      ? readFileSync("src/logger.development.js", "utf-8")
      : readFileSync("src/logger.production.js", "utf-8"),
  }),
  pluginJson(),
  nodejsResolve({ preferBuiltins: false }),
  commonjs({ transformMixedEsModules: false, ignoreGlobal: false }),
];

if (DEV) {
  pluginsCommon.push(
    nodePolyfills({
      fs: false,
      crypto: false,
      sourceMap: false,
      path: false,
      process: true,
      util: true,
    }),
  );
}

const babelCommon = babel({
  babelHelpers: "inline",
  include: "src/**/*.js",
  overrides: [
    {
      test: "./src/workers/*.worker.js",
      compact: true,
    },
  ],
  plugins: [
    ["@babel/plugin-transform-destructuring", { useBuiltIns: true }],
    ["@babel/plugin-proposal-object-rest-spread", { useBuiltIns: true }],
  ],
});

const polyfills = {
  fetch_noop: readFileSync("polyfills/fetch-noop.js", "utf-8"),
  set_timeout_noop: readFileSync("polyfills/set-timeout-noop.js", "utf-8"),
  text_encoding: readFileSync("polyfills/text-encoding.js", "utf-8"),
  performance: readFileSync("polyfills/performance.js", "utf-8"),
};

const fileExt = {
  module: ".esm.js",
  cjs: ".cjs.js",
  iife: ".iife.js",
};

const mainByType = (type) => ({
  input: "src/index.js",
  output: {
    intro: "let global = window;",
    file: DEV ? `dist/csound.dev${fileExt[type]}` : `dist/csound${fileExt[type]}`,
    format: type,
    sourcemap: DEV ? "inline" : false,
    exports: type === "module" ? "auto" : "named",
    name: "Csound",
    globals,
  },
  plugins: [
    ...pluginsCommon,
    ...(PROD ? [nodePolyfills()] : []),
    inlineWebWorkerPlugin({
      include: ["**/worklet.worker.js", "**/worklet.singlethread.worker.js"],
      dataUrl: true,
    }),
    inlineWebWorkerPlugin({
      include: ["**/sab.worker.js", "**/vanilla.worker.js", "**/old-spn.worker.js"],
      dataUrl: false,
    }),
    // R.assoc("plugins", R.append("add-module-exports", babelCommon.plugins), babelCommon),
    arraybufferPlugin({
      include: ["@csound/wasm-bin/lib/csound.dylib.wasm.z"],
    }),
    ...(PROD ? [terser({ module: type === "module" })] : []),
  ],
});

let mainConfiguration;

if (RELEASE) {
  mainConfiguration = [mainByType("module"), mainByType("cjs"), mainByType("iife")];
} else {
  mainConfiguration = [mainByType("module")];
}

export default [
  {
    input: "src/workers/sab.worker.js",
    output: {
      intro: "let global = typeof self !== 'undefined' ? self : window;",
      file: "dist/__compiled.sab.worker.js",
      format: "iife",
      sourcemap: DEV ? "inline" : false,
      sourcemapFile: "sab.worker.js",
      name: "sab.worker",
      globals,
    },
    plugins: [...pluginsCommon, babelCommon, ...(PROD ? [terser()] : [])],
  },
  {
    input: "src/workers/vanilla.worker.js",
    output: {
      intro: "let global = typeof self !== 'undefined' ? self : window;",
      file: "dist/__compiled.vanilla.worker.js",
      format: "iife",
      sourcemap: DEV ? "inline" : false,
      sourcemapFile: "vanilla.worker.js",
      name: "vanilla.worker",
      globals,
    },
    plugins: [...pluginsCommon, babelCommon, ...(PROD ? [terser()] : [])],
  },
  {
    input: "src/workers/worklet.worker.js",
    output: {
      intro: "let global = this;",
      file: "dist/__compiled.worklet.worker.js",
      format: "iife",
      sourcemap: DEV ? "inline" : false,
      sourcemapFile: "worklet.worker.js",
      name: "worklet.worker",
      globals,
    },
    plugins: [...pluginsCommon, babelCommon, ...(PROD ? [terser()] : [])],
  },
  {
    input: "src/workers/worklet.singlethread.worker.js",
    output: {
      intro: [
        "const global = AudioWorkletGlobalScope;",
        polyfills.fetch_noop,
        polyfills.set_timeout_noop,
        polyfills.text_encoding,
        polyfills.performance,
      ].join("\n"),
      file: "dist/__compiled.worklet.singlethread.worker.js",
      format: "es",
      sourcemap: DEV ? "inline" : false,
      name: "worklet.singlethread.worker",
      globals,
    },
    plugins: [
      alias({
        entries: {
          "@wasmer/wasm-transformer/lib/wasm-transformer.wasm":
            "@wasmer/wasm-transformer/lib/wasm-transformer.wasm",
          "@wasmer/wasm-transformer": resolve("./script/polyfilled-transformer.js"),
        },
      }),
      arraybufferPlugin({
        include: ["@wasmer/wasm-transformer/lib/wasm-transformer.wasm"],
      }),
      ...pluginsCommon,
      babelCommon,
      ...(PROD ? [terser()] : []),
    ],
  },
  {
    input: "src/workers/old-spn.worker.js",
    output: {
      intro: "let global = typeof self !== 'undefined' ? self : window;",
      file: "dist/__compiled.old-spn.worker.js",
      format: "iife",
      sourcemap: DEV ? "inline" : false,
      name: "old-spn.worker",
      globals,
    },
    plugins: [...pluginsCommon, babelCommon, ...(PROD ? [terser()] : [])],
  },
  {
    input: "src/libcsound.js",
    output: {
      intro: "// this file is intended for @csound/nodejs\n",
      file: "dist/factory.esm.js",
      format: "module",
      sourcemap: false,
      globals,
    },
    plugins: [...pluginsCommon],
  },
  ...mainConfiguration,
];
