/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { encoder } from "../utils/text-encoders.js";

/** @export */
function writeFile(wasm) {
  return (_, path, data_) => {
    const data = typeof data_ === "string" ? encoder.encode(data_) : data_;
    wasm.wasi.writeFile(path, data);
  };
}

writeFile["toString"] = () => "async (path, data) => void";

/** @export */
function appendFile(wasm) {
  return (_, path, data_) => {
    const data = typeof data_ === "string" ? encoder.encode(data_) : data_;
    wasm.wasi.appendFile(path, data);
  };
}

appendFile["toString"] = () => "async (path, data) => void";

/** @export */
function readFile(wasm) {
  return (_, path) => {
    return wasm.wasi.readFile(path);
  };
}

readFile["toString"] = () => "async (path) => ?Uint8Array";

/** @export */
function unlink(wasm) {
  return (_, path) => {
    return wasm.wasi.unlink(path);
  };
}

unlink["toString"] = () => "async (path) => void";

/** @export */
function readdir(wasm) {
  return (_, path) => wasm.wasi.readdir(path);
}

readdir["toString"] = () => "async (path) => string[]";

/** @export */
function mkdir(wasm) {
  return (_, path) => {
    return wasm.wasi.mkdir(path);
  };
}

mkdir["toString"] = () => "async (path) => void";

/** @export */
function stat(wasm) {
  return (_, path) => {
    return wasm.wasi.stat(path);
  };
}

stat["toString"] = () => "async (path) => ?object";

/** @export */
function pathExists(wasm) {
  return (_, path) => {
    return wasm.wasi.pathExists(path);
  };
}

pathExists["toString"] = () => "async (path) => boolean";

/** @export */
function chdir(wasm) {
  return (_, path) => {
    return wasm.wasi.chdir(path);
  };
}

chdir["toString"] = () => "async (path) => number";

/** @export */
function getcwd(wasm) {
  return (_) => {
    return wasm.wasi.cwd;
  };
}

getcwd["toString"] = () => "async () => string";

export const fs = {};

fs["writeFile"] = writeFile;
fs["appendFile"] = appendFile;
fs["readFile"] = readFile;
fs["unlink"] = unlink;
fs["readdir"] = readdir;
fs["mkdir"] = mkdir;
fs["stat"] = stat;
fs["pathExists"] = pathExists;
fs["chdir"] = chdir;
fs["getcwd"] = getcwd;

export default fs;
