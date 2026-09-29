import { readFileSync, writeFileSync } from "fs";
import { basename, resolve } from "path";

export function inlineWebworker(filename, modulename, asDataUrl = true) {
  const inputFile = resolve("./dist", `__compiled.${filename}`);
  writeFileSync(
    `./dist/__compiled.${filename.replace(/\.js$/g, ".inline.js")}`,
    asDataUrl
      ? `export default () => "data:application/javascript;base64,${readFileSync(inputFile, {
          encoding: "base64",
        })}";
            `
      : `export default () => (window.URL || window.webkitURL).createObjectURL(new Blob([${JSON.stringify(
          readFileSync(inputFile).toString("utf8"),
        )}]));`,
  );
}
