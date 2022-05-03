import { uint2String } from "../utils/text-encoders.js";
import { trimNull } from "../utils/trim-null.js";

/*
   csound general i/o module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Returns the audio output name (-o)
 * @function
 * @name getOutputName
 * @memberof CsoundObj
 * @return {Promise.<string>}
 */
export const csoundGetOutputName = (wasm) => (csound) => {
  const { buffer } = wasm.wasi.memory;
  const ptr = wasm.exports.csoundGetOutputName(csound);
  const stringBuffer = new Uint8Array(buffer, ptr, 64);
  return trimNull(uint2String(stringBuffer)) || "";
};

csoundGetOutputName.toString = () => "getOutputName = async () => String;";

/**
 * Returns the audio input name (-i)
 * @function
 * @name getInputName
 * @memberof CsoundObj
 * @return {Promise.<string>}
 */
export const csoundGetInputName = (wasm) => (csound) => {
  const { buffer } = wasm.wasi.memory;
  const ptr = wasm.exports.csoundGetInputName(csound);
  const stringBuffer = new Uint8Array(buffer, ptr, 64);
  return trimNull(uint2String(stringBuffer)) || "";
};

csoundGetInputName.toString = () => "getInputName = async (csound) => String;";

// PUBLIC void 	csoundSetOutput (CSOUND *csound, const char *name, const char *type, const char *format)
// PUBLIC void 	csoundGetOutputFormat (CSOUND *csound, char *type, char *format)
// PUBLIC void 	csoundSetInput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetMIDIInput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetMIDIFileInput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetMIDIOutput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetMIDIFileOutput (CSOUND *csound, const char *name)
// PUBLIC void 	csoundSetFileOpenCallback (CSOUND *p, void(*func)(CSOUND *, const char *, int, int, int))
