import { readFileSync, writeFileSync } from "fs";
import { basename, resolve } from "path";

export function inlineWebworker(filename, modulename, asDataUrl = true) {
  const inputFile = resolve("./dist", `__compiled.${filename}`);
  writeFileSync(
    `./dist/__compiled.${filename.replace(/\.js$/g, ".inline.js")}`,
    asDataUrl
      ? `goog.provide("${modulename}");
         goog.scope(function () {
             ${modulename} = () => "data:application/javascript;base64,${readFileSync(inputFile, {
          encoding: "base64",
        })}";
          })
            `
      : `goog.provide("${modulename}");
         goog.scope(function () {
         ${modulename} = () => (window.URL || window.webkitURL).createObjectURL(new Blob([${JSON.stringify(
          readFileSync(inputFile).toString("utf8"),
        )}]));
         })`,
  );
}
