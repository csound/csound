// New artifacts can leave a packet in the shared ring until C has room.
// Keep loading older published artifacts, whose void shim has no capacity result.
export const tryPushMidiMessage = (wasm, csound, status, data1, data2) => {
  const tryPush = wasm.exports["tryPushMidiMessage"];
  if (typeof tryPush === "function") return tryPush(csound, status, data1, data2) === 1;
  wasm.exports["pushMidiMessage"](csound, status, data1, data2);
  return true;
};
