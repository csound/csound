import {
  csoundCreate,
  csoundDestroy,
  csoundGetAPIVersion,
  csoundGetVersion,
  csoundInitialize,
} from "./modules/instantiation";
import {
  csoundParseOrc,
  csoundCompileTree,
  csoundCompileOrc,
  csoundEvalCode,
  csoundStart,
  csoundCompileCsd,
  csoundCompileCsdText,
  csoundPerform,
  csoundPerformKsmps,
  csoundPerformBuffer,
  csoundStop,
  csoundCleanup,
  csoundReset,
} from "./modules/performance";
import {
  csoundGetSr,
  csoundGetKr,
  csoundGetKsmps,
  csoundGetNchnls,
  csoundGetNchnlsInput,
  csoundGet0dBFS,
  csoundGetA4,
  csoundGetCurrentTimeSamples,
  csoundGetSizeOfMYFLT,
  csoundSetOption,
  csoundSetParams as csoundSetParameters,
  csoundGetParams as csoundGetParameters,
  csoundGetDebug,
  csoundSetDebug,
} from "./modules/attributes";
import {
  csoundGetInputBufferSize,
  csoundGetOutputBufferSize,
  csoundGetInputBuffer,
  csoundGetOutputBuffer,
  csoundGetSpin,
  csoundGetSpout,
} from "./modules/rtaudio";
import {
  csoundGetMIDIDevList as csoundGetMIDIDevelopmentList,
  csoundSetMidiCallbacks,
  csoundGetRtMidiName,
  csoundGetMidiOutFileName,
  csoundPushMidiMessage,
  _isRequestingRtMidiInput,
} from "./modules/rtmidi";
import {
  csoundInputMessage,
  csoundInputMessageAsync,
  csoundGetControlChannel,
  csoundSetControlChannel,
  csoundGetStringChannel,
  csoundSetStringChannel,
} from "./modules/control-events";
import { csoundGetInputName, csoundGetOutputName } from "./modules/general-io";
import { csoundAppendEnv as csoundAppendEnvironment, csoundShouldDaemonize } from "./modules/extra";
import {
  csoundIsScorePending,
  csoundSetScorePending,
  csoundReadScore,
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
  csoundGetTableArgs as csoundGetTableArguments,
  csoundIsNamedGEN,
  csoundGetNamedGEN,
} from "./modules/table";
import * as fs from "./filesystem/worker-fs";

import { assoc, dissoc, keys, mergeAll, reduce } from "rambda/dist/rambda.mjs";

goog.declareModuleId("libcsound");

/*
   Don't call these functions directly.
   They are closures that take wasm instance as
   first argument before they can be called as
   documented.
*/
export const api = {
  // @module/instantiation
  csoundCreate,
  csoundDestroy,
  csoundGetAPIVersion,
  csoundGetVersion,
  csoundInitialize,
  // @module/performance
  csoundParseOrc,
  csoundCompileTree,
  csoundCompileOrc,
  csoundEvalCode,
  csoundStart,
  csoundCompileCsd,
  csoundCompileCsdText,
  csoundPerform,
  csoundPerformKsmps,
  csoundPerformBuffer,
  csoundStop,
  csoundCleanup,
  csoundReset,
  // @module/attributes
  csoundGetSr,
  csoundGetKr,
  csoundGetKsmps,
  csoundGetNchnls,
  csoundGetNchnlsInput,
  csoundGet0dBFS,
  csoundGetA4,
  csoundGetCurrentTimeSamples,
  csoundGetSizeOfMYFLT,
  csoundSetOption,
  csoundSetParams: csoundSetParameters,
  csoundGetParams: csoundGetParameters,
  csoundGetDebug,
  csoundSetDebug,
  // @module/rtaudio
  csoundGetInputBufferSize,
  csoundGetOutputBufferSize,
  csoundGetInputBuffer,
  csoundGetOutputBuffer,
  csoundGetSpin,
  csoundGetSpout,
  // @module/rtmidi
  csoundGetMIDIDevList: csoundGetMIDIDevelopmentList,
  csoundSetMidiCallbacks,
  csoundGetRtMidiName,
  csoundGetMidiOutFileName,
  csoundPushMidiMessage,
  _isRequestingRtMidiInput,
  // @module/control_events
  csoundInputMessage,
  csoundInputMessageAsync,
  csoundGetControlChannel,
  csoundSetControlChannel,
  csoundGetStringChannel,
  csoundSetStringChannel,
  // @module/general_io
  csoundGetInputName,
  csoundGetOutputName,
  // @module/extra
  csoundAppendEnv: csoundAppendEnvironment,
  csoundShouldDaemonize,
  // @module/score-handling
  csoundIsScorePending,
  csoundSetScorePending,
  csoundReadScore,
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
  csoundGetTableArgs: csoundGetTableArguments,
  csoundIsNamedGEN,
  csoundGetNamedGEN,
  // filesystem
  fs,
};

export default function (wasm) {
  return mergeAll([
    reduce((accumulator, k) => assoc(k, api[k](wasm), accumulator), {}, keys(dissoc("fs")(api))),
    reduce((accumulator, k) => assoc(k, api.fs[k](wasm), accumulator), {}, keys(fs)),
  ]);
}
