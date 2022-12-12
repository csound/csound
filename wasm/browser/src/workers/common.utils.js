import { logCommonUtils as log } from "../logger.js";

export const handleCsoundStart =
  (workerMessagePort, libraryCsound, wasi, createRealtimeAudioThread, renderFunction) =>
  (arguments_) => {
    const { csound } = arguments_;

    // If no orchestra was given, we assume realtime daemon mode
    // otherwise we'll end up rendering forever within the while loop
    const shouldDemonize = libraryCsound.csoundShouldDaemonize(csound) === 1;

    if (shouldDemonize) {
      libraryCsound.csoundSetOption(csound, "--daemon");
      libraryCsound.csoundSetOption(csound, "-odac");
    }
    const startError = libraryCsound.csoundStart(csound);

    const outputName = libraryCsound.csoundGetOutputName(csound) || "test.wav";
    log(`handleCsoundStart: actual csoundStart result ${startError}, outputName: ${outputName}`)();
    if (startError !== 0) {
      workerMessagePort.post(
        `error: csoundStart failed while trying to render ${outputName},` +
          " look out for errors in options and syntax",
      );
    }

    setTimeout(() => {
      const isRequestingRtMidiInput = libraryCsound._isRequestingRtMidiInput(csound);
      const isExpectingRealtimeOutput =
        shouldDemonize || isRequestingRtMidiInput || outputName.includes("dac");

      if (isExpectingRealtimeOutput) {
        createRealtimeAudioThread(arguments_);
      } else {
        // Do rendering
        workerMessagePort.broadcastPlayState("renderStarted");
        if (renderFunction) {
          renderFunction(arguments_);
        } else {
          // eslint-disable-next-line no-empty
          while (libraryCsound.csoundPerformKsmps(csound) === 0) {}
        }
      }
    }, 0);

    return startError;
  };

export const instantiateAudioPacket = (numberChannels, numberFrames) => {
  const channels = [];
  for (let chn = 0; chn < numberChannels; chn++) {
    channels.push(new Float64Array(numberFrames));
  }
  return channels;
};

export const renderFunction =
  ({ libraryCsound, workerMessagePort, wasi }) =>
  async ({ csound }) => {
    const kr = libraryCsound.csoundGetKr(csound);
    let lastResult = 0;
    let cnt = 0;

    while (
      (workerMessagePort.vanillaWorkerState === "renderStarted" ||
        workerMessagePort.workerState === "renderStarted") &&
      lastResult === 0
    ) {
      lastResult = libraryCsound.csoundPerformKsmps(csound);
      cnt += 1;

      if (typeof setTimeout === "function" && lastResult === 0 && cnt % (kr * 2) === 0) {
        // this is immediately executed, but allows events to be picked up
        await new Promise((resolve) => setTimeout(resolve, 0));
      }
    }
    workerMessagePort.broadcastPlayState("renderEnded");
  };
