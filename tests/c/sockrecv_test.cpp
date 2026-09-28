#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <vector>
#ifdef HAVE_SOCKETS
extern "C" {
void *csound_test_sockrecv_create(CSOUND *, INSDS *, cs_float *, cs_float *, int32_t);
void csound_test_sockrecv_packet(void *, const cs_float *, int32_t);
int32_t csound_test_sockrecv_perform(void *, int32_t);
void csound_test_sockrecv_destroy(void *);
}

class SockrecvTests : public ::testing::Test {
protected:
    CSOUND *cs;
    void *receiver = nullptr;
    INSDS instance = {};
    cs_float left[6] = {}, right[6] = {};
    void SetUp() override {
      cs = csoundCreate(nullptr, nullptr);
      instance.ksmps = 6;
      instance.ksmps_offset = 1;
      instance.ksmps_no_end = 1;
    }
    void TearDown() override {
      if (receiver) csound_test_sockrecv_destroy(receiver);
      csoundDestroy(cs);
    }
    void expect(const cs_float *output, std::initializer_list<cs_float> values) {
      EXPECT_EQ(std::vector<cs_float>(output, output + 6),
                std::vector<cs_float>(values));
    }
};

TEST_F(SockrecvTests, MonoAndControlUnderrunsStaySilentAndResume)
{
    receiver = csound_test_sockrecv_create(cs, &instance, left, nullptr, 8);
    const cs_float data[] = {FL(0.25), FL(0.5)};
    csound_test_sockrecv_packet(receiver, data, 2);
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 0), OK);
    expect(left, {0, FL(0.25), FL(0.5), 0, 0, 0});
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 0), OK);
    expect(left, {0, 0, 0, 0, 0, 0});

    csound_test_sockrecv_packet(receiver, data, 2);
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 1), OK);
    EXPECT_EQ(left[0], FL(0.25));
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 1), OK);
    EXPECT_EQ(left[0], FL(0.5));
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 1), OK);
    EXPECT_EQ(left[0], FL(0.0));
}

TEST_F(SockrecvTests, StereoQueueKeepsWholeFramesAcrossOverflowAndUnderrun)
{
    receiver = csound_test_sockrecv_create(cs, &instance, left, right, 3);
    const cs_float full[] = {1, 11, 2, 22};
    csound_test_sockrecv_packet(receiver, full, 4);
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 0), OK);
    expect(left, {0, 1, 0, 0, 0, 0});
    expect(right, {0, 11, 0, 0, 0, 0});
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 0), OK);
    expect(left, {0, 0, 0, 0, 0, 0});
    expect(right, {0, 0, 0, 0, 0, 0});

    const cs_float partial[] = {-1, -11, 99};
    csound_test_sockrecv_packet(receiver, partial, 3);
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 0), OK);
    expect(left, {0, -1, 0, 0, 0, 0});
    expect(right, {0, -11, 0, 0, 0, 0});
    csound_test_sockrecv_packet(receiver, full + 2, 2);
    ASSERT_EQ(csound_test_sockrecv_perform(receiver, 0), OK);
    expect(left, {0, 2, 0, 0, 0, 0});
    expect(right, {0, 22, 0, 0, 0, 0});
}
#endif
