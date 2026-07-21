import jsdoc2md from "jsdoc-to-markdown";
const title = `# @csound/browser`;
const npmShield = `[![npm (scoped with tag)](https://shields.shivering-isles.com/npm/v/@csound/browser/latest)](https://www.npmjs.com/package/@csound/browser)`;
const prettierShield = `[![styled with prettier](https://img.shields.io/badge/styled_with-prettier-ff69b4.svg)](https://github.com/prettier/prettier)`;
const workflowShield = `[![GitHub Workflow Status](https://shields.shivering-isles.com/github/workflow/status/csound/csound/csound_wasm)](https://github.com/csound/csound/actions?query=workflow%3Acsound_wasm)`;
const breakingChangesSection = `## Breaking Changes

### ScriptProcessorNode Support Removed (v7.0.0+)

ScriptProcessorNode support has been removed from the library. ScriptProcessorNode was deprecated in 2014 and provides inferior performance compared to AudioWorklet. All modern browsers (Chrome 64+, Firefox 76+, Safari 14.1+) support AudioWorklet.

**Migration:** If you were using \`useSPN: true\`, simply remove this parameter. The library will automatically use the superior AudioWorklet API.
`;
const microphoneInputSection = `## Microphone input

Browser microphone access requires a secure context. Use HTTPS for deployed
applications. For local development, use \`http://localhost\` or a loopback
address such as \`http://127.0.0.1\`. A computer name or LAN address served over
HTTP does not count as secure, even when it points to the same computer.

Set the Csound input option to \`-iadc\` before calling \`start()\`. Worker modes
detect this option and request microphone permission during startup. A direct
call to \`enableAudioInput()\` only applies to the single-thread mode; worker
modes ignore it. The returned promise rejects when the browser cannot provide
microphone access.

When Csound runs in an iframe, the parent page must also allow microphone
access through its Permissions Policy.

See the
[MediaDevices.getUserMedia documentation](https://developer.mozilla.org/en-US/docs/Web/API/MediaDevices/getUserMedia#privacy_and_security)
for browser security and permission rules.
`;
const apiDocTitle = `## Api Documentation\n\n`;
const licenseSection =
  `## License\n\n` +
  `\`@csound/browser\` is licensed under the [Apache License 2.0](./LICENSE).\n\n` +
  `> **Note:** The \`@csound/wasm-bin\` package (which contains the compiled Csound WebAssembly binary) ` +
  `remains licensed under the GNU Lesser General Public License v2.1 (LGPL-2.1), as required by the ` +
  `Csound core library it is built from. \`@csound/browser\` dynamically loads \`@csound/wasm-bin\` at ` +
  `runtime and is therefore not subject to LGPL copyleft requirements.\n`;
jsdoc2md
  .render({ files: "src/**/*.js" })
  .then((jsdocMarkdown) =>
    console.log(
      `${title}\n` +
        `${npmShield}\n` +
        `${workflowShield}\n` +
        `${prettierShield}\n` +
        `\n\n` +
        `${breakingChangesSection}\n` +
        `${microphoneInputSection}\n` +
        `${apiDocTitle}${jsdocMarkdown}` +
        `\n${licenseSection}`,
    ),
  );
