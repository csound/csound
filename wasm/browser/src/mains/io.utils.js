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

export const requestMicrophoneNode = async () => {
  console.log("requesting microphone access");

  if (navigator.mediaDevices && navigator.mediaDevices.getUserMedia) {
    return navigator.mediaDevices.getUserMedia({
      audio: { echoCancellation: false, sampleSize: 32 },
    });
  }

  const legacyGetUserMedia =
    navigator.getUserMedia || navigator.webkitGetUserMedia || navigator.mozGetUserMedia;

  if (!legacyGetUserMedia) {
    throw new Error(
      "Microphone input is unavailable. Use HTTPS, localhost, or a loopback address and allow microphone access.",
    );
  }

  return new Promise((resolve, reject) => {
    legacyGetUserMedia.call(
      navigator,
      {
        audio: {
          optional: [{ echoCancellation: false, sampleSize: 32 }],
        },
      },
      resolve,
      reject,
    );
  });
};

// rebind this to exportApi instance to use
/**
 * Requests browser microphone access and connects the stream to Csound.
 * This requires HTTPS, localhost, or a loopback address. Worker modes should
 * request input with `-iadc` before calling `start()` instead.
 *
 * @function
 * @returns {Promise<void>} Resolves after the microphone stream is connected.
 */
export async function enableAudioInput() {
  console.log("enabling audio input");
  const stream = await requestMicrophoneNode();
  const audioContext = await this["getAudioContext"]();
  const liveInput = audioContext.createMediaStreamSource(stream);
  this.inputsCount = liveInput.channelCount;

  const node = await this["getNode"]();
  liveInput.connect(node);
}
