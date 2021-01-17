// import TextEncoderShim from 'text-encoding-shim';

export const appendBuffers = (buffer1, buffer2) => {
  const temporary = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
  temporary.set(new Uint8Array(buffer1), 0);
  temporary.set(new Uint8Array(buffer2), buffer1.byteLength);
  return temporary.buffer;
};

export const freeStringPtr = (wasm, ptr) => {
  wasm.exports.freeStringMem(ptr);
};

export const nearestPowerOf2 = (n) => {
  return 1 << (31 - Math.clz32(n));
};

export const isIos = () => /iPhone|iPad|iPod/.test(navigator.userAgent);

const isFirefox = () => navigator.userAgent.toLowerCase().includes("firefox");

export const isSabSupported = () =>
  !isFirefox() &&
  typeof window.Atomics !== "undefined" &&
  typeof window.SharedArrayBuffer !== "undefined";

export const areWorkletsSupported = () =>
  typeof AudioNode !== "undefined" && typeof AudioWorkletNode !== "undefined";

export const WebkitAudioContext = () => {
  if (typeof window.webkitAudioContext !== "undefined") {
    return window.webkitAudioContext;
  } else if (typeof window.AudioContext !== "undefined") {
    return window.AudioContext;
  }
};

export const isScriptProcessorNodeSupported = () => {
  const audioContext = WebkitAudioContext();
  return (
    typeof audioContext !== "undefined" &&
    typeof audioContext.prototype.createScriptProcessor !== "undefined"
  );
};

export const csoundApiRename = (apiName) => {
  const minusCsound = apiName.replace(/^csound/i, "");
  return minusCsound.charAt(0).toLowerCase() + minusCsound.slice(1);
};

export const makeProxyCallback = (proxyPort, csoundInstance, apiK) => async (...arguments_) => {
  return await proxyPort.callUncloned(apiK, [csoundInstance, ...arguments_]);
};

export const makeSingleThreadCallback = (csoundInstance, apiCallback) => async (...arguments_) => {
  return await apiCallback.apply({}, [csoundInstance, ...arguments_]);
  // return await proxyPort.callUncloned(apiK, [csoundInstance, ...arguments_]);
};

export const stopableStates = new Set([
  "realtimePerformanceStarted",
  "realtimePerformancePaused",
  "realtimePerformanceResumed",
  "renderStarted",
]);

export const fetchPlugins = async (withPlugins) => {
  return await Promise.all(
    withPlugins.map(async (url) => {
      const response = await fetch(url);
      return response.arrayBuffer();
    }),
  );
};
