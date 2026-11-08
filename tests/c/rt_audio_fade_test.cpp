#include <gtest/gtest.h>

#include "../../InOut/rt_audio_fade.h"

TEST(RtAudioFadeTest, AppliesOneGainPerInterleavedFrame)
{
  RT_AUDIO_FADE fade;
  cs_float samples[] = {
    (cs_float) 1.0, (cs_float) 1.0,
    (cs_float) 1.0, (cs_float) 1.0,
    (cs_float) 1.0, (cs_float) 1.0,
    (cs_float) 1.0, (cs_float) 1.0
  };

  rt_audio_fade_begin(&fade, 8, 2);
  rt_audio_fade_apply(&fade, samples, 8, 2);

  EXPECT_DOUBLE_EQ((cs_double) samples[0], 1.0);
  EXPECT_DOUBLE_EQ((cs_double) samples[1], 1.0);
  EXPECT_NEAR((cs_double) samples[2], 2.0 / 3.0, 1.0e-6);
  EXPECT_NEAR((cs_double) samples[3], 2.0 / 3.0, 1.0e-6);
  EXPECT_NEAR((cs_double) samples[4], 1.0 / 3.0, 1.0e-6);
  EXPECT_NEAR((cs_double) samples[5], 1.0 / 3.0, 1.0e-6);
  EXPECT_DOUBLE_EQ((cs_double) samples[6], 0.0);
  EXPECT_DOUBLE_EQ((cs_double) samples[7], 0.0);
}

TEST(RtAudioFadeTest, KeepsFollowingFramesSilent)
{
  RT_AUDIO_FADE fade;
  cs_float samples[] = {(cs_float) 1.0, (cs_float) 1.0};

  rt_audio_fade_begin(&fade, 1, 1);
  rt_audio_fade_apply(&fade, samples, 1, 1);
  rt_audio_fade_apply(&fade, &samples[1], 1, 1);

  EXPECT_DOUBLE_EQ((cs_double) samples[0], 0.0);
  EXPECT_DOUBLE_EQ((cs_double) samples[1], 0.0);
}

TEST(RtAudioFadeTest, ContinuesAcrossBuffers)
{
  RT_AUDIO_FADE fade;
  cs_float first[] = {(cs_float) 1.0, (cs_float) 1.0};
  cs_float second[] = {(cs_float) 1.0, (cs_float) 1.0};

  rt_audio_fade_begin(&fade, 4, 1);
  rt_audio_fade_apply(&fade, first, 2, 1);
  rt_audio_fade_apply(&fade, second, 2, 1);

  EXPECT_DOUBLE_EQ((cs_double) first[0], 1.0);
  EXPECT_NEAR((cs_double) first[1], 2.0 / 3.0, 1.0e-6);
  EXPECT_NEAR((cs_double) second[0], 1.0 / 3.0, 1.0e-6);
  EXPECT_DOUBLE_EQ((cs_double) second[1], 0.0);
}
