/**
 * @fileoverview Public API.
 * @externs
 */

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

/**
 * @suppress {duplicate}
 * @param {!(function(!MIDIInput))}
 * @return {IteratorIterable<string>}
 */
MIDIInputMap.prototype.values = function () {};

/** @typedef {number}  */
var CsoundInst;

/** @typedef {{
 * freeStringMem: function(number): void,
 * csoundGetSpout: function(CsoundInst): number,
 * csoundGetSpin: function(CsoundInst): number,
 * csoundGetInputBuffer: function(CsoundInst): number,
 * csoundGetOutputBuffer: function(CsoundInst): number,
 * }}  */
var WasmExports;

/** @typedef {{
 * exports: WasmExports
 * }}  */
var WasmInst;

/** @typedef {{
 * start: function(): Promise.<number>,
 * stop: function(): Promise.<number>,
 * }}
 */
var CsoundObj;
