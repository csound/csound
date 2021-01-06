/*
   csound Score Handling module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

import { freeStringPtr, string2ptr, ptr2string } from "@root/utils";

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
