export const requestMicrophoneNode = (microphoneCallback) => {
  const getUserMedia =
    navigator.mediaDevices === undefined
      ? navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia
      : navigator.mediaDevices.getUserMedia;

  console.log("requesting microphone access");
  navigator.mediaDevices === undefined
    ? getUserMedia.call(
        navigator,
        {
          audio: {
            optional: [{ echoCancellation: false, sampleSize: 32 }],
          },
        },
        microphoneCallback,
        console.error,
      )
    : getUserMedia
        .call(navigator.mediaDevices, {
          audio: { echoCancellation: false, sampleSize: 32 },
        })
        .then(microphoneCallback)
        .catch(console.error);
};

// rebind this to exportApi instance to use
/**
 * @function
 * @this {{
 * getNode: function(): Promise.<Object>,
 * getAudioContext: function(): Promise.<Object>,
 * }}
 */
export async function enableAudioInput() {
  console.log("enabling audio input");
  requestMicrophoneNode(async (stream) => {
    if (stream) {
      const audioContext = await this.getAudioContext();
      const liveInput = audioContext.createMediaStreamSource(stream);
      this.inputsCount = liveInput.channelCount;

      const node = await this.getNode();
      liveInput.connect(node);
    }
  });
}
