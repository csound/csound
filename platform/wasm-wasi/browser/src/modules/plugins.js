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

import { ptr2string } from "../utils/string-pointers.js";

/**
 * Returns whether Csound has one or more pending --opcode-lib requests.
 * @function
 */
export const isRequestingPlugins = (wasm) => (csound) =>
  wasm.exports["isRequestingPlugins"](csound);

isRequestingPlugins["toString"] = () => "isRequestingPlugins = async () => Number;";

/**
 * Returns the pending --opcode-lib paths as a comma-separated string.
 * @function
 */
export const getRequestedPlugins = (wasm) => (csound) => {
  const pointer = wasm.exports["getRequestedPlugins"](csound);
  return pointer ? ptr2string(wasm, pointer) : "";
};

getRequestedPlugins["toString"] = () => "getRequestedPlugins = async () => String;";
