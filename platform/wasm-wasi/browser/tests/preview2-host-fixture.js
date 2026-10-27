import { WASI } from "../src/filesystem/preview2.js";
const encode = (text) => [...new TextEncoder().encode(text)];
const leb = (n) => {
  const bytes = [];
  do {
    const byte = n & 127;
    n >>>= 7;
    bytes.push(byte | (n ? 128 : 0));
  } while (n);
  return bytes;
};
const name = (s) => [...leb(encode(s).length), ...encode(s)];
const section = (id, bytes) => [id, ...leb(bytes.length), ...bytes];
export function importsFor(host, module, names) {
  const binary = new Uint8Array([
    0,
    97,
    115,
    109,
    1,
    0,
    0,
    0,
    ...section(1, [1, 0x60, 0, 0]),
    ...section(2, [
      ...leb(names.length),
      ...names.flatMap((s) => [...name(module), ...name(s), 0, 0]),
    ]),
  ]);
  return host.getImports(new WebAssembly.Module(binary))[module];
}
export function setupHost(host = new WASI()) {
  const memory = new WebAssembly.Memory({ initial: 2 });
  let heap = 4096;
  host.setInstance({
    exports: {
      memory,
      cabi_realloc: (_ptr, _old, align, length) => {
        const pointer = Math.ceil(heap / align) * align;
        heap = pointer + length;
        if (heap > memory.buffer.byteLength)
          memory.grow(Math.ceil((heap - memory.buffer.byteLength) / 65536));
        return pointer;
      },
    },
  });
  const dirs = importsFor(host, "wasi:filesystem/preopens@0.2.0", ["get-directories"]);
  dirs["get-directories"](16);
  const root = host.getMemory().getUint32(host.getMemory().getUint32(16, true), true);
  const methods = [
    "open-at",
    "write-via-stream",
    "read-via-stream",
    "unlink-file-at",
    "stat",
    "get-flags",
    "set-size",
    "write",
  ];
  const filesystem = importsFor(
    host,
    "wasi:filesystem/types@0.2.0",
    methods.map((m) => `[method]descriptor.${m}`),
  );
  return {
    host,
    root,
    filesystem,
    call(method, ...args) {
      return filesystem[`[method]descriptor.${method}`](...args);
    },
  };
}
