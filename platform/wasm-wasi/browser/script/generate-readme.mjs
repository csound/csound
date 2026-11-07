import jsdoc2md from "jsdoc-to-markdown";
const title = `# @csound/browser`;
const engineSection = `\`@csound/browser\` uses wasm32 with float samples and WASI Preview 2 by default.
To opt into true 64-bit pointers and double samples, import \`Csound\` or
\`libcsound\` from \`@csound/browser/wasm64.js\`. Each module bundles one engine;
there is no automatic switch to wasm64. The opt-in module requires memory64,
table64 and multiple-memory support. Use Node.js 24 or newer for builds.
See [the build guide](../README.md) for outputs and host requirements.
`;
const npmShield = `[![npm (scoped with tag)](https://shields.shivering-isles.com/npm/v/@csound/browser/latest)](https://www.npmjs.com/package/@csound/browser)`;
const prettierShield = `[![styled with prettier](https://img.shields.io/badge/styled_with-prettier-ff69b4.svg)](https://github.com/prettier/prettier)`;
const workflowShield = `[![GitHub Workflow Status](https://shields.shivering-isles.com/github/workflow/status/csound/csound/csound_wasm)](https://github.com/csound/csound/actions?query=workflow%3Acsound_wasm)`;
const breakingChangesSection = `## Breaking Changes

### Startup errors

\`Csound()\` now rejects its promise when AudioWorklet support is missing;
it previously logged an error and returned \`undefined\`. Catch startup errors
around \`await Csound()\` instead of checking its result for \`undefined\`.

### ScriptProcessorNode Support Removed (v7.0.0+)

ScriptProcessorNode support has been removed from the library. ScriptProcessorNode was deprecated in 2014 and provides inferior performance compared to AudioWorklet. All modern browsers (Chrome 64+, Firefox 76+, Safari 14.1+) support AudioWorklet.

**Migration:** If you were using \`useSPN: true\`, simply remove this parameter. The library will automatically use the superior AudioWorklet API.
`;
const iosSilentModeSection = `## iOS Silent mode

iOS can mute Web Audio when Silent mode is on. Safari on iOS 17 and later can
[use a playback audio session](https://bugs.webkit.org/show_bug.cgi?id=237322#c6)
to keep it audible. Run this on the main thread from your Play button handler,
before \`await Csound()\` or before creating a custom \`AudioContext\`:

\`\`\`js
try {
  const session = navigator.audioSession;
  if (session && session.type !== "play-and-record") {
    session.type = "playback";
  }
} catch {
  // Keep playback available if the browser rejects this optional setting.
}
\`\`\`

This does not bypass autoplay rules or a media volume of zero. On browsers
without this API, ask the user to turn Silent mode off. For microphone input,
use \`"play-and-record"\` before \`enableAudioInput()\` or starting with \`-iadc\`.

Audio sessions affect the whole page. Save the previous type and restore it
when playback ends or startup fails, only if another audio user has not changed it.
`;
const microphoneInputSection = `## Microphone input

Browser microphone access requires a secure context. Use HTTPS for deployed
applications. For local development, use \`http://localhost\` or a loopback
address such as \`http://127.0.0.1\`. A computer name or LAN address served over
HTTP does not count as secure, even when it points to the same computer.

Call \`await csound.enableAudioInput()\` before \`start()\` to set the Csound
input option, request microphone permission, and prepare the input stream. This
works in single-thread, worker, and SharedArrayBuffer modes. The returned
promise rejects when the browser cannot provide microphone access.

You can instead set the Csound input option to \`-iadc\` in a CSD or with
\`setOption()\`. Each mode detects this option and requests microphone access
during startup. Use \`enableAudioInput()\` when the application needs to handle
permission errors before it starts Csound.

When Csound runs in an iframe, the parent page must also allow microphone
access through its Permissions Policy.

See the
[MediaDevices.getUserMedia documentation](https://developer.mozilla.org/en-US/docs/Web/API/MediaDevices/getUserMedia#privacy_and_security)
for browser security and permission rules.
`;
const realtimeSection = `## Realtime init

Both published Preview 2 engines use synchronous init. \`supportsRealtime()\`
returns 0; \`--realtime\` cannot start a native background init thread with the
current toolchain.

AudioWorklet playback and worker playback still work. \`useWorker\` defaults to
\`false\`; set it to \`true\` to run Csound in a worker. \`useSAB: true\` uses shared
audio transport when available, while \`useSAB: false\` uses MessagePort transport.
Neither option changes the selected engine or enables native Csound threads.

Private thread fixtures remain for future toolchain support. See the
[build guide](../README.md#browser-playback-and-threads) for details.

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
      `${title}\n\n${engineSection}\n` +
        `${npmShield}\n` +
        `${workflowShield}\n` +
        `${prettierShield}\n` +
        `\n\n` +
        `${breakingChangesSection}\n` +
        `${iosSilentModeSection}\n` +
        `${microphoneInputSection}\n` +
        `${realtimeSection}` +
        `${apiDocTitle}${jsdocMarkdown}` +
        `\n${licenseSection}`,
    ),
  );
