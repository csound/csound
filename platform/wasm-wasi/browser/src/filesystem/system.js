// Import the implementation files: Closure cannot parse the package entry's
// `export * as` syntax. The build bundles these files into every backend.
import {
  File,
  Directory,
  OpenFile,
  PreopenDirectory,
  ConsoleStdout,
} from "../../node_modules/@bjorn3/browser_wasi_shim/dist/fs_mem.js";
import CommandWASI from "../../node_modules/@bjorn3/browser_wasi_shim/dist/wasi.js";
import { getGlobalScope } from "../utils/global-scope.js";
import { decoder, encoder } from "../utils/text-encoders.js";

// This is an argv parser, not a shell. Never silently interpret shell syntax
// as filenames or try to fetch commands from the network.
export function commandArguments(command) {
  const args = [];
  let word = "";
  let quote = "";
  let started = false;
  for (let index = 0; index < command.length; index++) {
    const char = command[index];
    if (char === "\n" || char === "\r") throw new Error("Shell syntax is not supported");
    if (char === "\\" && quote !== "'") {
      if (++index === command.length) throw new Error("Trailing command escape");
      word += command[index];
      started = true;
    } else if (quote) {
      if (char === quote) quote = "";
      else word += char;
    } else if (char === '"' || char === "'") {
      quote = char;
      started = true;
    } else if (/\s/.test(char)) {
      if (started) args.push(word);
      word = "";
      started = false;
    } else {
      if (/[|&;<>()$`\n\r]/.test(char)) throw new Error("Shell syntax is not supported");
      word += char;
      started = true;
    }
  }
  if (quote) throw new Error("Unclosed command quote");
  if (started) args.push(word);
  if (!args[0]) throw new Error("Missing command name");
  return args;
}

function equalBytes(left, right) {
  return left.length === right.length && left.every((byte, index) => byte === right[index]);
}

function directoryAt(root, path) {
  let dir = root;
  for (const name of path.split("/").filter(Boolean)) {
    let child = dir.contents.get(name);
    if (!child) {
      child = new Directory(new Map());
      dir.contents.set(name, child);
    }
    if (!(child instanceof Directory)) throw new Error(`Not a directory: ${path}`);
    dir = child;
  }
  return dir;
}

// Copy large project samples only when the command reads them. Truncation
// can replace a file without first copying its previous contents.
class HostFile extends File {
  constructor(read, sourcePath) {
    super([]);
    this.read = read;
    this.loaded = false;
    this.original = undefined;
    this.sourcePath = sourcePath;
  }

  get data() {
    if (!this.loaded) {
      this.original = this.read();
      this.value = new Uint8Array(this.original);
      this.loaded = true;
    }
    return this.value;
  }

  set data(value) {
    this.value = value;
    this.loaded = true;
  }
}

function diagnosticStream() {
  let tail = new Uint8Array();
  return {
    fd: new ConsoleStdout((bytes) => {
      const keep = Math.min(tail.length, Math.max(0, 16000 - bytes.length));
      const next = new Uint8Array(keep + Math.min(bytes.length, 16000));
      next.set(tail.subarray(tail.length - keep));
      next.set(bytes.subarray(Math.max(0, bytes.length - 16000)), keep);
      tail = next;
    }),
    read: () => decoder.decode(tail),
  };
}

// Each command gets its own memory, arguments, descriptors and exit handling.
// Only changed files are copied back, so open Csound descriptors keep their
// position. No files outside Csound's virtual filesystem are exposed.
export function createSystem(host, log) {
  // AudioWorklet lacks these globals. The command host uses the same UTF-8
  // fallback as Csound there; workers and window keep the native constructors.
  const scope = getGlobalScope();
  if (scope["TextEncoder"] === undefined) {
    scope["TextEncoder"] = function () {
      return encoder;
    };
  }
  if (scope["TextDecoder"] === undefined) {
    scope["TextDecoder"] = function () {
      return decoder;
    };
  }
  const modules = new Map();
  return (pointer) => {
    try {
      const memory = new Uint8Array(host.memory.buffer);
      const end = memory.indexOf(0, pointer);
      if (pointer <= 0 || end < pointer) throw new Error("Invalid command string");
      const args = commandArguments(decoder.decode(memory.subarray(pointer, end)));
      let path = host.resolvePath(args[0]);
      let bytes = host.readFile(path);
      if (!bytes && !path.endsWith(".wasm")) {
        path += ".wasm";
        bytes = host.readFile(path);
      }
      if (!bytes) throw new Error(`WASI command not found: ${args[0]}`);
      let cached = modules.get(path);
      if (!cached || !equalBytes(cached.bytes, bytes)) {
        cached = { bytes, module: new WebAssembly.Module(bytes) };
        if (modules.size >= 4) modules.delete(modules.keys().next().value);
        modules.set(path, cached);
      }
      const root = new Directory(new Map());
      const originals = new Set();
      // Like `wasmtime --dir=.`, mount the parent's current directory at the
      // command's root. Preview 1 has no host syscall for inheriting a cwd.
      const prefix = host.cwd === "/" ? "/" : `${host.cwd}/`;
      for (const entry of Object.values(host.fd)) {
        if (!entry || entry.fd <= 3 || !entry.path) continue;
        if (!entry.path.startsWith(prefix)) continue;
        const relative = entry.path.slice(prefix.length);
        if (entry.type === "dir") {
          directoryAt(root, relative);
        } else {
          const slash = relative.lastIndexOf("/");
          directoryAt(root, slash === -1 ? "" : relative.slice(0, slash)).contents.set(
            relative.slice(slash + 1),
            new HostFile(() => host.readFile(entry.path), entry.path),
          );
          originals.add(entry.path);
        }
      }
      const stdout = diagnosticStream();
      const stderr = diagnosticStream();
      const wasi = new CommandWASI(
        args,
        [],
        [
          new OpenFile(new File([])),
          stdout.fd,
          stderr.fd,
          new PreopenDirectory("/", root.contents),
        ],
        { debug: false },
      );
      // Shim 0.4.2 counts UTF-16 code units here. wasi-libc allocates bytes,
      // so non-ASCII filenames otherwise overwrite the argument buffer.
      wasi.wasiImport.args_sizes_get = (argc, size) => {
        const view = new DataView(wasi.inst.exports.memory.buffer);
        view.setUint32(argc, args.length, true);
        view.setUint32(
          size,
          args.reduce((sum, arg) => sum + encoder.encode(arg).length + 1, 0),
          true,
        );
        return 0;
      };
      const instance = new WebAssembly.Instance(cached.module, {
        // eslint-disable-next-line no-useless-computed-key -- Preserve the WASM import namespace in Closure builds.
        ["wasi_snapshot_preview1"]: wasi.wasiImport,
      });
      if (
        !(instance.exports["memory"] instanceof WebAssembly.Memory) ||
        typeof instance.exports["_start"] !== "function"
      ) {
        throw new TypeError("Expected a WASI Preview 1 command with memory and _start exports");
      }
      let status;
      try {
        status = wasi.start(instance);
      } finally {
        // Keep diagnostics even when the command traps.
        for (const stream of [stdout, stderr]) {
          const text = stream.read();
          if (text) log(text);
        }
      }
      const writes = [];
      const save = (dir, parent) => {
        for (const [name, entry] of dir.contents) {
          const filePath = `${parent}/${name}`;
          if (entry instanceof Directory) {
            if (!host.pathExists(filePath)) host.mkdir(filePath);
            save(entry, filePath);
          } else if (entry instanceof File) {
            const moved = entry instanceof HostFile && entry.sourcePath !== filePath;
            if (!(entry instanceof HostFile) || entry.loaded || moved) {
              const original = entry instanceof HostFile ? entry.original : undefined;
              if (moved || !original || !equalBytes(original, entry.data))
                writes.push([filePath, entry.data]);
            }
            originals.delete(filePath);
          }
        }
      };
      save(root, prefix.slice(0, -1));
      // Read renamed files before overwriting their old paths (e.g. a swap).
      for (const [path, data] of writes) host.writeFile(path, data);
      for (const path of originals.keys()) host.unlink(path);
      // system() returns an encoded wait status, not proc_exit's raw code.
      return (status & 255) << 8;
    } catch (error) {
      log(`Cannot run score command: ${error.message}`);
      return -1;
    }
  };
}
