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
 * @typedef {{value:string, mutable:boolean}}
 */
var WasmGlobalMeta;

/**
 * @constructor
 * @param {WasmGlobalMeta} wasmGlobalMeta
 * @param {number} initialValue
 */
WebAssembly.Global = function (wasmGlobalMeta, initialValue) {};

/**
 * @type {number}
 */
WebAssembly.Global.prototype.value;

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
 * _isRequestingRtMidiInput: function(CsoundInst): number,
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
 * csoundCompileCsd: function(CsoundInst, string): number,
 * csoundCompileCsdText: function(CsoundInst, string): number,
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
 * fs: WasiFS,
 * }}  */
var WasmExports;

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
 * useSPN : boolean,
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
