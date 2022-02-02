import { readFileSync } from "fs";
import { basename, resolve } from "path";
import { createFilter } from "rollup-pluginutils";

const inlineCode = () => `
// http://stackoverflow.com/questions/10343913/how-to-create-a-web-worker-from-a-string
function __inlineWorker(content) {
  var URL = window.URL || window.webkitURL;
  try {
    try {
      var blob;

      try {
        // BlobBuilder = Deprecated, but widely implemented
        var BlobBuilder =
          window.BlobBuilder ||
          window.WebKitBlobBuilder ||
          window.MozBlobBuilder ||
          window.MSBlobBuilder;

        blob = new BlobBuilder();

        blob.append(content);

        blob = blob.getBlob();
      } catch (e) {
        // The proposed API
        blob = new Blob([content]);
      }

      return new Worker(URL.createObjectURL(blob));
    } catch (e) {
      return new Worker(
        "object-src 'self' blob: data:application/javascript," + encodeURIComponent(content)
      );
    }
  } catch (e) {
    throw Error('Inline worker is not supported');

    return new Worker(url);
  }
};
`;

export default function inlineWebworker(options = {}) {
  const filter = createFilter(options.include, options.exclude);
  const dataUrl = options.dataUrl;
  return {
    name: "inline-webworker",
    load(id) {
      if (filter(id)) {
        const filename = basename(id);
        id = resolve("./dist", `__compiled.${filename}`);
        return {
          code: dataUrl
            ? `
             export default () => "data:application/javascript;base64,${readFileSync(id, {
               encoding: "base64",
             })}";
            `
            : `
          export default () => (window.URL || window.webkitURL).createObjectURL(new Blob([${JSON.stringify(
            readFileSync(id).toString("utf8"),
          )}]));`,
          map: { mappings: "" },
        };
      }
    },
  };
}
