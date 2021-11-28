import { range } from "rambda/dist/rambda.esm.js";
import { freeStringPtr } from "../utils/string-pointers.js";
import { trimNull } from "../utils/trim-null.js";
import { structBufferToObject } from "../utils/structure-buffer-to-object.js";
import { sizeofStruct } from "../utils/native-sizes.js";
import { uint2String } from "../utils/text-encoders.js";
import { CS_MIDIDEVICE } from "../structures.js";

export const csoundSetMidiCallbacks = (wasm) => (csound) => {
  wasm.exports.csoundSetMidiCallbacks(csound);
};

/**
 * This function can be called to obtain a list of available input or output midi devices.
 * If list is NULL, the function will only return the number of devices
 * (isOutput=1 for out devices, 0 for in devices).
 * @async
 * @function
 * @name getMIDIDevList
 * @memberof CsoundObj
 * @param {number} isOutput
 * @return {Promise.<CS_MIDIDEVICE>}
 */
// eslint-disable-next-line unicorn/prevent-abbreviations
export const csoundGetMIDIDevList = (wasm) => (csound, isOutput) => {
  const { buffer } = wasm.exports.memory;
  const numberOfDevices = wasm.exports.csoundGetMIDIDevList(csound, undefined, isOutput ? 1 : 0);
  if (numberOfDevices === 0) return [];
  const structLength = sizeofStruct(CS_MIDIDEVICE);
  const structOffset = wasm.exports.allocCsMidiDeviceStruct(numberOfDevices);
  wasm.exports.csoundGetMIDIDevList(csound, structOffset, isOutput ? 1 : 0);
  const structBuffer = new Uint8Array(buffer, structOffset, structLength * numberOfDevices);
  const out = range(0, numberOfDevices).map((index) =>
    structBufferToObject(CS_MIDIDEVICE, structBuffer.subarray(index * structLength, structLength)),
  );
  wasm.exports.freeCsMidiDeviceStruct(structOffset);
  return out;
};

csoundGetMIDIDevList.toString = () => "getMIDIDevList = async (isOutput) => Object;";

/**
 * This function can be called to obtain a list of available input or output midi devices.
 * If list is NULL, the function will only return the number of devices
 * (isOutput=1 for out devices, 0 for in devices).
 * @async
 * @function
 * @name getRtMidiName
 * @memberof CsoundObj
 * @return {Promise.<string>}
 */
export const csoundGetRtMidiName = (wasm) => (csound) => {
  const { buffer } = wasm.exports.memory;
  const ptr = wasm.exports.getRtMidiName(csound);
  const stringBuffer = new Uint8Array(buffer, ptr, 128);
  return trimNull(uint2String(stringBuffer)) || "";
};

csoundGetRtMidiName.toString = () => "getRtMidiName = async () => String;";

export const csoundGetMidiOutFileName = (wasm) => (csound) => {
  const { buffer } = wasm.exports.memory;
  const ptr = wasm.exports.getMidiOutFileName(csound);
  const stringBuffer = new Uint8Array(buffer, ptr, 128);
  ptr && ptr.length > 0 && freeStringPtr(ptr);
  return trimNull(uint2String(stringBuffer)) || "";
};

export const _isRequestingRtMidiInput = (wasm) => (csound) => {
  return wasm.exports.isRequestingRtMidiInput(csound);
};

/**
 * Emit a midi message with a given triplet of values
 * in the range of 0 to 127.
 * @async
 * @function
 * @name midiMessage
 * @memberof CsoundObj
 * @param {number} midiStatus
 * @param {number} midiData1
 * @param {number} midiData2
 * @return {Promise.<void>}
 */
export const csoundPushMidiMessage = (wasm) => (csound, status, data1, data2) => {
  wasm.exports.pushMidiMessage(csound, status, data1, data2);
};

csoundPushMidiMessage.toString = () => "midiMessage = async (status, data1, data2) => undefined;";

// PUBLIC void 	csoundSetMIDIModule (CSOUND *csound, const char *module)
// PUBLIC void 	csoundSetHostImplementedMIDIIO (CSOUND *csound, int state)
// PUBLIC int 	csoundGetMIDIDevList (CSOUND *csound, CS_MIDIDEVICE *list, int isOutput)
// PUBLIC void 	csoundSetExternalMidiInOpenCallback (CSOUND *, int(*func)(CSOUND *, void **userData, const char *devName))
// PUBLIC void 	csoundSetExternalMidiReadCallback (CSOUND *, int(*func)(CSOUND *, void *userData, unsigned char *buf, int nBytes))
// PUBLIC void 	csoundSetExternalMidiInCloseCallback (CSOUND *, int(*func)(CSOUND *, void *userData))
// PUBLIC void 	csoundSetExternalMidiOutOpenCallback (CSOUND *, int(*func)(CSOUND *, void **userData, const char *devName))
// PUBLIC void 	csoundSetExternalMidiWriteCallback (CSOUND *, int(*func)(CSOUND *, void *userData, const unsigned char *buf, int nBytes))
// PUBLIC void 	csoundSetExternalMidiOutCloseCallback (CSOUND *, int(*func)(CSOUND *, void *userData))
// PUBLIC void 	csoundSetExternalMidiErrorStringCallback (CSOUND *, const char *(*func)(int))
// PUBLIC void 	csoundSetMIDIDeviceListCallback (CSOUND *csound, int(*mididevlist__)(CSOUND *, CS_MIDIDEVICE *list, int isOutput))
