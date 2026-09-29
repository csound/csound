/**
 * Vite config for the Csound UGEN AudioWorklet demo.
 *
 * The key challenge: AudioWorkletGlobalScope doesn't support import() or
 * ES module imports. The processor file must be a self-contained script.
 *
 * This config uses a custom Vite plugin that bundles src/processor.js
 * (which imports from csound.js) into a single IIFE via esbuild.
 * A banner injects polyfills so csound.js's DOM-dependent startup code
 * is safely skipped in the worklet scope.
 *
 * - Dev: middleware serves /processor.js as a fresh esbuild bundle per request
 * - Build: closeBundle hook emits dist/processor.js alongside the main app
 */

import { defineConfig } from "vite";
import { resolve, dirname } from "path";
import { fileURLToPath } from "url";
import { readFileSync } from "fs";
import { buildSync } from "esbuild";

const __dirname = dirname(fileURLToPath(import.meta.url));
const csoundDist = resolve(__dirname, "../../dist/csound.js");

// Polyfills prepended as a banner to the esbuild IIFE bundle so that
// csound.js (a Closure-compiled main-thread build) can execute safely
// inside an AudioWorkletGlobalScope.  See src/worklet-polyfills.js for details.
const workletPolyfills = readFileSync(
  resolve(__dirname, "src/worklet-polyfills.js"),
  "utf-8",
);

/** esbuild options shared by dev middleware and production build */
function processorBuildOptions(overrides = {}) {
  return {
    entryPoints: [resolve(__dirname, "src/processor.js")],
    bundle: true,
    format: "iife",
    banner: { js: workletPolyfills },
    alias: { "@csound/browser": csoundDist },
    ...overrides,
  };
}

function audioWorkletPlugin() {
  return {
    name: "bundle-audio-worklet",

    // Dev: serve the bundled processor on demand
    configureServer(server) {
      server.middlewares.use("/processor.js", (_req, res) => {
        const result = buildSync(
          processorBuildOptions({ write: false }),
        );
        res.setHeader("Content-Type", "application/javascript");
        res.end(result.outputFiles[0].text);
      });
    },

    // Production: emit the bundled processor alongside Vite's output
    closeBundle() {
      buildSync(
        processorBuildOptions({
          outfile: resolve(__dirname, "dist/processor.js"),
          minify: true,
        }),
      );
    },
  };
}

export default defineConfig({
  plugins: [audioWorkletPlugin()],
  server: {
    headers: {
      // Not strictly needed for this demo (no SharedArrayBuffer), but
      // included for compatibility if users experiment with SAB-based designs.
      "Cross-Origin-Opener-Policy": "same-origin",
      "Cross-Origin-Embedder-Policy": "require-corp",
    },
  },
});
