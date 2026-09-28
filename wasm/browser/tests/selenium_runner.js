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

import fs from "node:fs";
import { runBrowserTests } from "./webdriver-runner.js";

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
      // Use a synthetic microphone without a permission prompt or real hardware.
      "--use-fake-device-for-media-stream",
      "--use-fake-ui-for-media-stream",
      "--disable-cache",
    ],
  },
};

const CI_BIN = process.env["CHROME_BIN"];
if (CI_BIN && fs.existsSync(CI_BIN)) {
  webDriverCapabilities["goog:chromeOptions"]["binary"] = CI_BIN;
}

try {
  const success = await runBrowserTests({
    capabilities: webDriverCapabilities,
    port: 8081,
    output: "tests/GOOGLE_CHROME.junit.xml",
  });
  process.exitCode = success ? 0 : 1;
} catch (error) {
  console.error(error);
  process.exitCode = 1;
}
