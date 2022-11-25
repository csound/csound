export const WebkitAudioContext = () => {
  if (window.webkitAudioContext !== undefined) {
    return window.webkitAudioContext;
  } else if (window.AudioContext !== undefined) {
    return window.AudioContext;
  }
};

export const newAudioContext = () => {
  const AudioCTX = WebkitAudioContext();
  if (AudioCTX) {
    return new AudioCTX();
  }
};
