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

import { logMidiRequest as log } from "../logger";

export async function requestMidi({ onMidiMessage /** function(number,number,number):void */ }) {
  log("requesting for web-midi connection")();

  if (navigator && navigator.requestMIDIAccess) {
    try {
      const midiDevices = await navigator.requestMIDIAccess();

      if (midiDevices.inputs) {
        /**
         * @type {Iterator}
         *  @suppress {checkTypes}
         */
        const midiInputs = midiDevices.inputs.values();
        for (let input = midiInputs.next(); input && !input.done; input = midiInputs.next()) {
          log(`Connecting midi-input: ${input.value.name || "unkown"}`)();
          input.value.onmidimessage = onMidiMessage;
        }
      } else {
        log("no midi-device detected")();
      }
    } catch (error) {
      log("error while connecting web-midi: " + error)();
    }
  } else {
    log("no web-midi support found, midi-input will not work!")();
  }
}
