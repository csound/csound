import { readFileSync, realpathSync } from "fs";

const arraybufferCode = () =>
  `
function atobPolyfill(input) {
  var chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
  var str = String(input).replace(/[=]+$/, "");
  if (str.length % 4 === 1) {
    console.error(
      "'atob' failed: The string to be decoded is not correctly encoded."
    );
  }
  for (
    var bc = 0, bs, buffer, idx = 0, output = "";
    (buffer = str.charAt(idx++));
    ~buffer &&
    ((bs = bc % 4 ? bs * 64 + buffer : buffer),
    bc++ % 4)
      ? (output += String.fromCharCode(255 & (bs >> ((-2 * bc) & 6))))
      : 0
  ) {
    buffer = chars.indexOf(buffer);
  }
  return output;
}

function bufferFromBrowser(base64Data) {
  if (typeof AudioWorkletGlobalScope != "undefined") {
    return atobPolyfill(base64Data);
  } else {
    return window.atob(base64Data);
  }
}
function bufferFromNodeJS(base64Data) {
  return Buffer.from(base64Data, "base64").toString("binary");
}
function __toArrayBuffer(base64Data) {
  var window = window || this;
  var isBrowser = typeof process === "undefined";
  var binary = isBrowser
    ? bufferFromBrowser(base64Data)
    : bufferFromNodeJS(base64Data);
  var bytes = new Uint8Array(binary.length);
  for (var i = 0; i < binary.length; ++i) {
    bytes[i] = binary.charCodeAt(i);
  }
  return bytes.buffer;
}
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
      if (id.endsWith(".wasm.z")) {
        id = realpathSync(id);
        return {
          code: `export default __toArrayBuffer("${readFileSync(id).toString("base64")}");`,
          map: { mappings: "" },
        };
      } else if (id.includes("transformer.wasm")) {
        // id = resolve("../");
        return {
          code: `export default __toArrayBuffer("${readFileSync(require.resolve(id)).toString(
            "base64",
          )}");`,
          map: { mappings: "" },
        };
      }
    },
  };
}
