#include "csound.h"
#include "gtest/gtest.h"

#include <cstring>

extern "C" int32_t csound_test_alsaseq_device_list(CSOUND *, CS_MIDIDEVICE *,
                                                   int32_t);

TEST(AlsaSeqDeviceTests, OpenFailureReportsNoDevices)
{
    CSOUND *csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundSetMIDIDeviceListCallback(csound, csound_test_alsaseq_device_list);

    for (int32_t isOutput = 0; isOutput <= 1; ++isOutput) {
        SCOPED_TRACE(isOutput);
        EXPECT_EQ(csoundGetMIDIDevList(csound, nullptr, isOutput), 0);

        // A sequencer can disappear between the count and fill calls.
        CS_MIDIDEVICE device;
        std::memset(&device, 0xa5, sizeof(device));
        unsigned char before[sizeof(device)];
        std::memcpy(before, &device, sizeof(device));
        EXPECT_EQ(csoundGetMIDIDevList(csound, &device, isOutput), 0);
        EXPECT_EQ(std::memcmp(before, &device, sizeof(device)), 0);
    }

    csoundDestroy(csound);
}
