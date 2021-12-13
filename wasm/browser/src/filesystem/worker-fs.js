import { encoder } from "../utils/text-encoders.js";

export function writeFile(wasm) {
  return (_, path, data_) => {
    const data = typeof data_ === "string" ? encoder.encode(data_) : data_;
    wasm.wasi.writeFile(path, data);
  };
}

writeFile.toString = () => "async (path, data) => void";

export function readdir(wasm) {
  return (_, path) => wasm.wasi.readdir(path);
}

readdir.toString = () => "async (path) => string[]";

export function mkdir(wasm) {
  return (_, path) => {
    return wasm.wasi.mkdir(path);
  };
}

mkdir.toString = () => "async (path) => void";
