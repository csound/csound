import { decoder } from "../utils/text-encoders.js";

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

// Commands share files with Csound, but own their memory and open handles.
export function createSystem(host, log) {
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
      const wasi = host.createCommand(args);
      const instance = new WebAssembly.Instance(cached.module, wasi.getImports(cached.module));
      let status;
      try {
        status = wasi.startCommand(instance);
      } finally {
        for (const fd of [1, 2]) {
          const text = wasi.readOutput(fd);
          if (text) log(text);
        }
      }
      // system() returns an encoded wait status, not proc_exit's raw code.
      return (status & 255) << 8;
    } catch (error) {
      log(`Cannot run score command: ${error.message}`);
      return -1;
    }
  };
}
