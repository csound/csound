/*
   Realtime Audio I/O module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Returns the number of samples in Csound's input buffer.
 * @async
 * @function
 * @name getInputBufferSize
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetInputBufferSize = (wasm) => (csound) =>
  wasm.exports.csoundGetInputBufferSize(csound);

csoundGetInputBufferSize.toString = () => "getInputBufferSize = async () => Number;";

/**
 * Returns the number of samples in Csound's output buffer.
 * @async
 * @function
 * @name getOutputBufferSize
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundGetOutputBufferSize = (wasm) => (csound) =>
  wasm.exports.csoundGetOutputBufferSize(csound);

csoundGetOutputBufferSize.toString = () => "getOutputBufferSize = async () => Number;";

/**
 * Returns the address of the Csound audio input buffer.
 * @function
 */
export const csoundGetInputBuffer = (wasm /* WasmInst */) => (csound /* CsoundInst */) =>
  wasm.exports.csoundGetInputBuffer(csound);

csoundGetInputBuffer.toString = () => "getInputBuffer = async () => Number;";

/**
 * Returns the address of the Csound audio output buffer.
 * @function
 */
export const csoundGetOutputBuffer = (wasm /* WasmInst */) => (csound /* CsoundInst */) =>
  wasm.exports.csoundGetOutputBuffer(csound);

csoundGetOutputBuffer.toString = () => "getOutputBuffer = async () => Number;";

/**
 * Returns the address of the Csound audio input working buffer (spin).
 * Enables external software to write audio into Csound before calling csoundPerformKsmps.
 * @function
 */
export const csoundGetSpin = (wasm /* WasmInst */) => (csound /* CsoundInst */) =>
  wasm.exports.csoundGetSpin(csound);

csoundGetSpin.toString = () => "getSpin = async (csound) => Number;";

/**
 * Returns the address of the Csound audio output working buffer (spout).
 * Enables external software to read audio from Csound after calling csoundPerformKsmps.
 * @function
 */
export const csoundGetSpout = (wasm /* WasmInst */) => (csound /* CsoundInst */) =>
  wasm.exports.csoundGetSpout(csound);

csoundGetSpout.toString = () => "getSpout = async () => Number;";

// PUBLIC void 	csoundSetRTAudioModule (CSOUND *csound, const char *module)
// PUBLIC int 	csoundGetModule (CSOUND *csound, int number, char **name, char **type)

// PUBLIC void 	csoundClearSpin (CSOUND *)
// PUBLIC void 	csoundAddSpinSample (CSOUND *csound, int frame, int channel, MYFLT sample)
// PUBLIC void 	csoundSetSpinSample (CSOUND *csound, int frame, int channel, MYFLT sample)

// PUBLIC MYFLT 	csoundGetSpoutSample (CSOUND *csound, int frame, int channel)
// PUBLIC void ** 	csoundGetRtRecordUserData (CSOUND *)
// PUBLIC void ** 	csoundGetRtPlayUserData (CSOUND *)
// PUBLIC void 	csoundSetHostImplementedAudioIO (CSOUND *, int state, int bufSize)
// PUBLIC int 	csoundGetAudioDevList (CSOUND *csound, CS_AUDIODEVICE *list, int isOutput)
// PUBLIC void 	csoundSetPlayopenCallback (CSOUND *, int(*playopen__)(CSOUND *, const csRtAudioParams *parm))
// PUBLIC void 	csoundSetRtplayCallback (CSOUND *, void(*rtplay__)(CSOUND *, const MYFLT *outBuf, int nbytes))
// PUBLIC void 	csoundSetRecopenCallback (CSOUND *, int(*recopen_)(CSOUND *, const csRtAudioParams *parm))
// PUBLIC void 	csoundSetRtrecordCallback (CSOUND *, int(*rtrecord__)(CSOUND *, MYFLT *inBuf, int nbytes))
// PUBLIC void 	csoundSetRtcloseCallback (CSOUND *, void(*rtclose__)(CSOUND *))
// PUBLIC void 	csoundSetAudioDeviceListCallback (CSOUND *csound, int(*audiodevlist__)(CSOUND *, CS_AUDIODEVICE *list, int isOutput))
