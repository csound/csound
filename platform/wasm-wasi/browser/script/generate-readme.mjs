import jsdoc2md from "jsdoc-to-markdown";
const title = `# @csound/browser`;
const npmShield = `[![npm (scoped with tag)](https://shields.shivering-isles.com/npm/v/@csound/browser/latest)](https://www.npmjs.com/package/@csound/browser)`;
const prettierShield = `[![styled with prettier](https://img.shields.io/badge/styled_with-prettier-ff69b4.svg)](https://github.com/prettier/prettier)`;
const workflowShield = `[![GitHub Workflow Status](https://shields.shivering-isles.com/github/workflow/status/csound/csound/csound_wasm)](https://github.com/csound/csound/actions?query=workflow%3Acsound_wasm)`;
const breakingChangesSection = `## Breaking Changes

### Default host and startup errors

On pages with cross-origin isolation and the required Wasm features, \`Csound()\`
now selects the shared worker host. Previously it used one AudioWorklet.
Pass \`useWorker: false\` to keep the previous execution model. The shared host
freezes plugins at its first start and uses the realtime API limits only when
\`--realtime\` is enabled. Pages without isolation still use one AudioWorklet.

\`Csound()\` now rejects its promise when AudioWorklet support is missing;
it previously logged an error and returned \`undefined\`. Catch startup errors
around \`await Csound()\` instead of checking its result for \`undefined\`.

### ScriptProcessorNode Support Removed (v7.0.0+)

ScriptProcessorNode support has been removed from the library. ScriptProcessorNode was deprecated in 2014 and provides inferior performance compared to AudioWorklet. All modern browsers (Chrome 64+, Firefox 76+, Safari 14.1+) support AudioWorklet.

**Migration:** If you were using \`useSPN: true\`, simply remove this parameter. The library will automatically use the superior AudioWorklet API.
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

Normal builds include thread support when the toolchain supports it. \`Csound()\`
uses the shared worker host when the page has isolation headers and the browser
supports it. Otherwise, it uses the same engine in one AudioWorklet with
synchronous init. Explicit \`useWorker: true\` requires the shared host unless
\`useSAB: false\` selects MessagePort mode; missing features cause an error
before startup.

On the shared worker host, set \`--realtime\` before \`start()\` to enable
background init; omitting the flag keeps synchronous init. Reset clears it.

During realtime playback, \`compileOrc()\`, \`evalCode()\`, and string channel APIs
run on a separate Csound thread. Their promises resolve when each call finishes;
compilation includes merging and global init. Existing voices and the sample
clock continue, but long global init can delay new notes because it shares
Csound's init mutex. This does not guarantee zero underruns.

Numeric channels declared before \`start()\` use atomic access to saved addresses.
Channels created later use the control thread. Numeric notes (\`event(0,
[instrument, start, duration, ...])\`) and end events (\`event(2, [])\` or
\`event(2, [0, delay])\`) remain available. The end-event promise confirms admission;
wait for \`realtimePerformanceEnded\` for completed shutdown.

The control queue holds four requests, including the running call, with at most
1 MiB of UTF-8 text per request. Stop rejects queued work and waits for running
work before cleanup. Resume before submitting engine calls while paused.
Filesystem APIs, score text, table changes, and full CSD compilation still
require a stop. Background workers cannot open the render worker's files, so
live \`#include\` and file-dependent global init remain unsupported even when
files are preloaded.

The bundle contains one binary for all APIs. Explicit \`useWorker: false\`,
\`useSAB: false\`, and the named \`libcsound()\` constructor use it locally with
synchronous init. They need shared Wasm support but no isolation headers.

Plugins work with the shared engine. Rebuild them with the matching plugin
SDK, then load them before the first \`start()\`. The shared worker host keeps
that plugin set across reset; create a new instance to change it.

\`supportsRealtime()\` reports whether the selected engine supports the mode;
\`isRealtime()\` reports its native setting. Both return a promise for 0 or 1.
See the [build guide](../README.md#wasi-realtime-browser) for headers, limits,
and setup.

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
        `${realtimeSection}` +
        `${apiDocTitle}${jsdocMarkdown}` +
        `\n${licenseSection}`,
    ),
  );
