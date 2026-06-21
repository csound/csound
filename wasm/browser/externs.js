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

/**
 * @fileoverview Public API.
 * @externs
 */

/**
 * @param {number} index
 * @return {number}
 * @nosideeffects
 */
String.prototype.charPointAt = function (index) {};

/**
 * @constructor
 * @struct
 * @nosideeffects
 * @suppress {duplicate}
 * @param {!(string|number|Uint8Array|ArrayBuffer|Array<*>|SharedArrayBuffer)} arg
 * @param {string=} encoding
 */
var Buffer = function (arg, encoding) {};

/**
 * @param {ArrayBuffer|SharedArrayBuffer} arrayBuffer
 * @param {number=} byteOffset
 * @param {number=} length
 * @return {Buffer}
 * @nosideeffects
 */
Buffer.from = function (arrayBuffer, byteOffset, length) {};

/**
 * @param {Array.<Buffer|ArrayBuffer|SharedArrayBuffer>} arrayBuffer
 * @param {number=} length
 * @return {Buffer}
 * @nosideeffects
 */
Buffer.concat = function (arrayBuffer, length) {};

/**
 * @function
 * @param {string} awScopeName
 * @param {Object} awClassName
 */
var registerProcessor = function (awScopeName, awClassName) {};

/** @typedef {{
 * csoundCreate: function(): number,
 * csoundDestroy: function(number): number,
 * csoundGetAPIVersion: function(number): number,
 * csoundGetVersion: function(number): number,
 * csoundInitialize: function(): number,
 * }}
 */
var LibcsoundUncloned;

// not sure if this hack is a good idea
var process = { cwd: () => "/" };

/** @typedef {number}  */
var CsoundInst;

/** @typedef {{
 * writeFile: function(Object),
 * appendFile: function(Object),
 * readFile: function(Object),
 * unlink: function(Object),
 * readdir: function(Object),
 * mkdir: function(Object),
 * }}  */
var WasiFS;

/** @typedef {{
 * _start: function(): void,
 * __wasm_call_ctors: function(): void,
 * __wasi_js_csoundSetMessageStringCallback: function(): void,
 * isRequestingRtMidiInput: function(CsoundInst): number,
 * _isRequestingRtMidiInput: function(CsoundInst): number,
 * isRequestingRtMidiInput: function(CsoundInst): number,
 * freeStringMem: function(number): void,
 * csoundCreate: function(CsoundInst): number,
 * csoundDestroy: function(CsoundInst): number,
 * csoundGetAPIVersion: function(CsoundInst): number,
 * csoundGetVersion: function(CsoundInst): number,
 * csoundInitialize: function(CsoundInst): number,
 * csoundParseOrc: function(CsoundInst, string): number,
 * csoundCompileTree: function(CsoundInst, Object): number,
 * csoundCompileOrc: function(CsoundInst, string): number,
 * csoundEvalCode: function(CsoundInst, string): number,
 * csoundStart: function(CsoundInst): number,
 * csoundCompileCSD: function(CsoundInst, string, number, number): number,
 * csoundPerform: function(CsoundInst): number,
 * csoundPerformKsmps: function(CsoundInst): number,
 * csoundPerformBuffer: function(CsoundInst): number,
 * csoundStop: function(CsoundInst): number,
 * csoundCleanup: function(CsoundInst): number,
 * csoundReset: function(CsoundInst): number,
 * csoundGetSr: function(CsoundInst): number,
 * csoundGetKr: function(CsoundInst): number,
 * csoundGetKsmps: function(CsoundInst): number,
 * csoundGetNchnls: function(CsoundInst): number,
 * csoundGetNchnlsInput: function(CsoundInst): number,
 * csoundGet0dBFS: function(CsoundInst): number,
 * csoundGetA4: function(CsoundInst): number,
 * csoundGetCurrentTimeSamples: function(CsoundInst): number,
 * csoundGetSizeOfMYFLT: function(CsoundInst): number,
 * csoundSetOption: function(CsoundInst, string): number,
 * csoundSetParams: function(CsoundInst, Object): number,
 * csoundGetParams: function(CsoundInst, number): number,
 * csoundGetDebug: function(CsoundInst): number,
 * csoundSetDebug: function(CsoundInst, number): number,
 * csoundGetInputBufferSize: function(CsoundInst, number): number,
 * csoundGetOutputBufferSize: function(CsoundInst, number): number,
 * csoundGetInputBuffer: function(CsoundInst): number,
 * csoundGetOutputBuffer: function(CsoundInst): number,
 * csoundGetSpout: function(CsoundInst): number,
 * csoundGetSpin: function(CsoundInst): number,
 * isRequestingRtAudioInput: function(CsoundInst): number,
 * _isRequestingRtAudioInput: function(CsoundInst): number,
 * csoundGetMIDIDevList: function(CsoundInst, Object, number): number,
 * csoundSetMidiCallbacks: function(CsoundInst): number,
 * csoundGetRtMidiName: function(CsoundInst): string,
 * csoundGetMidiOutFileName: function(CsoundInst): string,
 * csoundPushMidiMessage: function(CsoundInst, number, number, number): number,
 * csoundInputMessage: function(CsoundInst, string): number,
 * csoundInputMessageAsync: function(CsoundInst, string): number,
 * csoundGetControlChannel: function(CsoundInst, string): number,
 * csoundSetControlChannel: function(CsoundInst, string, number): undefined,
 * csoundGetStringChannel: function(CsoundInst, string): string,
 * csoundSetStringChannel: function(CsoundInst, string, string): undefined,
 * csoundGetInputName: function(CsoundInst): string,
 * csoundGetOutputName: function(CsoundInst): string,
 * csoundAppendEnv: function(CsoundInst, string, string): number,
 * csoundShouldDaemonize: function(CsoundInst): number,
 * csoundIsScorePending: function(CsoundInst): number,
 * csoundSetScorePending: function(CsoundInst, number): number,
 * csoundReadScore: function(CsoundInst, string): number,
 * csoundGetScoreTime: function(CsoundInst): number,
 * csoundGetScoreOffsetSeconds: function(CsoundInst): number,
 * csoundSetScoreOffsetSeconds: function(CsoundInst, number): number,
 * csoundRewindScore: function(CsoundInst): undefined,
 * csoundTableLength: function(CsoundInst): number,
 * csoundTableGet: function(CsoundInst, number, number): number,
 * csoundTableSet: function(CsoundInst, number, number, number): undefined,
 * csoundTableCopyIn: function(CsoundInst, number, number): undefined,
 * csoundTableCopyOut: function(CsoundInst, number, number): (Float64Array | undefined),
 * csoundGetTable: function(CsoundInst, number): (Float64Array | undefined),
 * csoundGetTableArgs: function(CsoundInst, number): (Float64Array | undefined),
 * csoundIsNamedGEN: function(CsoundInst, number): number,
 * csoundGetNamedGEN: function(CsoundInst, number): number,
 * UGEN_ARG_TYPE: UgenArgTypeEnum,
 * csoundUgenFactoryNew: function(CsoundInst): number,
 * csoundUgenFactoryDelete: function(number): number,
 * csoundUgenContextNew: function(CsoundInst): number,
 * csoundUgenContextDelete: function(number): number,
 * csoundUgenSetContext: function(number, number): number,
 * csoundUgenNew: function(number, string, string, string): number,
 * csoundUgenDelete: function(number): number,
 * csoundUgenGetOutVar: function(number, number): number,
 * csoundUgenGetInVar: function(number, number): number,
 * csoundUgenSetInputVar: function(number, number, number): number,
 * csoundUgenVarNew: function(CsoundInst, number): number,
 * csoundUgenVarDelete: function(number): number,
 * csoundUgenVarGetType: function(number): number,
 * csoundUgenVarGetSize: function(number): number,
 * csoundUgenVarSetValue: function(number, number): number,
 * csoundUgenVarGetValue: function(number): number,
 * csoundUgenVarGetData: function(number): number,
 * csoundUgenVarGetDataAsFloat64Array: function(number): number,
 * csoundUgenVarGetKsmps: function(number): number,
 * csoundUgenVarSetString: function(number, string): number,
 * csoundUgenVarGetString: function(number): (string|null),
 * csoundUgenSetValue: function(number, number, number): number,
 * csoundUgenGetValue: function(number, number): number,
 * csoundUgenSetString: function(number, number, string): number,
 * csoundUgenGetString: function(number, number): (string|null),
 * csoundUgenGetInCount: function(number): number,
 * csoundUgenGetOutCount: function(number): number,
 * csoundUgenGetInType: function(number, number): number,
 * csoundUgenGetOutType: function(number, number): number,
 * csoundUgenInit: function(number): number,
 * csoundUgenPerform: function(number): number,
 * csoundUgenListOpcodes: function(CsoundInst, number, number): Array,
 * csoundUgenFindOpcode: function(CsoundInst, string, string, string): number,
 * csoundUgenGraphNew: function(CsoundInst): number,
 * csoundUgenGraphAdd: function(number, number): number,
 * csoundUgenGraphInit: function(number): number,
 * csoundUgenGraphPerform: function(number): number,
 * csoundUgenGraphDelete: function(number): number,
 * csoundUgenGraphDeleteAll: function(number): number,
 * csoundUgenVarGetFloat64Array: function(number): Float64Array,
 * wasm: Object,
 * getMemory: function(): Object,
 * fs: WasiFS,
 * eventNames: function(): Array<string>,
 * listenerCount: function(): number,
 * listeners: function(string): Array,
 * on: function(string, function()): undefined,
 * once: function(string, function()): undefined,
 * off: function(function()): undefined,
 * removeAllListeners: function(string): undefined,
 * removeListener: function(string, function()): undefined,
 * }}  */
var WasmExports;

/** @typedef {{
 * I: number,
 * K: number,
 * A: number,
 * S: number,
 * F: number,
 * UNKNOWN: number,
 * }}
 */
var UgenArgTypeEnum;

/** @typedef {{
 * exports: WasmExports,
 * memory: DataView,
 * }}  */
var WasmInst;

/** @typedef {{
 * start: function(): Promise.<number>,
 * stop: function(): Promise.<number>,
 * getNchnls: function(CsoundInst): Promise.<number>,
 * getInputName: function(CsoundInst): Promise.<string>,
 * getSr: function(CsoundInst): Promise.<number>,
 * isRequestingRtMidiInput: function(CsoundInst): Promise.<number>,
 * _isRequestingRtMidiInput: function(CsoundInst): Promise.<number>,
 * isRequestingRtMidiInput: function(CsoundInst): Promise.<number>,
 * csoundPushMidiMessage: function(CsoundInst, number, number, number): void,
 * }}
 */
var CsoundObj;

/** @typedef {{
 * audioContext: Object,
 * inputChannelCount: number,
 * outputChannelCount: number,
 * autoConnect: boolean,
 * withPlugins: Array<string>,
 * useWorker: boolean,
 * useSAB: boolean,
 * }}  */
var CsoundExportArguments;

/** @typedef {{
 * fd: number,
 * path: string,
 * seekPos: BigInt,
 * buffers: Array<ArrayBuffer>
 * }}
 */
var WasiFileDescriptor;

/** @typedef {{
 * fd: WasiFileDescriptor
 * }}  */
var WasiThis;

/** @typedef {{
 * resume: function(): Promise.<void>,
 * initializeMessagePort: function(Object): Promise<void>,
 * }}  */
var WorkletSinglethreadProxy;

/** @typedef {{
 * messagePort: Object!,
 * rtmidiPort: Object!,
 * }}  */
var InitializeMessagePortPayload;

/** @typedef {{
 * callUncloned: function(string, Array<*>): Promise.<number | string | undefined>,
 * initializeMessagePort: function(Object): Promise<void>,
 * }}  */
var SABMainProxy;

/** @typedef {{
 * handleMidiInput: function(Object): void,
 * eventPromises: Object,
 * publicEvents: Object,
 * hasSharedArrayBuffer: boolean,
 * audioStatePointer: Int32Array,
 * audioStreamIn: SharedArrayBuffer,
 * audioStreamOut: SharedArrayBuffer,
 * }}  */
var CsoundWorkerMain;
