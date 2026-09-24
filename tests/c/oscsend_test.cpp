#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
#ifdef HAVE_SOCKETS
#if defined(WIN32) && !defined(__CYGWIN__)
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

static std::vector<std::vector<unsigned char>> messages;
template <typename Socket, typename Length, typename AddressSize>
static int capture_message(Socket, const void *data, Length size, int,
                           const struct sockaddr *, AddressSize)
{
    const auto *bytes = static_cast<const unsigned char *>(data);
    messages.emplace_back(bytes, bytes + size);
    return (int)size;
}
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#define sendto capture_message
#include "../../Opcodes/socksend.c"
#undef sendto

static int32_t initResult;
static int32_t observeInit(CSOUND *csound, void *opcode)
{
    initResult = osc_send2_init(csound, (OSCSEND2 *)opcode);
    return initResult;
}

class OscsendTests : public ::testing::Test {
protected:
    CSOUND *csound;
    void SetUp() override {
      messages.clear();
      initResult = 999;
      csound = csoundCreate(nullptr, nullptr);
      csoundCreateMessageBuffer(csound, 0);
      csoundSetOption(csound, "-n");
      csoundSetOption(csound, "-m0");
      ASSERT_EQ(csoundAppendOpcode(csound, "test_oscsend", sizeof(OSCSEND2),
          0, "", "kSkSN", observeInit, (SUBR)osc_send2,
          (SUBR)oscsend_deinit), OK);
    }
    void TearDown() override { csoundDestroy(csound); }
    void run(const char *body) {
      std::string csd = "<CsoundSynthesizer>\n<CsInstruments>\n"
        "sr = 48000\nksmps = 32\nnchnls = 1\n0dbfs = 1\ninstr 1\n";
      csd += body;
      csd += "\nendin\n</CsInstruments>\n<CsScore>\ni 1 0 .01\n"
             "</CsScore>\n</CsoundSynthesizer>\n";
      ASSERT_EQ(csoundCompileCSD(csound, csd.c_str(), 1, 0), OK);
      ASSERT_EQ(csoundStart(csound), OK);
      while (csoundPerformKsmps(csound) == 0) {}
    }
};

TEST_F(OscsendTests, TimestampsPreserveFollowingArgumentsAndWireTypes)
{
    run("Sformat sprintf \"%s\", \"tistbl\"\n"
        "kcycle timeinstk\n"
        "kflag = (kcycle == 1 ? 1 : 0)\n"
        "test_oscsend kflag, \"127.0.0.1\", 9000, \"/x\", Sformat, "
        "2147483648, 4026531840, 42, \"ok\", 3, 4, kflag, -17\n");
    ASSERT_EQ(initResult, OK);
    ASSERT_EQ(messages.size(), 2u);
    std::vector<unsigned char> expected = {
      '/', 'x', 0, 0, ',', 't', 'i', 's', 't', 'T', 'h', 0,
      0x80, 0, 0, 0, 0xf0, 0, 0, 0,
      0, 0, 0, 42, 'o', 'k', 0, 0,
      0, 0, 0, 3, 0, 0, 0, 4,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef
    };
    EXPECT_EQ(messages[0], expected);
    expected[9] = 'F';
    EXPECT_EQ(messages[1], expected);
}

TEST_F(OscsendTests, RejectsMissingArgumentForSecondTimestamp)
{
    run("test_oscsend 1, \"127.0.0.1\", 9000, \"/x\", \"tt\", 1, 2, 3\n");
    EXPECT_NE(initResult, OK);
    EXPECT_TRUE(messages.empty());
}
#endif
