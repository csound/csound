import { readFileSync, writeFileSync } from "fs";
import { basename, resolve } from "path";

export function inlineArraybuffer(inputFile, modulename) {
  return `function atobPolyfill(input) {
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
                && (output += String.fromCharCode(255 & (bs >> ((-2 * bc) & 6))))
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

          /** @noinline */
          function __toArrayBuffer(base64Data) {
            var binary = bufferFromBrowser(base64Data);
            var bytes = new Uint8Array(binary.length);
            for (var i = 0; i < binary.length; ++i) {
              bytes[i] = binary.charCodeAt(i);
            }
            return bytes.buffer;
          }

         export default () => __toArrayBuffer("${readFileSync(inputFile).toString("base64")}");
            `;
}
