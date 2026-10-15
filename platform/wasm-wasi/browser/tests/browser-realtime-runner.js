import { readFileSync, existsSync } from "node:fs";
import http from "node:http";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { Builder } from "selenium-webdriver";

const browserDir = fileURLToPath(new URL("..", import.meta.url));
const production = process.env.CSOUND_BROWSER_BUNDLE || path.join(browserDir, "dist/csound.js");
const probe = process.env.CSOUND_BROWSER_PROBE_BUNDLE;
if (!existsSync(production) || !probe || !existsSync(probe)) {
  throw new Error("Provide production and test-only probe browser bundles");
}
const routes = new Map([
  ["/", [path.join(browserDir, "tests/browser-realtime.html"), "text/html"]],
  ["/unisolated", [path.join(browserDir, "tests/browser-realtime.html"), "text/html"]],
  ["/dist/csound.js", [production, "text/javascript"]],
  ["/probe/csound.js", [probe, "text/javascript"]],
  [
    "/plugin_example.wasm",
    [path.join(browserDir, "../lib/plugin_example.wasm"), "application/wasm"],
  ],
  ...[
    "plugin_example_shared.wasm",
    "plugin_example_cpp_shared.wasm",
    "plugin_example_shared_missing_tls.wasm",
    "plugin_example_shared_trap_ctor.wasm",
  ].map((name) => [`/${name}`, [path.join(browserDir, "../lib", name), "application/wasm"]]),
]);
const server = http.createServer((request, response) => {
  const pathname = new URL(request.url, "http://localhost").pathname;
  if (pathname !== "/unisolated") {
    response.setHeader("Cross-Origin-Opener-Policy", "same-origin");
    response.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
  }
  const route = routes.get(pathname);
  if (!route) {
    response.writeHead(404, { "Content-Type": "text/html" });
    response.end("<!doctype html><title>404 Not Found</title>");
    return;
  }
  response.setHeader("Content-Type", route[1]);
  response.setHeader("Cache-Control", "no-store");
  response.end(readFileSync(route[0]));
});
await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
const browser = process.env.CSOUND_BROWSER_ENGINE || "chrome";
const capabilities =
  browser === "firefox"
    ? {
        browserName: "firefox",
        "moz:firefoxOptions": {
          args: ["--headless"],
          prefs: { "media.autoplay.default": 0, "media.autoplay.block-webaudio": false },
          ...(process.env.FIREFOX_BIN ? { binary: process.env.FIREFOX_BIN } : {}),
        },
      }
    : {
        browserName: "chrome",
        "goog:chromeOptions": {
          args: [
            "--headless",
            "--no-sandbox",
            "--disable-dev-shm-usage",
            "--autoplay-policy=no-user-gesture-required",
          ],
          ...(process.env.CHROME_BIN ? { binary: process.env.CHROME_BIN } : {}),
        },
      };
let driver;
try {
  driver = await new Builder().withCapabilities(capabilities).build();
  console.log(
    JSON.stringify({ browser, version: (await driver.getCapabilities()).get("browserVersion") }),
  );
  const pages = process.env.CSOUND_BROWSER_PAGES
    ? JSON.parse(process.env.CSOUND_BROWSER_PAGES)
    : [
        "/",
        "/?probe",
        "/?mode=singlethread",
        "/?mode=messageport",
        "/?mode=plugins",
        "/?mode=plugins&realtime",
        "/?mode=plugins&local",
        "/?mode=libcsound",
        "/?mode=bad-plugin",
        "/?mode=plugin-payloads",
        "/?mode=shared-context",
        "/?mode=shared-context&localFirst",
        "/unisolated?unsupported=isolation",
        "/unisolated?mode=singlethread",
        "/unisolated?mode=messageport",
        "/unisolated?mode=libcsound",
        "/?unsupported=wasm",
      ];
  for (const page of pages) {
    await driver.get(`http://127.0.0.1:${server.address().port}${page}`);
    const result = await driver.wait(
      () => driver.executeScript("return window.__csoundRealtimeResult || null"),
      120000,
      `Browser realtime checks did not finish: ${page}`,
    );
    console.log(JSON.stringify({ page, ...result }, null, 2));
    if (!result.ok) process.exitCode = 1;
  }
} finally {
  if (driver) {
    try {
      if (browser === "chrome")
        for (const entry of await driver.manage().logs().get("browser")) console.log(entry.message);
    } finally {
      await driver.quit();
    }
  }
  await new Promise((resolve) => server.close(resolve));
}
