import { logMidiRequest as log } from "../logger";

export async function requestMidi({ onMidiMessage /** function(number,number,number):void */ }) {
  log("requesting for web-midi connection")();

  if (navigator && navigator.requestMIDIAccess) {
    try {
      const midiDevices = await navigator.requestMIDIAccess();

      if (midiDevices.inputs) {
        /** @type {Iterator}
         *  @supress {JSC_WRONG_ARGUMENT_COUNT}
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
