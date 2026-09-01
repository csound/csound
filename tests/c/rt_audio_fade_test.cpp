#include <gtest/gtest.h>

#include "../../InOut/rt_audio_fade.h"

TEST(RtAudioFadeTest, AppliesOneGainPerInterleavedFrame)
{
  RT_AUDIO_FADE fade;
  MYFLT samples[] = {
    (MYFLT) 1.0, (MYFLT) 1.0,
    (MYFLT) 1.0, (MYFLT) 1.0,
    (MYFLT) 1.0, (MYFLT) 1.0,
    (MYFLT) 1.0, (MYFLT) 1.0
  };

  rt_audio_fade_begin(&fade, 8, 2);
  rt_audio_fade_apply(&fade, samples, 8, 2);

  EXPECT_DOUBLE_EQ((double) samples[0], 1.0);
  EXPECT_DOUBLE_EQ((double) samples[1], 1.0);
  EXPECT_NEAR((double) samples[2], 2.0 / 3.0, 1.0e-6);
  EXPECT_NEAR((double) samples[3], 2.0 / 3.0, 1.0e-6);
  EXPECT_NEAR((double) samples[4], 1.0 / 3.0, 1.0e-6);
  EXPECT_NEAR((double) samples[5], 1.0 / 3.0, 1.0e-6);
  EXPECT_DOUBLE_EQ((double) samples[6], 0.0);
  EXPECT_DOUBLE_EQ((double) samples[7], 0.0);
}

TEST(RtAudioFadeTest, KeepsFollowingFramesSilent)
{
  RT_AUDIO_FADE fade;
  MYFLT samples[] = {(MYFLT) 1.0, (MYFLT) 1.0};

  rt_audio_fade_begin(&fade, 1, 1);
  rt_audio_fade_apply(&fade, samples, 1, 1);
  rt_audio_fade_apply(&fade, &samples[1], 1, 1);

  EXPECT_DOUBLE_EQ((double) samples[0], 0.0);
  EXPECT_DOUBLE_EQ((double) samples[1], 0.0);
}
