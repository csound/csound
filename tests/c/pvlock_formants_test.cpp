#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include "fixtures/pvlock_formant_test_fixture.h"
#include <cmath>
#include <vector>

namespace {

class PvlockFormantTests : public ::testing::Test {
protected:
  CSOUND *csound;

  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
  }

  void TearDown() override { csoundDestroy(csound); }

  void fillPackedFrame(std::vector<cs_float> &frame, int32_t N,
                       const std::vector<cs_float> &logs)
  {
    int32_t M = N/2;
    frame.assign((size_t) N + 2, FL(0.0));
    for (int32_t k = 0; k < M; k++) {
      frame[2*k] = (cs_float) std::exp((double) logs[k]);
      frame[2*k + 1] = FL(0.0);
    }
  }

  cs_float magnitude(const std::vector<cs_float> &frame, int32_t k)
  {
    return (cs_float) std::sqrt((double) (frame[2*k]*frame[2*k] +
                                          frame[2*k + 1]*frame[2*k + 1]));
  }
};

TEST_F(PvlockFormantTests, PackedLifterRemovesHighestTerm)
{
  const int32_t M = 8;
  std::vector<cs_float> storage((size_t) M + 3, FL(0.0));
  cs_float *ceps = storage.data() + 1;
  void *fwd = csound->RealFFTSetup(csound, M, FFT_FWD);
  void *inv = csound->RealFFTSetup(csound, M, FFT_INV);

  ASSERT_NE(fwd, nullptr);
  ASSERT_NE(inv, nullptr);
  for (int32_t k = 0; k < M; k++) ceps[k] = k % 2 ? FL(1.0) : FL(0.0);
  csound->RealFFT(csound, fwd, ceps);
  EXPECT_NEAR(ceps[0], FL(4.0), FL(1e-5));
  EXPECT_NEAR(ceps[1], FL(-4.0), FL(1e-5));

  csound_test_pv_formant_lifter(ceps, M, 2);
  EXPECT_NEAR(ceps[1], FL(0.0), FL(1e-5));
  for (int32_t k = 4; k < M; k++) EXPECT_NEAR(ceps[k], FL(0.0), FL(1e-5));

  csound->RealFFT(csound, inv, ceps);
  for (int32_t k = 0; k < M; k++)
    EXPECT_NEAR(ceps[k], FL(0.5), FL(1e-5)) << "bin " << k;
}

TEST_F(PvlockFormantTests, AlternatingSpectrumProducesFlatEnvelope)
{
  const int32_t N = 16, M = N/2;
  void *handle = csound_test_pv_formant_new(csound, N);
  ASSERT_NE(handle, nullptr);
  std::vector<cs_float> logs((size_t) M);
  std::vector<cs_float> front, back, env((size_t) M);

  for (int32_t k = 0; k < M; k++) logs[k] = k % 2 ? FL(1.0) : FL(0.0);
  fillPackedFrame(front, N, logs);
  back = front;
  csound_test_pv_formant_analyse(csound, handle, N, 1, 2,
                                 front.data(), back.data());
  csound_test_pv_formant_get_env(handle, env.data(), M);
  for (int32_t k = 0; k < M; k++)
    EXPECT_NEAR(env[k], FL(1.0), FL(1e-4)) << "bin " << k;

  csound_test_pv_formant_free(csound, handle);
}

TEST_F(PvlockFormantTests, ModeTwoKeepsSparseHarmonicEnvelopeSmooth)
{
  const int32_t N = 128, M = N/2;
  void *handle = csound_test_pv_formant_new(csound, N);
  ASSERT_NE(handle, nullptr);
  std::vector<cs_float> logs((size_t) M, FL(0.0));
  std::vector<cs_float> front, back, env((size_t) M);

  logs[10] = FL(4.0);
  fillPackedFrame(front, N, logs);
  back = front;
  csound_test_pv_formant_analyse(csound, handle, N, 2, 1,
                                 front.data(), back.data());
  csound_test_pv_formant_get_env(handle, env.data(), M);
  for (int32_t k = 0; k < M; k++)
    EXPECT_NEAR(env[k], FL(1.0), FL(1e-4)) << "bin " << k;

  csound_test_pv_formant_free(csound, handle);
}

TEST_F(PvlockFormantTests, RestoreUsesPitchMappedTargetEnvelope)
{
  const int32_t N = 32, M = N/2;
  void *handle = csound_test_pv_formant_new(csound, N);
  ASSERT_NE(handle, nullptr);
  std::vector<cs_float> env((size_t) M), front((size_t) N + 2, FL(0.0));

  for (int32_t k = 0; k < M; k++) env[k] = FL(0.1)*(cs_float) (k + 1);
  csound_test_pv_formant_set_env(handle, env.data(), M);
  for (int32_t k = 1; k < M; k++) {
    front[2*k] = FL(1.0);
    front[2*k + 1] = FL(0.0);
  }
  csound_test_pv_formant_restore(handle, N, front.data(), FL(2.0));
  for (int32_t k = 1; k < M; k++) {
    int32_t idx = 2*k > M - 1 ? M - 1 : 2*k;
    EXPECT_NEAR(magnitude(front, k), env[idx], FL(1e-5)) << "bin " << k;
  }

  csound_test_pv_formant_free(csound, handle);
}

}
