import { freeStringPtr, ptr2string, string2ptr } from "../utils/string-pointers.js";

/*
   csound control-events module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

/**
 * Inputs an immediate score event
 * without any pre-process parsing
 * @async
 * @function
 * @name inputMessage
 * @memberof CsoundObj
 * @param {string} scoreEvent
 * @return {Promise.<number>}
 */
export const csoundInputMessage = (wasm) => (csound, scoEvent) => {
  const stringPtr = string2ptr(wasm, scoEvent);
  const result = wasm.exports.csoundInputMessage(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundInputMessage.toString = () => "inputMessage = async (scoreEvent) => Number;";

/**
 * Inputs an immediate score event
 * without any pre-process parsing
 * @async
 * @function
 * @name inputMessageAsync
 * @memberof CsoundObj
 * @param {string} scoreEvent
 * @return {Promise.<number>}
 */
export const csoundInputMessageAsync = (wasm) => (csound, scoEvent) => {
  const stringPtr = string2ptr(wasm, scoEvent);
  const result = wasm.exports.csoundInputMessageAsync(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundInputMessageAsync.toString = () => "inputMessageAsync = async (scoreEvent) => Number;";

/**
 * Retrieves the value of control channel identified by channelName.
 * If the err argument is not NULL, the error (or success) code finding
 * or accessing the channel is stored in it.
 * @async
 * @function
 * @name getControlChannel
 * @memberof CsoundObj
 * @param {string} channelName
 * @return {Promise.<undefined>}
 */
export const csoundGetControlChannel = (wasm) => (csound, channelName) => {
  const stringPtr = string2ptr(wasm, channelName);
  const result = wasm.exports.csoundGetControlChannelWasi(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundGetControlChannel.toString = () => "getControlChannel = async (channelName) => Number;";

/**
 * Sets the value of control channel identified by channelName
 * @async
 * @function
 * @name setControlChannel
 * @memberof CsoundObj
 * @param {string} channelName
 * @param {number} value
 * @return {Promise.<undefined>}
 */
export const csoundSetControlChannel = (wasm) => (csound, channelName, value) => {
  const stringPtr = string2ptr(wasm, channelName);
  wasm.exports.csoundSetControlChannel(csound, stringPtr, value);
  freeStringPtr(wasm, stringPtr);
};

csoundSetControlChannel.toString = () => "setControlChannel = async (channelName, value) => void;";

/**
 * Retrieves the string channel identified by channelName
 * @async
 * @function
 * @name getStringChannel
 * @memberof CsoundObj
 * @param {string} channelName
 * @return {Promise.<undefined>}
 */
export const csoundGetStringChannel = (wasm) => (csound, channelName) => {
  const stringPtr = string2ptr(wasm, channelName);
  const pointerToResult = wasm.exports.csoundGetStringChannelWasi(csound, stringPtr);
  const result = ptr2string(wasm, pointerToResult);

  freeStringPtr(wasm, stringPtr);
  freeStringPtr(wasm, pointerToResult);
  return result;
};

csoundGetStringChannel.toString = () => "getStringChannel = async (channelName) => String;";

/**
 * Sets the string channel value identified by channelName
 * @async
 * @function
 * @name setStringChannel
 * @memberof CsoundObj
 * @param {string} channelName
 * @param {string} value
 * @return {Promise.<undefined>}
 */
export const csoundSetStringChannel = (wasm) => (csound, channelName, value) => {
  const stringPtr = string2ptr(wasm, channelName);
  const stringPtr2 = string2ptr(wasm, value);
  wasm.exports.csoundSetStringChannel(csound, stringPtr, stringPtr2);
  freeStringPtr(wasm, stringPtr);
  freeStringPtr(wasm, stringPtr2);
};

csoundSetStringChannel.toString = () => "setStringChannel = async (channelName, value) => void;";

// csoundGetChannelPtr (CSOUND *, MYFLT **p, const char *name, int type)
// csoundListChannels (CSOUND *, controlChannelInfo_t **lst)
// csoundDeleteChannelList (CSOUND *, controlChannelInfo_t *lst)
// csoundSetControlChannelHints (CSOUND *, const char *name, controlChannelHints_t hints)
// csoundGetControlChannelHints (CSOUND *, const char *name, controlChannelHints_t *hints)
// csoundGetChannelLock (CSOUND *, const char *name)
// csoundSetControlChannel (CSOUND *csound, const char *name, MYFLT val)
// csoundGetChannelDatasize (CSOUND *csound, const char *name)
// csoundSetInputChannelCallback (CSOUND *csound, channelCallback_t inputChannelCalback)
// csoundSetOutputChannelCallback (CSOUND *csound, channelCallback_t outputChannelCalback)
// csoundSetPvsChannel (CSOUND *, const PVSDATEXT *fin, const char *name)
// csoundGetPvsChannel (CSOUND *csound, PVSDATEXT *fout, const char *name)
// csoundScoreEvent (CSOUND *, char type, const MYFLT *pFields, long numFields)
// csoundScoreEventAsync (CSOUND *, char type, const MYFLT *pFields, long numFields)
// csoundScoreEventAbsolute (CSOUND *, char type, const MYFLT *pfields, long numFields, double time_ofs)
// csoundScoreEventAbsoluteAsync (CSOUND *, char type, const MYFLT *pfields, long numFields, double time_ofs)
// csoundKillInstance (CSOUND *csound, MYFLT instr, char *instrName, int mode, int allow_release)
// csoundRegisterSenseEventCallback (CSOUND *, void(*func)(CSOUND *, void *), void *userData)
// csoundKeyPress (CSOUND *, char c)
// csoundRegisterKeyboardCallback (CSOUND *, int(*func)(void *userData, void *p, unsigned int type), void *userData, unsigned int type)
// csoundRemoveKeyboardCallback (CSOUND *csound, int(*func)(void *, void *, unsigned int))
