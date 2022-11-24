export const appendBuffers = (buffer1, buffer2) => {
  const temporary = new Uint8Array(buffer1.byteLength + buffer2.byteLength);
  temporary.set(new Uint8Array(buffer1), 0);
  temporary.set(new Uint8Array(buffer2), buffer1.byteLength);
  return temporary.buffer;
};

export const nearestPowerOf2 = (n) => {
  return 1 << (31 - Math.clz32(n));
};

export const isIos = () => /iPhone|iPad|iPod/.test(navigator.userAgent);

const isFirefox = () => navigator.userAgent.toLowerCase().includes("firefox");

export const isSafari = () =>
  typeof navigator.vendor === "string" && navigator.vendor.includes("Apple");

export const isSabSupported = () =>
  !isFirefox() && window && window.Atomics !== undefined && window.SharedArrayBuffer !== undefined;

export const areWorkletsSupported = () => AudioNode !== undefined && AudioWorkletNode !== undefined;

export const WebkitAudioContext = () => {
  if (window.webkitAudioContext !== undefined) {
    return window.webkitAudioContext;
  } else if (window.AudioContext !== undefined) {
    return window.AudioContext;
  }
};

export const isScriptProcessorNodeSupported = () => {
  const audioContext = WebkitAudioContext();
  return audioContext !== undefined && audioContext.prototype.createScriptProcessor !== undefined;
};

export const csoundApiRename = (apiName) => {
  let minusCsound = apiName.replace(/^csound/i, "");
  if (apiName === "csoundPushMidiMessage") {
    minusCsound = "midiMessage";
  }
  return minusCsound.charAt(0).toLowerCase() + minusCsound.slice(1);
};

export const stopableStates = new Set([
  "realtimePerformanceStarted",
  "realtimePerformancePaused",
  "realtimePerformanceResumed",
  "renderStarted",
]);

export const makeProxyCallback =
  (proxyPort, csoundInstance, apiK, playState) =>
  async (...arguments_) => {
    if (!playState || !stopableStates.has(playState)) {
      const modifiedFs = {}; // getModifiedPersistentStorage();
      Object.values(modifiedFs).length > 0 &&
        (await proxyPort.callUncloned("syncWorkerFs", [csoundInstance, modifiedFs]));
    }
    return await proxyPort.callUncloned(apiK, [csoundInstance, ...arguments_]);
  };

export const makeSingleThreadCallback =
  (csoundInstance, apiCallback) =>
  async (...arguments_) => {
    return await apiCallback.apply({}, [csoundInstance, ...arguments_]);
  };

export const fetchPlugins = async (withPlugins) => {
  return await Promise.all(
    withPlugins.map(async (url) => {
      const response = await fetch(url);
      return response.arrayBuffer();
    }),
  );
};
