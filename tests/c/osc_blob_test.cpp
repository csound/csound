#include <array>
#include <cstring>
#include <limits>
#include <random>
#include <vector>

#include "gtest/gtest.h"
#include "Opcodes/osc_blob.h"

namespace {

template <typename T>
void appendValue(std::vector<unsigned char>& bytes, const T& value)
{
    const size_t oldSize = bytes.size();
    bytes.resize(oldSize + sizeof(value));
    std::memcpy(bytes.data() + oldSize, &value, sizeof(value));
}

void expectViewInsidePayload(const unsigned char *payload, size_t payloadSize,
                             const OSC_MYFLT_BLOB_VIEW& view)
{
    ASSERT_LE(view.count, payloadSize / sizeof(MYFLT));
    if (view.count != 0) {
        ASSERT_NE(nullptr, view.data);
        EXPECT_GE(view.data, payload);
        EXPECT_LE(view.data + view.count * sizeof(MYFLT),
                  payload + payloadSize);
    }
}

} // namespace

TEST(OscBlobTest, AudioCountIsClampedToCompleteSamplesAndKsmps)
{
    std::vector<unsigned char> storage(1);
    const MYFLT advertised = FL(10.0);
    const std::array<MYFLT, 3> samples = {FL(1.0), FL(2.0), FL(3.0)};
    appendValue(storage, advertised);
    for (const MYFLT sample : samples) {
        appendValue(storage, sample);
    }

    OSC_MYFLT_BLOB_VIEW view{};
    ASSERT_EQ(OK, osc_blob_parse_audio(storage.data() + 1,
                                       storage.size() - 1, 2, &view));
    ASSERT_EQ(2u, view.count);
    std::array<MYFLT, 2> decoded{};
    std::memcpy(decoded.data(), view.data, decoded.size() * sizeof(MYFLT));
    EXPECT_EQ(FL(1.0), decoded[0]);
    EXPECT_EQ(FL(2.0), decoded[1]);
}

TEST(OscBlobTest, AudioRejectsInvalidCountFields)
{
    const std::array<MYFLT, 4> invalidCounts = {
        FL(-1.0),
        FL(1.5),
        std::numeric_limits<MYFLT>::infinity(),
        std::numeric_limits<MYFLT>::quiet_NaN()
    };
    for (const MYFLT count : invalidCounts) {
        OSC_MYFLT_BLOB_VIEW view{};
        EXPECT_EQ(NOTOK,
                  osc_blob_parse_audio(&count, sizeof(count), 64, &view));
        EXPECT_EQ(nullptr, view.data);
        EXPECT_EQ(0u, view.count);
    }
}

TEST(OscBlobTest, NumericArrayParsesUnalignedShapeAndValues)
{
    std::vector<unsigned char> storage(1);
    const int32_t dimensions = 2;
    const int32_t rows = 2;
    const int32_t columns = 3;
    appendValue(storage, dimensions);
    appendValue(storage, rows);
    appendValue(storage, columns);
    for (int32_t i = 0; i < rows * columns; i++) {
        const MYFLT value = (MYFLT)i;
        appendValue(storage, value);
    }

    OSC_ARRAY_BLOB_VIEW view{};
    ASSERT_EQ(OK, osc_blob_parse_array(storage.data() + 1,
                                       storage.size() - 1, &view));
    EXPECT_EQ(dimensions, view.dimensions);
    EXPECT_EQ(6u, view.values.count);
    int32_t parsedRows = 0;
    int32_t parsedColumns = 0;
    ASSERT_EQ(OK, osc_blob_array_size(&view, 0, &parsedRows));
    ASSERT_EQ(OK, osc_blob_array_size(&view, 1, &parsedColumns));
    EXPECT_EQ(rows, parsedRows);
    EXPECT_EQ(columns, parsedColumns);
}

TEST(OscBlobTest, NumericArrayRejectsTruncatedAndOverflowingShapes)
{
    const std::vector<std::vector<int32_t>> shapes = {
        {},
        {-1},
        {2, 4},
        {1, -1},
        {3, INT32_MAX, INT32_MAX, INT32_MAX}
    };
    for (const auto& shape : shapes) {
        std::vector<unsigned char> payload;
        for (const int32_t field : shape) {
            appendValue(payload, field);
        }
        OSC_ARRAY_BLOB_VIEW view{};
        EXPECT_EQ(NOTOK, osc_blob_parse_array(
                           payload.empty() ? nullptr : payload.data(),
                           payload.size(), &view));
    }

    std::vector<unsigned char> missingValues;
    appendValue(missingValues, (int32_t)1);
    appendValue(missingValues, (int32_t)2);
    appendValue(missingValues, FL(1.0));
    OSC_ARRAY_BLOB_VIEW view{};
    EXPECT_EQ(NOTOK, osc_blob_parse_array(
                       missingValues.data(), missingValues.size(), &view));
}

TEST(OscBlobTest, DirectArrayAndFtableRejectPartialSamples)
{
    std::array<unsigned char, sizeof(MYFLT) + 1> payload{};
    OSC_MYFLT_BLOB_VIEW view{};
    EXPECT_EQ(NOTOK, osc_blob_parse_myflts(
                       payload.data(), payload.size(), &view));
    EXPECT_EQ(nullptr, view.data);
    EXPECT_EQ(0u, view.count);
}

TEST(OscBlobTest, MalformedPayloadFuzzKeepsViewsWithinPackets)
{
    std::mt19937 generator(2682);
    std::uniform_int_distribution<int> byteDistribution(0, 255);
    for (size_t payloadSize = 0; payloadSize <= 128; payloadSize++) {
        for (size_t alignment = 0; alignment < alignof(MYFLT); alignment++) {
            std::vector<unsigned char> storage(payloadSize + alignment);
            for (unsigned char& byte : storage) {
                byte = (unsigned char)byteDistribution(generator);
            }
            unsigned char *payload = storage.empty()
                ? nullptr : storage.data() + alignment;

            OSC_MYFLT_BLOB_VIEW values{};
            if (osc_blob_parse_myflts(payload, payloadSize, &values) == OK) {
                expectViewInsidePayload(payload, payloadSize, values);
            }
            if (osc_blob_parse_audio(payload, payloadSize, 64, &values) == OK) {
                expectViewInsidePayload(payload, payloadSize, values);
            }

            OSC_ARRAY_BLOB_VIEW array{};
            if (osc_blob_parse_array(payload, payloadSize, &array) == OK) {
                EXPECT_GT(array.dimensions, 0);
                expectViewInsidePayload(payload, payloadSize, array.values);
                for (int32_t i = 0; i < array.dimensions; i++) {
                    int32_t dimensionSize = -1;
                    ASSERT_EQ(OK, osc_blob_array_size(
                                      &array, i, &dimensionSize));
                    EXPECT_GE(dimensionSize, 0);
                }
            }
        }
    }
}
