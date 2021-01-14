import alias from "@rollup/plugin-alias";
import commonjs from "@rollup/plugin-commonjs";
import nodejsResolve from "@rollup/plugin-node-resolve";
import arraybufferPlugin from "./script/rollup-arraybuffer";
import inlineWebWorkerPlugin from "./script/inline-webworker";
import nodePolyfills from "rollup-plugin-node-polyfills";
import { terser } from "rollup-plugin-terser";
// import strip from "@rollup/plugin-strip";
import babel from "@rollup/plugin-babel";
import pluginJson from "@rollup/plugin-json";
import replace from "@rollup/plugin-replace";
import { resolve } from "path";
import { readFileSync } from "fs";
import * as R from "ramda";

const PROD = process.env.BUILD_TARGET === "production";

const DEV = process.env.BUILD_TARGET === "development";

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
      ? readFileSync("src/logger.dev.js", "utf-8")
      : readFileSync("src/logger.prod.js", "utf-8"),
  }),
  // strip({
  //   include: ["src/**/*.js"],
  //   debugger: true,
  //   exclude: !PROD
  //     ? []
  //     : [
  //         "./src/logger.js",
  //         "@root/logger",
  //         "ololog",
  //         "ansicolor",
  //         "pipez",
  //         "printable-characters",
  //         "stacktracey",
  //         "string.bullet",
  //         "string.ify",
  //       ],
  //   functions: !PROD
  //     ? []
  //     : [
  //         "_log",
  //         "log",
  //         "logWorklet",
  //         "logVAN",
  //         "logWorkletWorker",
  //         "logSPN",
  //         "logSABMain",
  //         "logSABWorker",
  //         "logIndex",
  //       ],
  // }),
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

const workletPolyfill = readFileSync("src/workers/worklet.polyfill.js", "utf-8");

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
      intro: workletPolyfill,
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
    input: "src/index.js",
    output: {
      intro: "let global = window;",
      file: DEV ? "dist/libcsound.dev.mjs" : "dist/libcsound.mjs",
      format: "module",
      sourcemap: DEV ? "inline" : false,
      globals,
    },
    plugins: [
      ...pluginsCommon,
      inlineWebWorkerPlugin({
        include: ["**/worklet.worker.js", "**/worklet.singlethread.worker.js"],
        dataUrl: true,
      }),
      inlineWebWorkerPlugin({
        include: ["**/sab.worker.js", "**/vanilla.worker.js", "**/old-spn.worker.js"],
        dataUrl: false,
      }),
      R.assoc("plugins", R.append("add-module-exports", babelCommon.plugins), babelCommon),
      arraybufferPlugin({
        include: ["@csound/wasm/lib/libcsound.wasm.zlib"],
      }),
      ...(PROD ? [terser()] : []),
    ],
  },
];
