import ololog from "ololog";
import { bgLightCyan, bgLightMagenta, bgBlack, bgYellow, yellow, white } from "ansicolor";
import { isWebWorker } from "browser-or-node";

const defaultLogger = ololog.configure({
  tag: true,
  time: {
    yes: true,
    format: "iso",
    print: (date) =>
      `${date.getHours()}:${date.getMinutes()}:${date.getSeconds()}.${date.getMilliseconds()}`,
  },
  locate: { shift: 4 },
});

export const logWorklet = (...argumentz) =>
  defaultLogger.info.apply(undefined, [`${bgLightCyan("AudioWorklet")}`].concat(argumentz));

export const logSPN = (...argumentz) =>
  defaultLogger.info.apply(
    undefined,
    [`${bgLightMagenta("ScriptProcessorNode")}`].concat(argumentz),
  );

export const logSAB = (...argumentz) =>
  defaultLogger.info.apply(undefined, [`${bgBlack(white("SAB"))}`].concat(argumentz));

export const logVAN = (...argumentz) =>
  defaultLogger.info.apply(
    undefined,
    [isWebWorker ? `${yellow("VANILLA")}` : `${bgYellow(white("VANILLA"))}`].concat(argumentz),
  );

export default defaultLogger;
