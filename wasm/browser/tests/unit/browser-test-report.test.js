import assert from "node:assert/strict";
import Mocha from "mocha";
import { runWithXmlReport } from "../browser-test-report.js";

describe("browser XML report", () => {
  it("reports passing, failing and pending tests with escaped XML", async () => {
    const mocha = new Mocha();
    const suite = Mocha.Suite.create(mocha.suite, 'suite <&"');
    suite.addTest(new Mocha.Test("passes <&", () => {}));
    suite.addTest(
      new Mocha.Test("fails <&", () => {
        throw new Error("failure <&");
      }),
    );
    suite.addTest(new Mocha.Test("pending <&"));

    const { stats, xml } = await runWithXmlReport(mocha, Mocha.reporters.XUnit, "Browser <&");

    assert.equal(stats.tests, 3);
    assert.equal(stats.passes, 1);
    assert.equal(stats.failures, 1);
    assert.equal(stats.pending, 1);
    assert.match(xml, /name="Browser &#x3C;&#x26;"/);
    assert.match(xml, /name="passes &#x3C;&#x26;"/);
    assert.match(xml, /<failure/);
    assert.match(xml, /failure &#x3C;&#x26;/);
    assert.match(xml, /<skipped\/>/);
    assert.equal((xml.match(/<testcase /g) || []).length, 3);
  });
});
