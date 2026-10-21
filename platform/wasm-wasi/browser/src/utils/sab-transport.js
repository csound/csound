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

// Publish the state change before waking the render worker. The AudioWorklet
// never waits; only the producer sleeps when it has filled the output buffer.
export function wakeSAB(state) {
  if (Atomics.exchange(state, AUDIO_STATE.CSOUND_LOCK, 0) === 1)
    Atomics.notify(state, AUDIO_STATE.CSOUND_LOCK);
}

export function waitForSABOutput(state, observedReadIndex) {
  const shouldWait = () =>
    Atomics.load(state, AUDIO_STATE.OUTPUT_READ_INDEX) === observedReadIndex &&
    Atomics.load(state, AUDIO_STATE.IS_PERFORMING) === 1 &&
    Atomics.load(state, AUDIO_STATE.STOP) !== 1 &&
    Atomics.load(state, AUDIO_STATE.IS_PAUSED) !== 1 &&
    Atomics.load(state, AUDIO_STATE.HAS_PENDING_CALLBACKS) !== 1;
  while (shouldWait()) {
    Atomics.store(state, AUDIO_STATE.CSOUND_LOCK, 1);
    // Arm before checking again: a read/control change just before wait must
    // either change the predicate or clear the lock, so no wakeup is lost.
    if (!shouldWait()) break;
    Atomics.wait(state, AUDIO_STATE.CSOUND_LOCK, 1);
  }
  Atomics.store(state, AUDIO_STATE.CSOUND_LOCK, 0);
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
