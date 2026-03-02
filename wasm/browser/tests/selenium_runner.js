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
