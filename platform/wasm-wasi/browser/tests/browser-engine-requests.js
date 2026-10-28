// Embedded worker URLs are blobs. Other HTTP JS/Wasm would be another engine.
export function extraEngineRequests(requests, expectedBundle) {
  return requests.filter(
    (pathname) =>
      /\.(?:js|wasm)(?:\.z)?$/.test(pathname) &&
      pathname !== expectedBundle &&
      pathname !== "/missing-plugin.wasm" &&
      !/^\/plugin_example(?:_cpp)?(?:_shared|-wasm64)?(?:_missing_tls|_trap_ctor)?\.wasm$/.test(
        pathname,
      ),
  );
}
