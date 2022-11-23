const path = require("path");
const fs = require("fs");

const constants = fs
  .readFileSync(path.resolve(__dirname, "../src/constants.js"))
  .toString();
const main = fs
  .readFileSync(path.resolve(__dirname, "../src/csound.worklet.js"))
  .toString();

fs.writeFileSync(
  path.resolve(__dirname, "../src/worklet.bundle.js"),
  `/* eslint-disable */
${constants}
${main}`
);
