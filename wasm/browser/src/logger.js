import LoggerFactoryNS from "lines-logger/lib/index.js";

let logWorkletMain,
  logWorkletWorker,
  logSABMain,
  logSABWorker,
  logVANMain,
  logVANWorker,
  logOldSpnMain,
  logOldSpnWorker,
  logIndex,
  logSPNMainSingle,
  logSinglethreadWorkletMain,
  logSinglethreadWorkletWorker,
  logCommonUtils,
  logWasmModule,
  logMidiRequest;

/** @define {boolean} */
const isProd = goog.define("PRODUCTION", false);

if (isProd) {
  /**
   * @suppress {checkTypes}
   */
  logWorkletMain = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logWorkletWorker = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logSABMain = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logSABWorker = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logVANMain = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logVANWorker = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logOldSpnMain = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logOldSpnWorker = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logIndex = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logSPNMainSingle = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logSinglethreadWorkletMain = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logSinglethreadWorkletWorker = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logCommonUtils = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logWasmModule = (ignore1) => (ignore2) => {};
  /**
   * @suppress {checkTypes}
   */
  logMidiRequest = (ignore1) => (ignore2) => {};
} else {
  // catches chromium and chrome

  const loggerFactory = new LoggerFactoryNS.LoggerFactory();

  const indexLogger = loggerFactory.getLoggerColor("index.js", "#1E88E5");
  const workletMainLogger = loggerFactory.getLoggerColor("WorkletMain", "#6D4C41");
  const workletWorkerLogger = loggerFactory.getLoggerColor("WorkletWorker", "#D7CCC8");
  const sabMainLogger = loggerFactory.getLoggerColor("Sab_Main", "#000000");
  const sabWorkerLogger = loggerFactory.getLoggerColor("Sab_Worker", "#222222");
  const vanMainLogger = loggerFactory.getLoggerColor("Van_Main", "#F4511E");
  const vanWorkerLogger = loggerFactory.getLoggerColor("Van_Worker", "#FFAB91");
  const oldSpnMainLogger = loggerFactory.getLoggerColor("OldSpn_Main", "#8E24AA");
  const oldSpnWorkerLogger = loggerFactory.getLoggerColor("OldSpn_Worker", "#E1BEE7");
  const singleWorkletMainLogger = loggerFactory.getLoggerColor(
    "WorkletSinglethread_Main",
    "#1E88E5",
  );
  const singleWorkletWorkerLogger = loggerFactory.getLoggerColor(
    "WorkletSinglethread_Worker",
    "#90CAF9",
  );
  const commonUtilsLogger = loggerFactory.getLoggerColor("common.utils.js", "#FFD600");
  const wasmModuleLogger = loggerFactory.getLoggerColor("module.js", "#FFF59D");
  const midiRequestLogger = loggerFactory.getLoggerColor("utils/request-midi.js", "#FFD600");

  logWorkletMain = workletMainLogger.log;
  logWorkletWorker = workletWorkerLogger.log;
  logSABMain = sabMainLogger.log;
  logSABWorker = sabWorkerLogger.log;
  logVANMain = vanMainLogger.log;
  logVANWorker = vanWorkerLogger.log;
  logOldSpnMain = oldSpnMainLogger.log;
  logOldSpnWorker = oldSpnWorkerLogger.log;
  logIndex = indexLogger.log;
  logSPNMainSingle =
    (...argz) =>
    () =>
      console.log(...argz); // TODO
  logSinglethreadWorkletMain = singleWorkletMainLogger.log;
  logSinglethreadWorkletWorker = singleWorkletWorkerLogger.log;
  logCommonUtils = commonUtilsLogger.log;
  logWasmModule = wasmModuleLogger.log;
  logMidiRequest = midiRequestLogger.log;
}

/**
 * @suppress {checkTypes}
 */
export default (ignore1) => (ignore2) => {};

export {
  logWorkletMain,
  logWorkletWorker,
  logSABMain,
  logSABWorker,
  logVANMain,
  logVANWorker,
  logOldSpnMain,
  logOldSpnWorker,
  logIndex,
  logSPNMainSingle,
  logSinglethreadWorkletMain,
  logSinglethreadWorkletWorker,
  logCommonUtils,
  logWasmModule,
  logMidiRequest,
};
