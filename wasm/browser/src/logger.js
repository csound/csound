import _log from "log";
import { bgLightCyan, bgLightMagenta, bgBlack, bgYellow, yellow, white } from "ansicolor";
import { isWebWorker } from "browser-or-node";

export const logWorklet = (...argumentz) =>
  _log.apply(undefined, [`${bgLightCyan("AudioWorklet")}`].concat(argumentz));

export const logSPN = (...argumentz) =>
  _log.apply(undefined, [`${bgLightMagenta("ScriptProcessorNode")}`].concat(argumentz));

export const logSAB = (...argumentz) =>
  _log.apply(undefined, [`${bgBlack(white("SAB"))}`].concat(argumentz));

export const logVAN = (...argumentz) =>
  _log.apply(
    undefined,
    [isWebWorker ? `${yellow("VANILLA")}` : `${bgYellow(white("VANILLA"))}`].concat(argumentz),
  );

export default _log;
