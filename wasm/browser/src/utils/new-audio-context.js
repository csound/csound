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

import { getGlobalScope } from "./global-scope.js";

export const WebkitAudioContext = () => {
  const globalScope = getGlobalScope();
  if (globalScope?.webkitAudioContext !== undefined) {
    return globalScope.webkitAudioContext;
  } else if (globalScope?.AudioContext !== undefined) {
    return globalScope.AudioContext;
  }
};

export const newAudioContext = () => {
  const AudioCTX = WebkitAudioContext();
  if (AudioCTX) {
    return new AudioCTX();
  }
};
