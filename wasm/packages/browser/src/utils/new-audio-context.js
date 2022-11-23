export const WebkitAudioContext = () => {
  if (typeof window.webkitAudioContext !== "undefined") {
    return window.webkitAudioContext;
  } else if (typeof window.AudioContext !== "undefined") {
    return window.AudioContext;
  }
};

export const newAudioContext = () => {
  const AudioCTX = WebkitAudioContext();
  if (AudioCTX) {
    return new AudioCTX();
  }
}
