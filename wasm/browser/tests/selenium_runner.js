// import util from "util";
import { spawn } from "child_process";
// const spawnAsync = util.promisify(spawn);
// import Webdriver from "selenium-webdriver";
// const { Builder, By, until } = Webdriver;
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

(async function () {
  const result = await runMochaWebDriverTest(
    webDriverCapabilities,
    "http://localhost:8081/index.html",
  );
  httpServerPs.kill();
  if (result.success) {
    process.exit(-1);
  } else {
    process.exit(0);
  }
})();
