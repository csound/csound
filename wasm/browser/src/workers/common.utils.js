import log from "@root/logger";

export const handleCsoundStart = (workerMessagePort, libraryCsound, createRealtimeAudioThread) => (
  _,
  arguments_,
) => {
  const { csound } = arguments_;

  const startError = libraryCsound.csoundStart(csound);
  const outputName = libraryCsound.csoundGetOutputName(csound) || "test.wav";
  log(`handleCsoundStart: actual csoundStart result ${startError}, outputName: ${outputName}`);
  if (startError !== 0) {
    workerMessagePort.post(
      `error: csoundStart failed while trying to render ${outputName},` +
        " look out for errors in options and syntax",
    );
  }

  setTimeout(() => {
    const isRequestingRtMidiInput = libraryCsound._isRequestingRtMidiInput(csound);
    const isExpectingRealtimeOutput = isRequestingRtMidiInput || outputName.includes("dac");

    if (isExpectingRealtimeOutput) {
      createRealtimeAudioThread(arguments_);
    } else {
      // Do rendering
      workerMessagePort.broadcastPlayState("renderStarted");
      while (libraryCsound.csoundPerformKsmps(csound) === 0) {}
      workerMessagePort.broadcastPlayState("renderEnded");
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
