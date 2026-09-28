import fs from "node:fs";
import path from "node:path";
import { fork } from "node:child_process";
import { once } from "node:events";
import { fileURLToPath } from "node:url";
import { Builder } from "selenium-webdriver";

const testsDir = fileURLToPath(new URL(".", import.meta.url));

export async function runBrowserTests({
  capabilities,
  port,
  output,
  page = "/index.html?ci=true",
  timeout = 180000,
}) {
  // A failed run must not leave a previous report looking like a fresh result.
  fs.rmSync(output, { force: true });
  const server = fork(path.join(testsDir, "server.cjs"), [], {
    cwd: path.dirname(testsDir),
    env: { ...process.env, PORT: String(port) },
    stdio: ["ignore", "inherit", "inherit", "ipc"],
  });
  const ready = new Promise((resolve, reject) => {
    server.once("message", resolve);
    server.once("error", reject);
    server.once("exit", (code) => reject(new Error(`Test server exited before startup: ${code}`)));
  });
  let driver;
  try {
    await ready;
    driver = await new Builder().withCapabilities(capabilities).build();
    await driver.get(`http://localhost:${port}${page}`);
    const result = await driver.wait(
      () => driver.executeScript("return globalThis.__csoundTestResult || null"),
      timeout,
      "Browser tests did not produce a report",
    );
    if (!result.xml || !result.stats || result.stats.tests === 0) {
      throw new Error("Browser tests returned an empty report");
    }
    fs.mkdirSync(path.dirname(output), { recursive: true });
    fs.writeFileSync(output, result.xml);
    if (result.stats.failures > 0) console.error(result.xml);
    console.log(
      `${result.stats.passes} browser tests passed, ${result.stats.failures} failed: ${output}`,
    );
    return result.stats.failures === 0;
  } finally {
    try {
      if (driver) {
        try {
          // Chrome exposes worker and page errors through its browser log.
          if (capabilities.browserName === "chrome") {
            const entries = await driver.manage().logs().get("browser");
            for (const entry of entries) console.log(entry.message);
          }
        } finally {
          await driver.quit();
        }
      }
    } finally {
      if (server.exitCode === null && server.signalCode === null) {
        const exited = once(server, "exit");
        server.kill();
        await exited;
      }
    }
  }
}
