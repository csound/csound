import { freeStringPtr, string2ptr } from "../utils/string-pointers.js";
/*
   csound Score Handling module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Sees whether Csound score events are performed or not,
 * independently of real-time MIDI events
 * @function
 */
export const csoundIsScorePending = (wasm) => (csound /* CsoundInst */) =>
  wasm.exports.csoundIsScorePending(csound);

csoundIsScorePending.toString = () => "isScorePending = async () => Number;";

/**
 * Sets whether Csound score events are performed or not
 * (real-time events will continue to be performed).
 * Can be used by external software, such as a VST host,
 * to turn off performance of score events (while continuing to perform real-time events),
 * for example to mute a Csound score while working on other tracks of a piece,
 * or to play the Csound instruments live.
 * @function
 */
export const csoundSetScorePending = (wasm) => (csound /* CsoundInst */, pending /* number */) =>
  wasm.exports.csoundSetScorePending(csound, pending);

csoundSetScorePending.toString = () => "setScorePending = async (pending) => Number;";

/**
 * Read, preprocess, and load a score from an ASCII string It can be called repeatedly,
 * with the new score events being added to the currently scheduled ones.
 * @function
 */
export const csoundReadScore = (wasm) => (csound /* CsoundInst */, score /* string */) => {
  const stringPtr = string2ptr(wasm, score);
  const result = wasm.exports.csoundReadScore(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundReadScore.toString = () => "readScore = async (score) => Number;";

/**
 * Returns the current score time in seconds since the beginning of performance.
 * @function
 */
export const csoundGetScoreTime = (wasm) => (csound /* CsoundInst */) =>
  wasm.exports.csoundGetScoreTime(csound);

csoundGetScoreTime.toString = () => "getScoreTime = async () => Number;";

/**
 * Returns the score time beginning at which score events will actually immediately be performed
 * @function
 */
export const csoundGetScoreOffsetSeconds = (wasm) => (csound /* CsoundInst */) =>
  wasm.exports.csoundGetScoreOffsetSeconds(csound);

csoundGetScoreOffsetSeconds.toString = () => "getScoreOffsetSeconds = async () => Number;";

/**
 * Csound score events prior to the specified time are not performed,
 * and performance begins immediately at the specified time
 * (real-time events will continue to be performed as they are received).
 * Can be used by external software, such as a VST host, to begin
 * score performance midway through a Csound score,
 * for example to repeat a loop in a sequencer,
 * or to synchronize other events with the Csound score.
 * @function
 */
export const csoundSetScoreOffsetSeconds = (wasm) => (csound /* CsoundInst */, time /* string */) =>
  wasm.exports.csoundSetScoreOffsetSeconds(csound, time);

csoundSetScoreOffsetSeconds.toString = () => "setScoreOffsetSeconds = async () => Number;";

/**
 * Rewinds a compiled Csound score to the time specified with csoundObj.setScoreOffsetSeconds().
 * @function
 */
export const csoundRewindScore = (wasm) => (csound) =>
  wasm.exports.csoundRewindScore(csound /* CsoundInst */);

csoundRewindScore.toString = () => "rewindScore = async () => undefined;";

// PUBLIC void 	csoundSetCscoreCallback (CSOUND *, void(*cscoreCallback_)(CSOUND *))
// PUBLIC int 	csoundScoreSort (CSOUND *, FILE *inFile, FILE *outFile)
// PUBLIC int 	csoundScoreExtract (CSOUND *, FILE *inFile, FILE *outFile, FILE *extractFile)
