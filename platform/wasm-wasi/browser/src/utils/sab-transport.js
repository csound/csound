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

import { AUDIO_STATE } from "../constants.js";

// Worker only: notifications can arrive before wait(), or without a state
// change. The shared predicates, rather than the notification, decide when
// performance can continue.
export function waitForSABResume(state, onPaused, onResumed) {
  if (
    Atomics.load(state, AUDIO_STATE.IS_PAUSED) !== 1 ||
    Atomics.load(state, AUDIO_STATE.STOP) === 1
  )
    return;
  onPaused();
  while (
    Atomics.load(state, AUDIO_STATE.IS_PAUSED) === 1 &&
    Atomics.load(state, AUDIO_STATE.STOP) !== 1
  ) {
    Atomics.wait(state, AUDIO_STATE.IS_PAUSED, 1);
  }
  if (Atomics.load(state, AUDIO_STATE.STOP) !== 1) onResumed();
}

export function stopSAB(state) {
  Atomics.store(state, AUDIO_STATE.STOP, 1);
  Atomics.store(state, AUDIO_STATE.IS_PERFORMING, 0);
  // Clear both predicates before notifying, even if the worker has not yet
  // entered wait() or its pause acknowledgement is still in transit.
  Atomics.store(state, AUDIO_STATE.IS_PAUSED, 0);
  Atomics.notify(state, AUDIO_STATE.IS_PAUSED);
  Atomics.store(state, AUDIO_STATE.CSOUND_LOCK, 0);
  Atomics.notify(state, AUDIO_STATE.CSOUND_LOCK);
}
