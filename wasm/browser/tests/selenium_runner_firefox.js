import fs from "fs";
import { spawn } from "child_process";
import MochaWebdriverRunner from "mocha-webdriver-runner";
const { runMochaWebDriverTest } = MochaWebdriverRunner;

const httpServerPs = spawn(`node tests/server.cjs`, {
  shell: true,
  env: { PATH: process.env.PATH, PORT: "8082" },
});

httpServerPs.stdout.on("data", (d) => console.log(d.toString()));
httpServerPs.stderr.on("data", (d) => console.log(d.toString()));

const webDriverCapabilities = {
  browserName: "firefox",
  "moz:firefoxOptions": {
    args: ["--no-sandbox", "--headless"],
  },
};

const CI_BIN = process.env["FIREFOX_BIN"];
if (CI_BIN && fs.existsSync(CI_BIN)) {
  webDriverCapabilities["moz:firefoxOptions"]["binary"] = CI_BIN;
}

(async function () {
  let result;
  await new Promise((resolve) => setTimeout(resolve, 1000));
  try {
    result = await runMochaWebDriverTest(
      webDriverCapabilities,
      "http://localhost:8082/index.html?ci=true",
      {
        reporter: "mocha-junit-reporter",
        reporterOptions: {
          mochaFile: "tests/results-firefox.junit.xml",
          useFullSuiteTitle: true,
          rootSuiteTitle: undefined,
        },
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
