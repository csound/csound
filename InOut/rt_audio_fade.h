#ifndef CSOUND_RT_AUDIO_FADE_H
#define CSOUND_RT_AUDIO_FADE_H

#include "csound.h"

typedef struct RT_AUDIO_FADE_ {
  int32_t lengthFrames;
  int32_t positionFrames;
} RT_AUDIO_FADE;

static inline void rt_audio_fade_reset(RT_AUDIO_FADE *fade)
{
  fade->lengthFrames = 0;
  fade->positionFrames = 0;
}

static inline void rt_audio_fade_begin(RT_AUDIO_FADE *fade,
                                       int32_t queuedSamples,
                                       int32_t channels)
{
  int32_t queuedFrames = channels > 0 ? queuedSamples / channels : 0;
  fade->lengthFrames = queuedFrames > 0 ? queuedFrames : 1;
  fade->positionFrames = 0;
}

static inline void rt_audio_fade_apply(RT_AUDIO_FADE *fade,
                                       MYFLT *samples,
                                       int32_t sampleCount,
                                       int32_t channels)
{
  int32_t frame, channel;
  int32_t frameCount;

  if (samples == NULL || sampleCount <= 0 || channels <= 0)
    return;
  frameCount = sampleCount / channels;
  for (frame = 0; frame < frameCount; frame++) {
    MYFLT gain;
    if (fade->lengthFrames <= 1 ||
        fade->positionFrames >= fade->lengthFrames) {
      gain = (MYFLT) 0.0;
    }
    else {
      gain = (MYFLT) (fade->lengthFrames - fade->positionFrames - 1) /
             (MYFLT) (fade->lengthFrames - 1);
    }
    for (channel = 0; channel < channels; channel++)
      samples[frame * channels + channel] *= gain;
    fade->positionFrames++;
  }
}

#endif
