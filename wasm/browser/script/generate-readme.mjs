import jsdoc2md from "jsdoc-to-markdown";
const title = `# @csound/browser`;
const npmShield = `[![npm (scoped with tag)](https://shields.shivering-isles.com/npm/v/@csound/browser/latest)](https://www.npmjs.com/package/@csound/browser)`;
const prettierShield = `[![styled with prettier](https://img.shields.io/badge/styled_with-prettier-ff69b4.svg)](https://github.com/prettier/prettier)`;
const workflowShield = `[![GitHub Workflow Status](https://shields.shivering-isles.com/github/workflow/status/csound/csound/csound_wasm)](https://github.com/csound/csound/actions?query=workflow%3Acsound_wasm)`;
const apiDocTitle = `## Api Documentation\n\n`;
jsdoc2md
  .render({ files: "src/**/*.js" })
  .then((jsdocMarkdown) =>
    console.log(
      `${title}\n` +
        `${npmShield}\n` +
        `${workflowShield}\n` +
        `${prettierShield}\n` +
        `\n\n` +
        `${apiDocTitle}${jsdocMarkdown}`,
    ),
  );
