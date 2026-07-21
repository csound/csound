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

export async function requestMicrophoneStream() {
  if (this.microphoneStream) {
    return this.microphoneStream;
  }

  if (!this.microphonePromise) {
    const microphonePromise = requestMicrophoneNode().then((stream) => {
      this.microphoneStream = stream;
      return stream;
    });
    this.microphonePromise = microphonePromise;

    try {
      return await microphonePromise;
    } finally {
      if (this.microphonePromise === microphonePromise) {
        delete this.microphonePromise;
      }
    }
  }

  return this.microphonePromise;
}

export const releaseMicrophoneStream = (owner) => {
  if (owner.microphoneInput) {
    owner.microphoneInput.disconnect();
    delete owner.microphoneInput;
  }
  if (owner.microphoneStream) {
    owner.microphoneStream.getTracks().forEach((track) => track.stop());
    delete owner.microphoneStream;
  }
  delete owner.microphonePromise;
};

const setAudioInputOption = async (api) => {
  const result = await api["setOption"]("-iadc");
  if (result !== 0) {
    throw new Error(`Could not set the Csound microphone input option: ${result}`);
  }
};

// rebind this to exportApi instance to use
/**
 * Sets `-iadc`, requests browser microphone access, and connects the stream to
 * Csound. This requires HTTPS, localhost, or a loopback address.
 *
 * @function
 * @returns {Promise<void>} Resolves after the microphone stream is connected.
 */
export async function enableAudioInput() {
  console.log("enabling audio input");
  await setAudioInputOption(this);
  const stream = await requestMicrophoneStream.call(this);

  if (this.microphoneInput) {
    return;
  }

  const audioContext = await this["getAudioContext"]();
  const liveInput = audioContext.createMediaStreamSource(stream);
  this.microphoneInput = liveInput;
  this.inputsCount = liveInput.channelCount;

  const node = await this["getNode"]();
  liveInput.connect(node);
}

export async function enableAudioInputInWorker() {
  if (this.currentPlayState) {
    throw new Error("enableAudioInput() must be called before start() in worker mode");
  }

  await setAudioInputOption(this.exportApi);
  await this.audioWorker["requestMicrophoneInput"]();
}
