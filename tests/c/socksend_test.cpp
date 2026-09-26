#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <cstring>
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

static std::vector<unsigned char> packet;
static int sends;
template <typename Socket, typename Length, typename AddressSize>
static int capture_packet(Socket, const void *data, Length size, int,
                          const struct sockaddr *, AddressSize)
{
    const auto *bytes = static_cast<const unsigned char *>(data);
    packet.assign(bytes, bytes + size);
    ++sends;
    return (int)size;
}
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#define sendto capture_packet
#include "../../Opcodes/socksend.c"
#undef sendto

static int32_t sendInitError(CSOUND *, const char *, ...) { return NOTOK; }

class SocksendTests : public ::testing::Test {
protected:
    CSOUND *cs;
    SOCKSEND mono = {};
    SOCKSENDS stereo = {};
    SOCKSENDT text = {};
    INSDS instance = {};
    STRINGDAT host = {};
    cs_float port = 9000, length = 4, format = 1;
    void SetUp() override {
      cs = csoundCreate(nullptr, nullptr);
      cs->InitError = sendInitError;
      cs->Get0dBFS = [](CSOUND *) -> cs_float { return FL(1.0); };
      cs->AuxAlloc = [](CSOUND *, size_t size, AUXCH *aux) {
        free(aux->auxp);
        aux->auxp = calloc(1, size);
        aux->size = size;
      };
      host.data = (char *)"127.0.0.1";
      mono.ipaddress = stereo.ipaddress = &host;
      mono.port = stereo.port = &port;
      mono.buffersize = stereo.buffersize = &length;
      mono.format = stereo.format = &format;
      text.ipaddress = &host; text.port = &port;
      text.buffersize = &length; text.format = &format;
      mono.h.insdshead = stereo.h.insdshead = &instance;
      sends = 0;
      packet.clear();
    }
    void TearDown() override {
      socksend_deinit(cs, &mono);
      socksends_deinit(cs, &stereo);
      socksend_deinit(cs, (SOCKSEND *)&text);
      free(mono.aux.auxp);
      free(stereo.aux.auxp);
      free(text.aux.auxp);
      csoundDestroy(cs);
    }
};

TEST_F(SocksendTests, PcmPacketsPreserveChannelsAndAdvance)
{
    cs_float left[] = {99, FL(0.25), 1}, right[] = {99, FL(-0.5), -1};
    instance.ksmps = 3;
    instance.ksmps_offset = 1;
    stereo.asigl = left;
    stereo.asigr = right;
    ASSERT_EQ(init_sendS(cs, &stereo), OK);
    ASSERT_EQ(send_sendS(cs, &stereo), OK);
    EXPECT_EQ(packet, (std::vector<unsigned char>{0,0x20,0,0xc0,0xff,0x7f,0,0x80}));
    EXPECT_EQ(stereo.wp, 0);
    EXPECT_EQ(sends, 1);

    length = 2;
    cs_float value = FL(0.25);
    mono.asig = &value;
    ASSERT_EQ(init_send(cs, &mono), OK);
    EXPECT_EQ(send_send_k(cs, &mono), OK);
    EXPECT_EQ(mono.wp, 1);
    value = FL(-0.5);
    EXPECT_EQ(send_send_k(cs, &mono), OK);
    EXPECT_EQ(packet, (std::vector<unsigned char>{0,0x20,0,0xc0}));
    EXPECT_EQ(mono.wp, 0);
    EXPECT_EQ(sends, 2);

    cs_float audio[] = {99, 2, -2, 99};
    mono.asig = audio;
    instance.ksmps = 4;
    instance.ksmps_no_end = 1;
    EXPECT_EQ(send_send(cs, &mono), OK);
    EXPECT_EQ(packet, (std::vector<unsigned char>{0xff,0x7f,0,0x80}));
    EXPECT_EQ(sends, 3);

    format = 0;
    length = 4;
    instance.ksmps = 3;
    instance.ksmps_no_end = 0;
    ASSERT_EQ(init_sendS(cs, &stereo), OK);
    EXPECT_EQ(send_sendS(cs, &stereo), OK);
    const cs_float expected[] = {FL(0.25), FL(-0.5), 1, -1};
    ASSERT_EQ(packet.size(), sizeof(expected));
    EXPECT_EQ(memcmp(packet.data(), expected, sizeof(expected)), 0);
}

TEST_F(SocksendTests, ValidatesBuffersAndReusesAndClosesSockets)
{
    for (cs_float size : {FL(0.0), FL(-1.0), FL(65508.0)}) {
      length = size;
      EXPECT_EQ(init_send(cs, &mono), NOTOK);
      EXPECT_EQ(init_sendS(cs, &stereo), NOTOK);
    }
    length = 3;
    EXPECT_EQ(init_sendS(cs, &stereo), NOTOK);
    length = 2;
    ASSERT_EQ(init_send(cs, &mono), OK);
    int socket = mono.sock;
    length = 8;
    ASSERT_EQ(init_send(cs, &mono), OK);
    EXPECT_EQ(mono.sock, socket);
    EXPECT_GE(mono.aux.size, 16u);
    EXPECT_EQ(socksend_deinit(cs, &mono), 0);
    struct sockaddr_in address;
#if defined(WIN32) && !defined(__CYGWIN__)
    int size = sizeof(address);
#else
    socklen_t size = sizeof(address);
#endif
    EXPECT_EQ(getsockname(socket, (struct sockaddr *)&address, &size), SOCKET_ERROR);
    EXPECT_EQ(socksend_deinit(cs, &mono), OK);

    char storage[8] = {'h', 'i', 0, 's', 'e', 'c', 'r', 't'};
    STRINGDAT value = {};
    value.data = storage;
    value.size = sizeof(storage);
    text.str = &value;
    ASSERT_EQ(init_send_Str(cs, &text), OK);
    EXPECT_EQ(send_send_Str(cs, &text), OK);
    EXPECT_EQ(packet, (std::vector<unsigned char>{'h','i',0,0,0,0,0,0}));
}
#endif
