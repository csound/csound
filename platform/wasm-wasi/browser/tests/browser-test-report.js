// Use Mocha's own XML formatter in the browser, without a version-specific bridge.
export function runWithXmlReport(mocha, XUnit, suiteName) {
  const lines = [];
  class BrowserReporter extends XUnit {
    write(line) {
      lines.push(line);
    }
  }
  mocha.reporter(BrowserReporter, { suiteName });
  return new Promise((resolve) => {
    const runner = mocha.run(() => {
      resolve({ stats: runner.stats, xml: `${lines.join("\n")}\n` });
    });
  });
}
