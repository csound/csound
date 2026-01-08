import {
  csoundCreate,
  csoundDestroy,
  csoundGetVersion,
  csoundInitialize,
} from "./modules/instantiation";
import {
  csoundParseOrc,
  csoundCompileTree,
  csoundCompileOrc,
  csoundEvalCode,
  csoundStart,
  csoundCompileCSD,
  csoundPerformKsmps,
  csoundStop,
  csoundReset,
} from "./modules/performance";
import {
  csoundGetSr,
  csoundSystemSr,
  csoundGetKr,
  csoundGetKsmps,
  csoundGetChannels,
  csoundGet0dBFS,
  csoundGetA4,
  csoundGetCurrentTimeSamples,
  csoundGetSizeOfMYFLT,
  csoundSetOption,
  csoundGetParams,
  csoundGetDebug,
  csoundSetDebug,
} from "./modules/attributes";
import {
  csoundGetSpin,
  csoundGetSpout,
} from "./modules/rtaudio";
import {
  csoundGetMIDIDevList,
  csoundSetMidiCallbacks,
  csoundGetRtMidiName,
  csoundGetMidiOutFileName,
  csoundPushMidiMessage,
  _isRequestingRtMidiInput,
} from "./modules/rtmidi";
import {
  csoundEventString,
  csoundGetControlChannel,
  csoundSetControlChannel,
  csoundGetStringChannel,
  csoundSetStringChannel,
} from "./modules/control-events";
import { csoundShouldDaemonize } from "./modules/extra";
import {
  csoundIsScorePending,
  csoundSetScorePending,
  csoundGetScoreTime,
  csoundGetScoreOffsetSeconds,
  csoundSetScoreOffsetSeconds,
  csoundRewindScore,
} from "./modules/score-handling";
import {
  csoundTableLength,
  csoundTableGet,
  csoundTableSet,
  csoundTableCopyIn,
  csoundTableCopyOut,
  csoundGetTable,
  csoundGetTableArgs,
} from "./modules/table";
import fs from "./filesystem/worker-fs";

goog.declareModuleId("libcsound");

/*
   Don't call these functions directly.
   They are closures that take wasm instance as
   first argument before they can be called as
   documented.
*/
/**
 * @type {WasmExports}
 * @suppress {checkTypes}
 */
export const api = {
  // @module/instantiation
  csoundCreate,
  csoundDestroy,
  csoundGetVersion,
  csoundInitialize,
  // @module/performance
  csoundParseOrc,
  csoundCompileTree,
  csoundCompileOrc,
  csoundEvalCode,
  csoundStart,
  csoundCompileCSD,
  csoundPerformKsmps,
  csoundStop,
  csoundReset,
  // @module/attributes
  csoundGetSr,
  csoundSystemSr,
  csoundGetKr,
  csoundGetKsmps,
  csoundGetChannels,
  csoundGet0dBFS,
  csoundGetA4,
  csoundGetCurrentTimeSamples,
  csoundGetSizeOfMYFLT,
  csoundSetOption,
  csoundGetParams,
  csoundGetDebug,
  csoundSetDebug,
  // @module/rtaudio
  csoundGetSpin,
  csoundGetSpout,
  // @module/rtmidi
  csoundGetMIDIDevList,
  csoundSetMidiCallbacks,
  csoundGetRtMidiName,
  csoundGetMidiOutFileName,
  csoundPushMidiMessage,
  _isRequestingRtMidiInput,
  // @module/control_events
  csoundEventString,
  csoundGetControlChannel,
  csoundSetControlChannel,
  csoundGetStringChannel,
  csoundSetStringChannel,
  // @module/extra
  csoundShouldDaemonize,
  // @module/score-handling
  csoundIsScorePending,
  csoundSetScorePending,
  csoundGetScoreTime,
  csoundGetScoreOffsetSeconds,
  csoundSetScoreOffsetSeconds,
  csoundRewindScore,
  // @module/table
  csoundTableLength,
  csoundTableGet,
  csoundTableSet,
  csoundTableCopyIn,
  csoundTableCopyOut,
  csoundGetTable,
  csoundGetTableArgs,
  // filesystem
  fs,
};

export default function (wasm) {
  const { fs: apiFs, ...apiRest } = api;

  return {
    ...Object.keys(apiRest).reduce((accumulator, k) => {
      accumulator[k] = apiRest[k](wasm);
      return accumulator;
    }, {}),
    ...Object.keys(apiFs).reduce((accumulator, k) => {
      accumulator[k] = apiFs[k](wasm);
      return accumulator;
    }, {}),
  };
}
