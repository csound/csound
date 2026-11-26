#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csound_standard_types.h"
#include "swtdat.h"
#include "wavelets.h"
#include "gtest/gtest.h"

#include <cstdlib>
#include <cstring>

namespace {

SWTDAT makeFrame() {
  SWTDAT frame;
  std::memset(&frame, 0, sizeof(frame));
  frame.framesize = 64;
  frame.hopsize = 32;
  frame.levels = 3;
  frame.waveletFamily = SWT_FAMILY_COIFLET;
  frame.waveletOrder = 6;
  frame.boundaryMode = SWT_BOUNDARY_PERIODIC;
  frame.windowPair = SWT_WINDOW_SQRT_HANN;
  frame.olaMode = SWT_OLA_OVERLAP_ADD;
  frame.latency = 64;
  frame.capacity = 256;
  frame.historyCount = 5;
  frame.framecount = 7;
  frame.revision = 9;
  frame.generation = 11;
  frame.startSample = 1234;
  frame.sampleRate = 44100.0;
  frame.frame.size = 256 * sizeof(cs_float);
  frame.frame.auxp = std::malloc(frame.frame.size);
  frame.history.size = 1280 * sizeof(cs_float);
  frame.history.auxp = std::malloc(frame.history.size);
  for (int32_t i = 0; i < 256; ++i)
    static_cast<cs_float *>(frame.frame.auxp)[i] = 0.25 * i;
  for (int32_t i = 0; i < 1280; ++i)
    static_cast<cs_float *>(frame.history.auxp)[i] = -0.5 * i;
  return frame;
}

void freeFrame(SWTDAT &frame) {
  std::free(frame.frame.auxp);
  std::free(frame.history.auxp);
}

TEST(SwtDataAccessors, ReturnsMetadataAndBandLayout) {
  SWTDAT frame = makeFrame();

  EXPECT_EQ(csoundSwtDataFrameSize(nullptr), 0);
  EXPECT_EQ(csoundSwtDataHopSize(nullptr), 0);
  EXPECT_EQ(csoundSwtDataLevels(nullptr), 0);
  EXPECT_EQ(csoundSwtDataWaveletFamily(nullptr), 0);
  EXPECT_EQ(csoundSwtDataWaveletOrder(nullptr), 0);
  EXPECT_EQ(csoundSwtDataBoundaryMode(nullptr), 0);
  EXPECT_EQ(csoundSwtDataWindowPair(nullptr), 0);
  EXPECT_EQ(csoundSwtDataOlaMode(nullptr), 0);
  EXPECT_EQ(csoundSwtDataLatency(nullptr), 0);
  EXPECT_EQ(csoundSwtDataCapacity(nullptr), 0);
  EXPECT_EQ(csoundSwtDataSampleRate(nullptr), 0.0);
  EXPECT_EQ(csoundSwtDataFramecount(nullptr), 0u);
  EXPECT_EQ(csoundSwtDataRevision(nullptr), 0u);
  EXPECT_EQ(csoundSwtDataBandOffset(nullptr, 0), -1);
  EXPECT_EQ(csoundSwtDataBandSize(nullptr, 0), 0);
  EXPECT_EQ(csoundGetSwtData(nullptr), nullptr);

  EXPECT_EQ(csoundSwtDataFrameSize(&frame), 64);
  EXPECT_EQ(csoundSwtDataHopSize(&frame), 32);
  EXPECT_EQ(csoundSwtDataLevels(&frame), 3);
  EXPECT_EQ(csoundSwtDataWaveletFamily(&frame), SWT_FAMILY_COIFLET);
  EXPECT_EQ(csoundSwtDataWaveletOrder(&frame), 6);
  EXPECT_EQ(csoundSwtDataBoundaryMode(&frame), SWT_BOUNDARY_PERIODIC);
  EXPECT_EQ(csoundSwtDataWindowPair(&frame), SWT_WINDOW_SQRT_HANN);
  EXPECT_EQ(csoundSwtDataOlaMode(&frame), SWT_OLA_OVERLAP_ADD);
  EXPECT_EQ(csoundSwtDataLatency(&frame), 64);
  EXPECT_EQ(csoundSwtDataCapacity(&frame), 256);
  EXPECT_EQ(csoundSwtDataSampleRate(&frame), 44100.0);
  EXPECT_EQ(csoundSwtDataFramecount(&frame), 7u);
  EXPECT_EQ(csoundSwtDataRevision(&frame), 9u);

  EXPECT_EQ(csoundSwtDataBandOffset(&frame, -1), 0);
  EXPECT_EQ(csoundSwtDataBandSize(&frame, -1), 256);
  EXPECT_EQ(csoundSwtDataBandOffset(&frame, 0), 0);
  EXPECT_EQ(csoundSwtDataBandSize(&frame, 0), 64);
  EXPECT_EQ(csoundSwtDataBandOffset(&frame, 3), 192);
  EXPECT_EQ(csoundSwtDataBandSize(&frame, 3), 64);
  EXPECT_EQ(csoundSwtDataBandOffset(&frame, 4), -1);
  EXPECT_EQ(csoundSwtDataBandSize(&frame, 4), 0);
  EXPECT_EQ(csoundGetSwtData(&frame), frame.frame.auxp);

  freeFrame(frame);
}

TEST(SwtDataAccessors, CopyValuePreservesFrameAndHistory) {
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  ASSERT_NE(csound, nullptr);
  ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundCompileOrc(csound,
                             "sr = 44100\nksmps = 32\nnchnls = 1\n0dbfs = 1\n",
                             0), CSOUND_SUCCESS);
  ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
  SWTDAT source = makeFrame();
  SWTDAT destination;
  std::memset(&destination, 0, sizeof(destination));

  void *auxchBefore = csound->curip != nullptr ? csound->curip->auxchp : nullptr;
  CS_VAR_TYPE_SWT.copyValue(csound, &CS_VAR_TYPE_SWT, &destination, &source,
                             nullptr);

  if (csound->curip != nullptr)
    EXPECT_EQ(csound->curip->auxchp, auxchBefore);
  EXPECT_EQ(csoundSwtDataFrameSize(&destination), 64);
  EXPECT_EQ(csoundSwtDataLevels(&destination), 3);
  EXPECT_EQ(csoundSwtDataCapacity(&destination), 256);
  EXPECT_EQ(csoundSwtDataFramecount(&destination), 7u);
  EXPECT_EQ(csoundSwtDataRevision(&destination), 9u);
  EXPECT_EQ(destination.generation, 11u);
  EXPECT_EQ(destination.startSample, 1234);
  EXPECT_NE(destination.frame.auxp, source.frame.auxp);
  EXPECT_NE(destination.history.auxp, source.history.auxp);
  EXPECT_EQ(std::memcmp(destination.frame.auxp, source.frame.auxp,
                        source.frame.size), 0);
  EXPECT_EQ(std::memcmp(destination.history.auxp, source.history.auxp,
                        source.history.size), 0);

  static_cast<cs_float *>(source.frame.auxp)[0] = 99.0;
  static_cast<cs_float *>(source.history.auxp)[0] = -99.0;
  EXPECT_NE(static_cast<cs_float *>(destination.frame.auxp)[0], 99.0);
  EXPECT_NE(static_cast<cs_float *>(destination.history.auxp)[0], -99.0);

  CS_VAR_TYPE_SWT.freeVariableMemory(csound, &destination);
  if (csound->curip != nullptr)
    EXPECT_EQ(csound->curip->auxchp, auxchBefore);
  freeFrame(source);
  csoundDestroy(csound);
}

TEST(SwtDataHistory, NoHistoryAllowsOnlyCurrentFrame) {
  SWTDAT frame = makeFrame();
  frame.historyCount = 0;
  frame.framecount = 1;

  EXPECT_EQ(swt_history_overrun(&frame, 0), 0);
  EXPECT_EQ(swt_history_overrun(&frame, 1), 0);

  frame.framecount = 2;
  EXPECT_NE(swt_history_overrun(&frame, 0), 0);
  EXPECT_EQ(swt_history_overrun(&frame, 1), 0);

  frame.historyCount = 5;
  frame.framecount = 6;
  EXPECT_EQ(swt_history_overrun(&frame, 1), 0);
  frame.framecount = 7;
  EXPECT_NE(swt_history_overrun(&frame, 1), 0);

  freeFrame(frame);
}

}
