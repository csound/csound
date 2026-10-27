export function assertBrowserPrecision(exports) {
  if (
    typeof exports["csoundGetSizeOfCsFloat"] !== "function" ||
    typeof exports["csoundGetSizeOfCsDouble"] !== "function" ||
    exports["csoundGetSizeOfCsFloat"]() !== 4 ||
    exports["csoundGetSizeOfCsDouble"]() !== 4
  )
    throw new Error("The browser requires a Csound binary built with USE_DOUBLE=0 and USE_FLOAT=1");
}

export function assertBrowserReactor(module) {
  const exports = new Map(WebAssembly.Module.exports(module).map(({ name, kind }) => [name, kind]));
  // Command modules own startup and their pthread runtime. Never hand them to
  // the browser host, which initializes a reactor and supplies plugin stacks.
  if (
    exports.has("_start") ||
    exports.get("_initialize") !== "function" ||
    exports.get("wasi_thread_start") !== "function" ||
    exports.get("__stack_pointer") !== "global"
  )
    throw new Error("Expected the threaded Csound browser reactor with an exported stack pointer");
}
