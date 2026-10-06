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
  browserName: "firefox",
  "moz:firefoxOptions": {
    args: ["--no-sandbox", "--headless"],
    prefs: {
      "media.navigator.streams.fake": true,
      "media.navigator.permission.disabled": true,
      "media.autoplay.default": 0,
      "media.autoplay.block-webaudio": false,
    },
  },
};

const CI_BIN = process.env["FIREFOX_BIN"];
if (CI_BIN && fs.existsSync(CI_BIN)) {
  webDriverCapabilities["moz:firefoxOptions"]["binary"] = CI_BIN;
}

try {
  const success = await runBrowserTests({
    capabilities: webDriverCapabilities,
    port: 8082,
    output: "tests/FIREFOX.junit.xml",
  });
  process.exitCode = success ? 0 : 1;
} catch (error) {
  console.error(error);
  process.exitCode = 1;
}
