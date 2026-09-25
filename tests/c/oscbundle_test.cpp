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

static std::vector<unsigned char> bundle;
static int sends;
template <typename Socket, typename Length, typename AddressSize>
static int capture_bundle(Socket, const void *data, Length size, int,
                          const struct sockaddr *, AddressSize)
{
    const auto *bytes = static_cast<const unsigned char *>(data);
    bundle.assign(bytes, bytes + size);
    ++sends;
    return (int)size;
}
#undef LINKAGE_BUILTIN
#define LINKAGE_BUILTIN(x)
#define sendto capture_bundle
#include "../../Opcodes/socksend.c"
#undef sendto

static int32_t bundleInitError(CSOUND *, const char *, ...) { return NOTOK; }
static int32_t bundlePerfError(CSOUND *, OPDS *, const char *, ...) { return NOTOK; }

class OscBundleTests : public ::testing::Test {
protected:
    CSOUND *cs;
    OSCBUNDLE p = {};
    ARRAYDAT dest = {}, types = {}, args = {};
    STRINGDAT addresses[2] = {}, tags[2] = {}, host = {};
    int32_t destRows = 2, typeRows = 2, shape[2] = {2, 1};
    cs_float values[2] = {42, 17}, trigger = 0, port = 9000, mtu = 48;
    void SetUp() override {
      cs = csoundCreate(nullptr, nullptr);
      cs->InitError = bundleInitError;
      cs->PerfError = bundlePerfError;
      cs->AuxAlloc = [](CSOUND *, size_t size, AUXCH *aux) {
        free(aux->auxp);
        aux->auxp = calloc(1, size);
        aux->size = size;
      };
      addresses[0].data = (char *)"/a";
      addresses[1].data = (char *)"/b";
      tags[0].data = (char *)"i";
      tags[1].data = (char *)"f";
      host.data = (char *)"127.0.0.1";
      dest.dimensions = types.dimensions = 1;
      dest.sizes = &destRows;
      types.sizes = &typeRows;
      dest.data = reinterpret_cast<cs_float *>(addresses);
      types.data = reinterpret_cast<cs_float *>(tags);
      dest.arrayMemberSize = types.arrayMemberSize = sizeof(STRINGDAT);
      args.dimensions = 2;
      args.sizes = shape;
      args.data = values;
      p.dest = &dest; p.type = &types; p.arg = &args;
      p.kwhen = &trigger; p.port = &port; p.imtu = &mtu; p.ipaddress = &host;
      sends = 0;
      bundle.clear();
    }
    void TearDown() override {
      oscbundle_deinit(cs, &p);
      free(p.aux.auxp);
      csoundDestroy(cs);
    }
};

TEST_F(OscBundleTests, RejectsInvalidShapesAndPacketLimits)
{
    shape[0] = 1;
    EXPECT_EQ(oscbundle_init(cs, &p), NOTOK);
    shape[0] = 3;
    EXPECT_EQ(oscbundle_init(cs, &p), NOTOK);
    shape[0] = 2;
    types.dimensions = 0;
    EXPECT_EQ(oscbundle_init(cs, &p), NOTOK);
    types.dimensions = 1;
    for (cs_float size : {FL(-1.0), FL(8.0), FL(65537.0)}) {
      mtu = size;
      EXPECT_EQ(oscbundle_init(cs, &p), NOTOK);
    }
    mtu = 48;
    ASSERT_EQ(oscbundle_init(cs, &p), OK);
    shape[0] = 1;
    EXPECT_EQ(oscbundle_perf(cs, &p), NOTOK);
    EXPECT_EQ(sends, 0);
    shape[0] = 2;
    tags[1].data = (char *)"s";
    EXPECT_EQ(oscbundle_perf(cs, &p), NOTOK);
    EXPECT_EQ(sends, 0);
}

TEST_F(OscBundleTests, SendsExactFitOnFirstCallAndGrowsOnReinit)
{
    const std::vector<unsigned char> expected = {
      '#','b','u','n','d','l','e',0, 0,0,0,0,0,0,0,1,
      0,0,0,12, '/','a',0,0, ',','i',0,0, 0,0,0,42,
      0,0,0,12, '/','b',0,0, ',','f',0,0, 0x41,0x88,0,0
    };
    ASSERT_EQ(oscbundle_init(cs, &p), OK);
    ASSERT_EQ(oscbundle_perf(cs, &p), OK);
    EXPECT_EQ(bundle, expected);
    EXPECT_EQ(sends, 1);
    EXPECT_EQ(oscbundle_perf(cs, &p), OK);
    EXPECT_EQ(sends, 1);
    int socket = p.sock;
    mtu = 256;
    ASSERT_EQ(oscbundle_init(cs, &p), OK);
    EXPECT_EQ(p.sock, socket);
    EXPECT_GE(p.aux.size, 256u);
    EXPECT_EQ(oscbundle_perf(cs, &p), OK);
    EXPECT_EQ(bundle, expected);
    EXPECT_EQ(sends, 2);
    mtu = 47;
    ASSERT_EQ(oscbundle_init(cs, &p), OK);
    EXPECT_EQ(oscbundle_perf(cs, &p), OK);
    EXPECT_EQ(sends, 2);
}
#endif
