export const requestMicrophoneNode = (microphoneCallback) => {
  const getUserMedia =
    typeof navigator.mediaDevices !== "undefined"
      ? navigator.mediaDevices.getUserMedia
      : navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia;

  console.log("requesting microphone access");
  typeof navigator.mediaDevices !== "undefined"
    ? getUserMedia
        .call(navigator.mediaDevices, {
          audio: { echoCancellation: false, sampleSize: 32 },
        })
        .then(microphoneCallback)
        .catch(console.error)
    : getUserMedia.call(
        navigator,
        {
          audio: {
            optional: [{ echoCancellation: false, sampleSize: 32 }],
          },
        },
        microphoneCallback,
        console.error,
      );
};

// rebind this to exportApi instance to use
export async function enableAudioInput() {
  console.log("enabling audio input");
  requestMicrophoneNode(async (stream) => {
    if (stream) {
      const liveInput = (await this.getAudioContext()).createMediaStreamSource(stream);
      this.inputsCount = liveInput.channelCount;

      // if (this.autoConnect) {
      const node = await this.getNode();
      liveInput.connect(node);
      // }
    }
  });
}
