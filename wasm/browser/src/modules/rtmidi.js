import { range } from "ramda";
import { CS_MIDIDEVICE } from "@root/structures";
import { structBufferToObject } from "@utils/structure-buffer-to-object";
import { freeStringPtr } from "@utils/string-pointers";
import { uint2String } from "@utils/text-encoders";
import { sizeofStruct } from "@utils/native-sizes";
import { trimNull } from "@utils/trim-null";

export const csoundSetMidiCallbacks = (wasm) => (csound) => {
  wasm.exports.csoundSetMidiCallbacks(csound);
};

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

export const csoundGetRtMidiName = (wasm) => (csound) => {
  const { buffer } = wasm.exports.memory;
  const ptr = wasm.exports.getRtMidiName(csound);
  const stringBuffer = new Uint8Array(buffer, ptr, 128);
  return trimNull(uint2String(stringBuffer)) || "";
};

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

export const csoundPushMidiMessage = (wasm) => (csound, status, data1, data2) => {
  return wasm.exports.pushMidiMessage(csound, status, data1, data2);
};

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
