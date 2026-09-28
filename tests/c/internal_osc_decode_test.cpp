#include <algorithm>
#include <array>
#include <string>
#include "gtest/gtest.h"
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"

extern "C" {
const char *OSC_message_get_number(const char *, char, cs_float *);
const char *csoundOSCMessageGetString(const char *, STRINGDAT *);
}

TEST(InternalOscDecodeTests, MixedNumbersPreserveInputAndAdvance)
{
    // OSC permits an eight-byte value after a four-byte value.
    alignas(8) const unsigned char bytes[] = {
        0x3f, 0xc0, 0x00, 0x00,                         // float: 1.5
        0xc0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // double: -2.25
        0xff, 0xff, 0xff, 0xf9,                         // int32: -7
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, // int64: -9
        0x00, 0x00, 0x00, 0x41                          // char: A
    };
    const char *data = reinterpret_cast<const char *>(bytes);
    const std::string original(data, sizeof(bytes));
    const char *types = "fdihc";
    const std::array<cs_float, 5> expected = {1.5, -2.25, -7, -9, 65};
    const std::array<size_t, 5> offsets = {4, 12, 16, 24, 28};
    const char *next = data;
    for (size_t i = 0; i < expected.size(); ++i) {
        cs_float value = 0;
        next = OSC_message_get_number(next, types[i], &value);
        ASSERT_EQ(next, data + offsets[i]);
        EXPECT_EQ(value, expected[i]);
    }
    EXPECT_EQ(std::string(data, sizeof(bytes)), original);
}

TEST(InternalOscDecodeTests, StringsKeepCapacityAndAdvancePastFullInput)
{
    const char data[] = "abcdefg\0\0\0\0\x2a";
    for (int32_t capacity : {1, 4, 8, 12}) {
        SCOPED_TRACE(capacity);
        std::array<char, 16> output;
        output.fill('z');
        STRINGDAT value = {};
        value.data = output.data();
        value.size = capacity;

        const char *next = csoundOSCMessageGetString(data, &value);

        EXPECT_EQ(value.data, output.data());
        EXPECT_EQ(value.size, capacity);
        EXPECT_EQ(std::string(value.data),
                  std::string("abcdefg").substr(0, capacity - 1));
        EXPECT_TRUE(std::all_of(output.begin() + capacity, output.end(),
                                [](char c) { return c == 'z'; }));
        ASSERT_EQ(next, data + 8);
        cs_float number = 0;
        EXPECT_EQ(OSC_message_get_number(next, 'i', &number), data + 12);
        EXPECT_EQ(number, 42);
    }
}

TEST(InternalOscDecodeTests, EmptyStringsAndOutputsNeedNoAllocation)
{
    const char data[] = "\0\0\0\0";
    STRINGDAT value = {};
    EXPECT_EQ(csoundOSCMessageGetString(data, &value), data + 4);
    EXPECT_EQ(value.data, nullptr);
    EXPECT_EQ(value.size, 0);

    char output = 'z';
    value.data = &output;
    value.size = 1;
    EXPECT_EQ(csoundOSCMessageGetString(data, &value), data + 4);
    EXPECT_EQ(output, '\0');
    EXPECT_EQ(value.data, &output);
    EXPECT_EQ(value.size, 1);
}
