/*
   csound control-events module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

import { freeStringPtr, string2ptr, ptr2string } from "@root/utils";

/**
 * Inputs an immediate score event
 * without any pre-process parsing
 * @async
 * @function
 * @name inputMessage
 * @memberof CsoundObj
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
 * @return {Promise.<number>}
 */
export const csoundInputMessageAsync = (wasm) => (csound, scoEvent) => {
  const stringPtr = string2ptr(wasm, scoEvent);
  const result = wasm.exports.csoundInputMessageAsync(csound, stringPtr);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundInputMessageAsync.toString = () => "inputMessageAsync = async (scoreEvent) => Number;";

export const csoundGetControlChannel = (wasm) => (csound, channelName) => {
  // console.log("Requesting channel value for: " + channelName);
  const stringPtr = string2ptr(wasm, channelName);
  const result = wasm.exports.csoundGetControlChannelWasi(csound, stringPtr);
  // console.log("result: " + result);
  freeStringPtr(wasm, stringPtr);
  return result;
};

csoundGetControlChannel.toString = () => "getControlChannel = async (channelName) => Number;";

export const csoundSetControlChannel = (wasm) => (csound, channelName, value) => {
  const stringPtr = string2ptr(wasm, channelName);
  wasm.exports.csoundSetControlChannel(csound, stringPtr, value);
  freeStringPtr(wasm, stringPtr);
  // TODO: Do our API methods needs to return anything?
  return null; 
};

csoundGetControlChannel.toString = () => "setControlChannel = async (channelName) => void;";

export const csoundGetStringChannel = (wasm) => (csound, channelName) => {
  const stringPtr = string2ptr(wasm, channelName);
  const resPtr = wasm.exports.csoundGetStringChannelWasi(csound, stringPtr);
  const result = ptr2string(wasm, resPtr);

  freeStringPtr(wasm, stringPtr);
  freeStringPtr(wasm, resPtr);
  return result;
};

csoundGetStringChannel.toString = () => "getStringChannel = async (channelName) => String;";

export const csoundSetStringChannel = (wasm) => (csound, channelName, value) => {
  const stringPtr = string2ptr(wasm, channelName);
  const stringPtr2 = string2ptr(wasm, value);
  wasm.exports.csoundSetStringChannel(csound, stringPtr, stringPtr2);
  freeStringPtr(wasm, stringPtr);
  freeStringPtr(wasm, stringPtr2);
  // TODO: Do our API methods needs to return anything?
  return null; 
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
