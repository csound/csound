import fs from "fs";
import { spawn } from "child_process";
import MochaWebdriverRunner from "mocha-webdriver-runner";
const { runMochaWebDriverTest } = MochaWebdriverRunner;

const httpServerPs = spawn(`node tests/server.cjs`, {
  shell: true,
  env: { ...process.env, PORT: "8081" },
});

const webDriverCapabilities = {
  browserName: "chrome",
  "goog:chromeOptions": {
    args: [
      "--no-sandbox",
      "--headless",
      // https://stackoverflow.com/a/50642913/3714556
      "--disable-dev-shm-usage",
      "--auto-select-desktop-capture-source",
      "--disable-gesture-requirement-for-media-playback",
      "--autoplay-policy=no-user-gesture-required",
      "--disable-cache",
    ],
  },
};

const CI_BIN = process.env["CHROME_BIN"];
if (CI_BIN && fs.existsSync(CI_BIN)) {
  webDriverCapabilities["goog:chromeOptions"]["binary"] = CI_BIN;
}

(async function () {
  let result;
  await new Promise((resolve) => setTimeout(resolve, 1000));
  try {
    result = await runMochaWebDriverTest(
      webDriverCapabilities,
      "http://localhost:8081/index.html?ci=true",
      {
        reporter: "mocha-junit-reporter",
        reporterOptions: {
          mochaFile: "tests/results.junit.xml",
          useFullSuiteTitle: true,
          rootSuiteTitle: undefined,
          outputs: true,
        },
        captureConsoleLog: true,
      },
    );
  } catch (error) {
    console.error(error);
    process.exit(-1);
  }

  httpServerPs.kill();
  if (result && result.success) {
    process.exit(0);
  } else {
    console.error(JSON.stringify(result || {}, null, 2));
    process.exit(0);
  }
})();
