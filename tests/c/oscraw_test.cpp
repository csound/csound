#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cstring>
#include <vector>
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
using TestSocket = SOCKET;
#else
#include <sys/socket.h>
#include <unistd.h>
using TestSocket = int;
#endif

namespace {
std::vector<unsigned char> packet;

void appendString(std::vector<unsigned char> &out, const char *value)
{
    size_t len = strlen(value) + 1;
    out.insert(out.end(), value, value + len);
    while ((out.size() & 3) != 0) out.push_back(0);
}

void appendU32(std::vector<unsigned char> &out, uint32_t value)
{
    out.push_back((unsigned char) (value >> 24));
    out.push_back((unsigned char) (value >> 16));
    out.push_back((unsigned char) (value >> 8));
    out.push_back((unsigned char) value);
}
}

#if defined(WIN32) && !defined(__CYGWIN__)
extern "C" int csound_test_oscraw_recvfrom(SOCKET, char *output, int size,
                                            int, struct sockaddr *, int *)
#else
extern "C" ssize_t csound_test_oscraw_recvfrom(int, void *output, size_t size,
                                                int, struct sockaddr *,
                                                socklen_t *)
#endif
{
    size_t copied = packet.size() < size ? packet.size() : size;
    memcpy(output, packet.data(), copied);
    return (int) copied;
}

extern "C" OENTRY *csound_test_oscraw_opcode(void);

namespace {
struct OscrawArgs {
    OPDS h;
    ARRAYDAT *output;
    cs_float *count;
    cs_float *port;
    AUXCH buffer;
    TestSocket socket;
    int32_t wsaStarted;
    int32_t initDone;
};

class OscrawTests : public ::testing::Test {
protected:
    CSOUND *cs;
    OENTRY *opcode;
    OscrawArgs *raw;
    ARRAYDAT output = {};
    int32_t outputSize = 13;
    STRINGDAT strings[13] = {};
    cs_float count = 0, port = 0;

    void SetUp() override {
      cs = csoundCreate(nullptr, nullptr);
      opcode = csound_test_oscraw_opcode();
      raw = (OscrawArgs *) calloc(1, opcode->dsblksiz);
      raw->output = &output;
      raw->count = &count;
      raw->port = &port;
      raw->buffer.auxp = calloc(1, 1456);
      raw->buffer.size = 1456;
      output.dimensions = 1;
      output.sizes = &outputSize;
      output.arrayMemberSize = sizeof(STRINGDAT);
      output.data = (cs_float *) strings;
      packet.clear();
    }

    void TearDown() override {
      for (STRINGDAT &value : strings) cs->Free(cs, value.data);
      free(raw->buffer.auxp);
      free(raw);
      csoundDestroy(cs);
    }

    void perform() { ASSERT_EQ(opcode->perf(cs, raw), OK); }

    bool isClosed(TestSocket socket) {
      struct sockaddr address;
      socklen_t length = sizeof(address);
      return getsockname(socket, &address, &length) == -1;
    }
};

TEST_F(OscrawTests, DecodesBoundedScalarsStringsAndBlobs)
{
    appendString(packet, "/x");
    appendString(packet, ",ifsbAaGTFIN");
    appendU32(packet, 42);
    float number = 1.5f;
    uint32_t bits;
    memcpy(&bits, &number, sizeof(bits));
    appendU32(packet, bits);
    appendString(packet, "ok");
    appendU32(packet, 3);
    packet.insert(packet.end(), {0, 1, 255, 0});
    const int32_t dimensions = 2;
    const int32_t sizes[] = {2, 1};
    const cs_float values[] = {FL(2.25), FL(-3.5)};
    appendU32(packet, sizeof(dimensions) + sizeof(sizes) + sizeof(values));
    const auto appendNative = [&](const void *data, size_t size) {
      const auto *bytes = static_cast<const unsigned char *>(data);
      packet.insert(packet.end(), bytes, bytes + size);
    };
    appendNative(&dimensions, sizeof(dimensions));
    appendNative(sizes, sizeof(sizes));
    appendNative(values, sizeof(values));
    const cs_float audio[] = {2, FL(0.25), FL(-0.5)};
    appendU32(packet, sizeof(audio));
    appendNative(audio, sizeof(audio));
    const cs_float table[] = {FL(4.0), FL(5.0)};
    appendU32(packet, sizeof(table));
    appendNative(table, sizeof(table));

    perform();
    ASSERT_EQ(count, 13);
    const char *expected[] = {
      "/x", ",ifsbAaGTFIN", "42", "1.5", "ok", "0x0001ff",
      "2:[2,1]:[2.25,-3.5]", "[0.25,-0.5]", "[4,5]",
      "true", "false", "inf", "nil"
    };
    for (int i = 0; i < 13; i++) EXPECT_STREQ(strings[i].data, expected[i]);

    packet = {'#','b','u','n','d','l','e',0, 0,0,0,0,0,0,0,1};
    for (const char *address : {"/a", "/b"}) {
      std::vector<unsigned char> message;
      appendString(message, address);
      appendString(message, ",");
      appendU32(packet, (uint32_t) message.size());
      packet.insert(packet.end(), message.begin(), message.end());
    }
    perform();
    ASSERT_EQ(count, 4);
    EXPECT_STREQ(strings[0].data, "/a");
    EXPECT_STREQ(strings[1].data, ",");
    EXPECT_STREQ(strings[2].data, "/b");
    EXPECT_STREQ(strings[3].data, ",");
}

TEST_F(OscrawTests, RejectsTruncatedAndOversizedFields)
{
    ASSERT_EQ(opcode->init(cs, raw), OK);
    ASSERT_EQ(opcode->init(cs, raw), OK);
    TestSocket currentSocket = raw->socket;
    EXPECT_EQ(opcode->deinit(cs, raw), OK);
    EXPECT_TRUE(isClosed(currentSocket));

    const std::vector<std::vector<unsigned char>> invalid = {
      {'/', 'x'},
      {'/', 'x', 0, 0, ',', 'b', 0, 0, 0xff, 0xff, 0xff, 0xff},
      {'#','b','u','n','d','l','e',0, 0,0,0,0,0,0,0,1,
       0,0,0,100},
      {'/', 'x', 0, 0, ',', 'A', 0, 0, 0,0,0,4, 99,0,0,0}
    };
    for (const auto &value : invalid) {
      packet = value;
      count = 99;
      perform();
      EXPECT_EQ(count, 0) << "packet length " << packet.size();
    }
}
} // namespace
#endif
