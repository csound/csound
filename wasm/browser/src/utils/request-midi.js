import { logMidiRequest as log } from "@root/logger";

const connectedMidiDevices = new Set();

export async function requestMidi({ onMidiMessage }) {
  log("requesting for web-midi connection");
  if (navigator && navigator.requestMIDIAccess) {
    try {
      const midiDevices = await navigator.requestMIDIAccess();
      if (midiDevices.inputs) {
        const midiInputs = midiDevices.inputs.values();
        for (let input = midiInputs.next(); input && !input.done; input = midiInputs.next()) {
          log(`Connecting midi-input: ${input.value.name || "unkown"}`);
          if (!connectedMidiDevices.has(input.value.name || "unkown")) {
            input.value.onmidimessage = onMidiMessage;
            connectedMidiDevices.add(input.value.name || "unkown");
          }
        }
      } else {
        log("no midi-device detected");
      }
    } catch (error) {
      log("error while connecting web-midi: " + error);
    }
  } else {
    log("no web-midi support found, midi-input will not work!");
  }
}
