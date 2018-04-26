/* convert.js
 * Author: Steven Yi
 * Description: Script to convert binary libcsound-worklet.wasm to JS file with
 * Uint8Array written into it. Requires node.js to execute.
 */
fs = require('fs');
let wasmData = fs.readFileSync("libcsound-worklet.wasm");

let wasmStr = wasmData.join(",");

let wasmOut = 'AudioWorkletGlobalScope.WAM = { ENVIRONMENT: "WEB" };\n'
wasmOut += "AudioWorkletGlobalScope.WAM.wasmBinary = new Uint8Array([" + wasmStr + "]);";

fs.writeFileSync("libcsound-worklet.wasm.js", wasmOut);
