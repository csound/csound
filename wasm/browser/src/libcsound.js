/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
  csoundGetNchnls,
  csoundGetNchnlsInput,
  csoundGetChannels,
  csoundGet0dBFS,
  csoundGetA4,
  csoundGetCurrentTimeSamples,
  csoundGetSizeOfMYFLT,
  csoundSetOption,
  csoundSetParams,
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
  csoundInputMessage,
  csoundInputMessageAsync,
  csoundGetControlChannel,
  csoundSetControlChannel,
  csoundGetStringChannel,
  csoundSetStringChannel,
} from "./modules/control-events";
import { csoundGetInputName, csoundGetOutputName } from "./modules/general-io";
import { csoundAppendEnv, csoundShouldDaemonize } from "./modules/extra";
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
  csoundTableCopyIn,
  csoundTableCopyOut,
  csoundGetTable,
  csoundGetTableArgs,
} from "./modules/table";
import {
  UGEN_ARG_TYPE,
  csoundUgenFactoryNew,
  csoundUgenFactoryDelete,
  csoundUgenContextNew,
  csoundUgenContextDelete,
  csoundUgenSetContext,
  csoundUgenNew,
  csoundUgenDelete,
  csoundUgenGetOutVar,
  csoundUgenGetInVar,
  csoundUgenSetInputVar,
  csoundUgenVarNew,
  csoundUgenVarDelete,
  csoundUgenVarGetType,
  csoundUgenVarGetSize,
  csoundUgenVarSetValue,
  csoundUgenVarGetValue,
  csoundUgenVarGetData,
  csoundUgenVarGetDataAsFloat64Array,
  csoundUgenVarGetKsmps,
  csoundUgenVarSetString,
  csoundUgenVarGetString,
  csoundUgenSetValue,
  csoundUgenGetValue,
  csoundUgenSetString,
  csoundUgenGetString,
  csoundUgenGetInCount,
  csoundUgenGetOutCount,
  csoundUgenGetInType,
  csoundUgenGetOutType,
  csoundUgenInit,
  csoundUgenPerform,
  csoundUgenListOpcodes,
  csoundUgenFindOpcode,
  csoundUgenGraphNew,
  csoundUgenGraphAdd,
  csoundUgenGraphInit,
  csoundUgenGraphPerform,
  csoundUgenGraphDelete,
  csoundUgenGraphDeleteAll,
  csoundUgenVarGetFloat64Array,
} from "./modules/ugen";
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
  csoundGetAPIVersion,
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
  csoundGetNchnls,
  csoundGetNchnlsInput,
  csoundGetChannels,
  csoundGet0dBFS,
  csoundGetA4,
  csoundGetCurrentTimeSamples,
  csoundGetSizeOfMYFLT,
  csoundSetOption,
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
  csoundAppendEnv,
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
  csoundTableCopyIn,
  csoundTableCopyOut,
  csoundGetTable,
  csoundGetTableArgs,
  // @module/ugen
  UGEN_ARG_TYPE,
  csoundUgenFactoryNew,
  csoundUgenFactoryDelete,
  csoundUgenContextNew,
  csoundUgenContextDelete,
  csoundUgenSetContext,
  csoundUgenNew,
  csoundUgenDelete,
  csoundUgenGetOutVar,
  csoundUgenGetInVar,
  csoundUgenSetInputVar,
  csoundUgenVarNew,
  csoundUgenVarDelete,
  csoundUgenVarGetType,
  csoundUgenVarGetSize,
  csoundUgenVarSetValue,
  csoundUgenVarGetValue,
  csoundUgenVarGetData,
  csoundUgenVarGetDataAsFloat64Array,
  csoundUgenVarGetKsmps,
  csoundUgenVarSetString,
  csoundUgenVarGetString,
  csoundUgenSetValue,
  csoundUgenGetValue,
  csoundUgenSetString,
  csoundUgenGetString,
  csoundUgenGetInCount,
  csoundUgenGetOutCount,
  csoundUgenGetInType,
  csoundUgenGetOutType,
  csoundUgenInit,
  csoundUgenPerform,
  csoundUgenListOpcodes,
  csoundUgenFindOpcode,
  csoundUgenGraphNew,
  csoundUgenGraphAdd,
  csoundUgenGraphInit,
  csoundUgenGraphPerform,
  csoundUgenGraphDelete,
  csoundUgenGraphDeleteAll,
  csoundUgenVarGetFloat64Array,
  // filesystem
  fs,
};

export default function (wasm) {
  /** @suppress {missingProperties} */
  const { fs: apiFs, UGEN_ARG_TYPE: ugenArgType, ...apiRest } = api;

  return {
    ...Object.keys(apiRest).reduce((accumulator, k) => {
      accumulator[k] = apiRest[k](wasm);
      return accumulator;
    }, {}),
    ...Object.keys(apiFs).reduce((accumulator, k) => {
      accumulator[k] = apiFs[k](wasm);
      return accumulator;
    }, {}),
    UGEN_ARG_TYPE: ugenArgType,
  };
}
