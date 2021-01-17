/*
   csound Score Handling module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Sees whether Csound score events are performed or not,
 * independently of real-time MIDI events
 * @async
 * @function
 * @name isScorePending
 * @memberof CsoundObj
 * @return {Promise.<number>}
 */
export const csoundIsScorePending = (wasm) => (csound) => wasm.exports.csoundIsScorePending(csound);

csoundIsScorePending.toString = () => "isScorePending = async () => Number;";

/**
 * Sets whether Csound score events are performed or not
 * (real-time events will continue to be performed).
 * Can be used by external software, such as a VST host,
 * to turn off performance of score events (while continuing to perform real-time events),
 * for example to mute a Csound score while working on other tracks of a piece,
 * or to play the Csound instruments live.
 * @async
 * @function
 * @name setScorePending
 * @memberof CsoundObj
 * @param {number} pending
 * @return {Promise.<undefined>}
 */
export const csoundSetScorePending = (wasm) => (csound, pending) =>
  wasm.exports.csoundSetScorePending(csound, pending);

csoundSetScorePending.toString = () => "setScorePending = async (pending) => Number;";

// PUBLIC int 	csoundReadScore (CSOUND *csound, const char *str)
// PUBLIC void 	csoundReadScoreAsync (CSOUND *csound, const char *str)
// PUBLIC double 	csoundGetScoreTime (CSOUND *)
// PUBLIC MYFLT 	csoundGetScoreOffsetSeconds (CSOUND *)
// PUBLIC void 	csoundSetScoreOffsetSeconds (CSOUND *, MYFLT time)
// PUBLIC void 	csoundRewindScore (CSOUND *)
// PUBLIC void 	csoundSetCscoreCallback (CSOUND *, void(*cscoreCallback_)(CSOUND *))
// PUBLIC int 	csoundScoreSort (CSOUND *, FILE *inFile, FILE *outFile)
// PUBLIC int 	csoundScoreExtract (CSOUND *, FILE *inFile, FILE *outFile, FILE *extractFile)
