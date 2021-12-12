import { encoder } from "../utils/text-encoders.js";

export function writeFile(wasm) {
  return (_, path, data_) => {
    const data = typeof data_ === "string" ? encoder.encode(data_) : data_;
    console.log({ data_, data });
    wasm.wasi.writeFile(path, data);
  };
}

export function readdir(wasm) {
  return (_, path, data) => wasm.wasi.readdir(path, data);
}

export function mkdir(wasm) {
  return (_, path) => {
    return wasm.wasi.mkdir(path);
  };
}

mkdir.toString = () => "async (path) => void";
