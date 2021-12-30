// catches chromium and chrome

import LoggerFactoryNS from "lines-logger/lib/index.js";

// console.log("LOGGERF", LoggerFactory);
// const hasColors =
//   typeof chrome !== "undefined" ||
//   (typeof navigator === "object" && navigator.vendor && navigator.vendor.includes("Google"));

const loggerFactory = new LoggerFactoryNS.LoggerFactory();
// const noStyleLoggerFactory = new LoggerFactory();

// noStyleLoggerFactory.getColorStyle = () => "";

const indexLogger = loggerFactory.getLoggerColor("index.js", "#1E88E5");
const workletMainLogger = loggerFactory.getLoggerColor("WorkletMain", "#6D4C41");
const workletWorkerLogger = loggerFactory.getLoggerColor("WorkletWorker", "#D7CCC8");
const sabMainLogger = loggerFactory.getLoggerColor("Sab_Main", "#000000");
const sabWorkerLogger = loggerFactory.getLoggerColor("Sab_Worker", "#222222");
const vanMainLogger = loggerFactory.getLoggerColor("Van_Main", "#F4511E");
const vanWorkerLogger = loggerFactory.getLoggerColor("Van_Worker", "#FFAB91");
const oldSpnMainLogger = loggerFactory.getLoggerColor("OldSpn_Main", "#8E24AA");
const oldSpnWorkerLogger = loggerFactory.getLoggerColor("OldSpn_Worker", "#E1BEE7");
const singleWorkletMainLogger = loggerFactory.getLoggerColor("WorkletSinglethread_Main", "#1E88E5");
const singleWorkletWorkerLogger = loggerFactory.getLoggerColor(
  "WorkletSinglethread_Worker",
  "#90CAF9",
);
const commonUtilsLogger = loggerFactory.getLoggerColor("common.utils.js", "#FFD600");
const wasmModuleLogger = loggerFactory.getLoggerColor("module.js", "#FFF59D");
const midiRequestLogger = loggerFactory.getLoggerColor("utils/request-midi.js", "#FFD600");

export const logWorkletMain = workletMainLogger.log;
export const logWorkletWorker = workletWorkerLogger.log;
export const logSABMain = sabMainLogger.log;
export const logSABWorker = sabWorkerLogger.log;
export const logVANMain = vanMainLogger.log;
export const logVANWorker = vanWorkerLogger.log;
export const logOldSpnMain = oldSpnMainLogger.log;
export const logOldSpnWorker = oldSpnWorkerLogger.log;
export const logIndex = indexLogger.log;
export const logSPNMainSingle = (...argz) => () => console.log(...argz); // TODO
export const logSinglethreadWorkletMain = singleWorkletMainLogger.log;
export const logSinglethreadWorkletWorker = singleWorkletWorkerLogger.log;
export const logCommonUtils = commonUtilsLogger.log;
export const logWasmModule = wasmModuleLogger.log;
export const logMidiRequest = midiRequestLogger.log;

/**
 * @supress JSC_WRONG_ARGUMENT_COUNT
 */
export default () => (ignore) => {};
