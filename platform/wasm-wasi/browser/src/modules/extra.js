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

import { freeStringPtr, string2ptr } from "../utils/string-pointers.js";

/**
 * Append 'value' to environment variable 'name'
 * added for internal usage of csound-wasm, feel
 * free to use as well ;)
 */
export const csoundAppendEnv = (wasm) => (csound, variable, value) => {
  const varStringPtr = string2ptr(wasm, variable);
  const valueStringPtr = string2ptr(wasm, value);
  const res = wasm.exports["csoundAppendEnv"](csound, varStringPtr, valueStringPtr);
  freeStringPtr(wasm, varStringPtr);
  freeStringPtr(wasm, valueStringPtr);
  return res;
};

csoundAppendEnv["toString"] = () => "appendEnv = async (csound, variable, value) => Number;";

// deliberately no jsdocs because this is internal only
export const csoundShouldDaemonize = (wasm) => (csound) =>
  wasm.exports["csoundShouldDaemonize"](csound);
