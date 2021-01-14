import fs from "fs";
import { spawn } from "child_process";
import MochaWebdriverRunner from "mocha-webdriver-runner";
const { runMochaWebDriverTest } = MochaWebdriverRunner;

const httpServerPs = spawn(`npx http-server ./tests -c-1 --no-dotfiles -p 8081`, { shell: true });

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
    ],
  },
};

const CI_BIN = process.env["CHROME_BIN"];
if (CI_BIN && fs.existsSync(CI_BIN)) {
  webDriverCapabilities["goog:chromeOptions"]["binary"] = CI_BIN;
}

(async function () {
  let result;
  try {
    result = await runMochaWebDriverTest(webDriverCapabilities, "http://localhost:8081/index.html");
  } catch (error) {
    console.error(error);
    process.exit(-1);
  }
  httpServerPs.kill();
  if (result && result.success) {
    process.exit(0);
  } else {
    process.exit(-1);
  }
})();
