/*
   csound general i/o module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

import { trimNull, uint2String } from '@root/utils';

/**
 * Returns the audio output name (-o)
 * @callback csoundGetOutputName
 * @param {Csound} csound
 * @return {string}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetOutputName}
 */
export const csoundGetOutputName = wasm => csound => {
  const { buffer } = wasm.exports.memory;
  const ptr = wasm.exports.csoundGetOutputName(csound);
  const stringBuffer = new Uint8Array(buffer, ptr, 64);
  return trimNull(uint2String(stringBuffer)) || '';
};

csoundGetOutputName.toString = () => 'csoundGetOutputName = async (csound) => String;';

/**
 * Returns the audio input name (-i)
 * @callback csoundGetInputName
 * @param {Csound} csound
 * @return {string}
 */
/**
 * @param {Object} wasm
 * @return {csoundGetInputName}
 */
export const csoundGetInputName = wasm => csound => {
  const { buffer } = wasm.exports.memory;
  const ptr = wasm.exports.csoundGetInputName(csound);
  const stringBuffer = new Uint8Array(buffer, ptr, 64);
  return trimNull(uint2String(stringBuffer)) || '';
};

csoundGetInputName.toString = () => 'csoundGetInputName = async (csound) => String;';

// PUBLIC void 	csoundSetOutput (CSOUND *csound, const char *name, const char *type, const char *format)
// PUBLIC void 	csoundGetOutputFormat (CSOUND *csound, char *type, char *format)
// PUBLIC void 	csoundSetInput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetMIDIInput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetMIDIFileInput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetMIDIOutput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetMIDIFileOutput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetFileOpenCallback (CSOUND *p, void(*func)(CSOUND *, const char *, int, int, int))
