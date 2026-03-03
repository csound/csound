/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
      const audioContext = await this["getAudioContext"]();
      const liveInput = audioContext.createMediaStreamSource(stream);
      this.inputsCount = liveInput.channelCount;

      const node = await this["getNode"]();
      liveInput.connect(node);
    }
  });
}
