import { readFileSync, realpathSync } from "fs";

const arraybufferCode = () =>
  `
function bufferFromBrowser(base64Data) {
  return window.atob(base64Data);
}
function bufferFromNodeJS(base64Data) {
  return Buffer.from(base64Data, 'base64').toString('binary');
}
function __toArrayBuffer(base64Data) {
  var window = window || this;
  var isBrowser = typeof process === 'undefined';
  var binary = isBrowser ? bufferFromBrowser(base64Data) : bufferFromNodeJS(base64Data);
  var bytes = new Uint8Array(binary.length);
  for (var i = 0; i < binary.length; ++i) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes.buffer;
};
`
    .split("\n")
    .map(Function.prototype.call, String.prototype.trim)
    .join("");

export default function arraybufferPlugin(options = {}) {
  // const filter = createFilter(options.include, options.exclude);
  // const wasmPath = "node_modules/@csound/wasm/lib/libcsound.wasm.zlib";
  return {
    name: "arraybuffer",
    intro: arraybufferCode,
    load(id) {
      if (id.endsWith(".wasm.zlib")) {
        id = realpathSync(id);
        return {
          code: `export default __toArrayBuffer("${readFileSync(id).toString(
            "base64"
          )}");`,
          map: { mappings: "" },
        };
      }
    },
  };
}
