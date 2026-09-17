export const nextAudioFadeGain = (fade) => {
  const gain =
    fade.lengthFrames <= 1 || fade.positionFrames >= fade.lengthFrames
      ? 0
      : (fade.lengthFrames - fade.positionFrames - 1) /
        (fade.lengthFrames - 1);
  fade.positionFrames += 1;
  return gain;
};

export const createAudioFade = (lengthFrames) => ({
  lengthFrames: Math.max(1, Math.floor(Number(lengthFrames) || 0)),
  positionFrames: 0,
});

export const getAudioFadeRemainingFrames = (fade) =>
  fade ? Math.max(0, fade.lengthFrames - fade.positionFrames) : 0;

export const applyAudioFade = (fade, channels, frameCount = channels[0]?.length || 0) => {
  if (!fade || !channels || channels.length === 0 || frameCount <= 0) {
    return;
  }

  const frames = Math.min(frameCount, ...channels.map((channel) => channel.length));
  for (let frame = 0; frame < frames; frame++) {
    const gain = nextAudioFadeGain(fade);
    channels.forEach((channel) => {
      channel[frame] *= gain;
    });
  }
};

export const fillAudioFade = (
  fade,
  channels,
  values,
  frameCount = channels[0]?.length || 0,
) => {
  if (!fade || !channels || channels.length === 0 || frameCount <= 0) {
    return;
  }

  const frames = Math.min(frameCount, ...channels.map((channel) => channel.length));
  for (let frame = 0; frame < frames; frame++) {
    const gain = nextAudioFadeGain(fade);
    channels.forEach((channel, channelIndex) => {
      channel[frame] = (values[channelIndex] || 0) * gain;
    });
  }
};
