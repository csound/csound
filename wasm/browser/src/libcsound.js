import { assoc, keys, reduce } from "ramda";
import {
  csoundCreate,
  csoundDestroy,
  csoundGetAPIVersion,
  csoundGetVersion,
  csoundInitialize,
} from "@module/instantiation";
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
} from "@module/performance";
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
  csoundSetParams,
  csoundGetParams,
  csoundGetDebug,
  csoundSetDebug,
} from "@module/attributes";
import {
  csoundGetInputBufferSize,
  csoundGetOutputBufferSize,
  csoundGetInputBuffer,
  csoundGetOutputBuffer,
  csoundGetSpin,
  csoundGetSpout,
} from "@module/rtaudio";
import {
  csoundGetMIDIDevList,
  csoundSetMidiCallbacks,
  csoundGetRtMidiName,
  csoundGetMidiOutFileName,
  csoundPushMidiMessage,
  _isRequestingRtMidiInput,
} from "@module/rtmidi";
import { csoundInputMessage, csoundInputMessageAsync } from "@module/control-events";
import { csoundGetInputName, csoundGetOutputName } from "@module/general-io";
import { csoundAppendEnv } from "@module/extra";

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
  csoundSetParams,
  csoundGetParams,
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
  csoundGetMIDIDevList,
  csoundSetMidiCallbacks,
  csoundGetRtMidiName,
  csoundGetMidiOutFileName,
  csoundPushMidiMessage,
  _isRequestingRtMidiInput,
  // @module/control_events
  csoundInputMessage,
  csoundInputMessageAsync,
  // @module/general_io
  csoundGetInputName,
  csoundGetOutputName,
  // @module/extra
  csoundAppendEnv,
  setupWasmBrowserFS,
};

export default function (wasm) {
  return reduce((accumulator, k) => assoc(k, api[k](wasm), accumulator), {}, keys(api));
}
