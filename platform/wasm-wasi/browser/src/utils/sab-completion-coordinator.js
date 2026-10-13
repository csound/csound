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

export class SABCompletionCoordinator {
  constructor(onComplete) {
    /** @type {function(string): void} */
    this.onComplete = onComplete;
    /** @type {number} */
    this.generation = 0;
    /** @type {string|undefined} */
    this.endState = undefined;
    /** @type {boolean} */
    this.stopReleased = false;
    /** @type {boolean} */
    this.completed = false;
    this.begin(0);
  }

  begin(generation) {
    this.generation = generation;
    this.endState = undefined;
    this.stopReleased = false;
    this.completed = false;
  }

  isCurrent(generation) {
    return generation === this.generation;
  }

  canAccept(generation) {
    return this.isCurrent(generation) && !this.completed;
  }

  markEnd(endState, generation) {
    if (!this.canAccept(generation)) {
      return false;
    }
    if (!this.endState) {
      this.endState = endState;
    }
    return this.finish();
  }

  markStopReleased(generation) {
    if (!this.canAccept(generation)) {
      return false;
    }
    this.stopReleased = true;
    return this.finish();
  }

  finish() {
    if (!this.endState || !this.stopReleased || this.completed) {
      return false;
    }

    // Latch before invoking application code so re-entrant or replayed
    // signals cannot expose completion more than once for this generation.
    this.completed = true;
    this.onComplete(this.endState);
    return true;
  }
}
