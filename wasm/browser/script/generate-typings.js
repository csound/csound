import { execSync } from "child_process";
import fs from "fs";

const prepend = `import { IFs } from "memfs";`;

execSync(
  "jsdoc -c ./.jsdoc.conf.json -t node_modules/tsd-jsdoc/dist -r src -d .; mv types.d.ts index.d.ts",
);

const file = fs
  .readFileSync("index.d.ts")
  .toString()
  .replace("declare namespace CsoundObj", "declare interface CsoundObj");
fs.writeFileSync("index.d.ts", `${prepend}\n${file}`);
