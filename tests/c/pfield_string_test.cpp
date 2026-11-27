#include "gtest/gtest.h"
#include <chrono>
#include <cstring>
#include <limits>
#include <vector>

#define __BUILDING_LIBCSOUND
#include "pfield.h"
#include "insds_private.h"
#include "udo.h"

namespace {
class PfieldStringTests : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;
  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
  }
  void TearDown() override { csoundDestroy(csound); }
  void checkReinit(bool realtime);
};

TEST_F(PfieldStringTests, LegacyIndicesHaveBoundsAndKeepTheirBitPattern)
{
  for (uint32_t index : {0U, 1U, 65535U}) {
    uint32_t decoded = 123;
    cs_float code = csoundPFieldStringCode(index);
    EXPECT_TRUE(IsStringCode(code));
    EXPECT_TRUE(csoundPFieldStringIndex(code, &decoded));
    EXPECT_EQ(decoded, index);
  }
  uint32_t decoded;
  EXPECT_FALSE(csoundPFieldStringIndex(FL(12.0), &decoded));
  EXPECT_FALSE(csoundPFieldStringIndex(std::numeric_limits<cs_float>::infinity(),
                                      &decoded));
  char strings[] = "alpha\0beta";
  cs_float values[] = {0, csoundPFieldStringCode(1)};
  EVTBLK event = {};
  event.p = values;
  event.pcnt = 1;
  event.strarg = strings;
  event.scnt = 2;
  EXPECT_STREQ(csoundEventPFieldString(&event, 1), "beta");
  EXPECT_EQ(csoundEventPFieldString(&event, 2), nullptr);
  EXPECT_EQ(csoundEventStringArg(&event, csoundPFieldStringCode(2)), nullptr);
  EXPECT_EQ(csoundEventStringArg(&event, csoundPFieldStringCode(65535)), nullptr);
  EXPECT_EQ(csoundEventStringBytes(&event), sizeof(strings));
  values[1] = csoundPFieldStringCode(2);
  EXPECT_EQ(csoundPFieldsImport(csound, &event), nullptr);
  event.scnt = 0; // Older single-string table generators omit the count.
  EXPECT_STREQ(csoundEventStringArg(&event, csoundPFieldStringCode(0)), "alpha");
  EXPECT_EQ(csoundEventStringArg(&event, csoundPFieldStringCode(1)), nullptr);
  event.scnt = -1;
  EXPECT_EQ(csoundPFieldsImport(csound, &event), nullptr);
  event.scnt = 65537;
  EXPECT_EQ(csoundEventStringBytes(&event), 0U);
}

TEST_F(PfieldStringTests, MalformedEventFailsBeforeOpcodeInitialization)
{
  ASSERT_EQ(csoundCompileOrc(csound, R"(
    sr = 1000
    ksmps = 10
    nchnls = 1
    instr 1
      SText strget p4
      chnset 1, "initialized"
    endin
  )", 0), 0);
  ASSERT_EQ(csoundStart(csound), 0);
  cs_float values[] = {0, 1, 0, FL(.1), csoundPFieldStringCode(1)};
  char string[] = "only one";
  EVTBLK event = {};
  event.p = values;
  event.pcnt = 4;
  event.strarg = string;
  event.scnt = 1;
  event.opcod = 'i';
  EXPECT_NE(insert_event(csound, 1, &event), 0);
  EXPECT_EQ(csound->engineState.instrtxtp[1]->active, 0);
  EXPECT_EQ(csoundGetControlChannel(csound, "initialized", nullptr), FL(0.0));
}

TEST_F(PfieldStringTests, TypedNumbersDoNotBecomeStrings)
{
  CS_PFIELDS *fields = csoundPFieldsCreate(csound, 5);
  ASSERT_NE(fields, nullptr);
  fields->fields[1].value.number = std::numeric_limits<cs_float>::quiet_NaN();
  fields->fields[2].value.number = std::numeric_limits<cs_float>::infinity();
  fields->fields[3].value.number = FL(42.0);
  ASSERT_EQ(csoundPFieldsSetString(csound, fields, 4, "text"), 0);
  EXPECT_EQ(csoundPFieldsString(fields, 1), nullptr);
  EXPECT_EQ(csoundPFieldsString(fields, 2), nullptr);
  EXPECT_EQ(fields->fields[1].kind, CS_PFIELD_NUMBER);
  EXPECT_STREQ(csoundPFieldsString(fields, 4), "text");
  EVTBLK event = {};
  // A legacy event cannot distinguish this number from string ordinal zero.
  EXPECT_EQ(csoundPFieldsExport(csound, fields, &event), CSOUND_ERROR);
  EXPECT_EQ(event.p, nullptr);
  EXPECT_EQ(event.strarg, nullptr);
  csoundPFieldsDestroy(csound, fields);

  cs_float values[] = {0, std::numeric_limits<cs_float>::quiet_NaN(),
                      std::numeric_limits<cs_float>::infinity()};
  event.p = values;
  event.pcnt = 2;
  fields = csoundPFieldsImport(csound, &event);
  ASSERT_NE(fields, nullptr);
  EXPECT_EQ(fields->fields[1].kind, CS_PFIELD_NUMBER);
  EXPECT_EQ(fields->fields[2].kind, CS_PFIELD_NUMBER);
  EXPECT_EQ(std::memcmp(&values[1], &fields->fields[1].value.number,
                        sizeof(cs_float)), 0);
  csoundPFieldsDestroy(csound, fields);
}

TEST_F(PfieldStringTests, RoundTripOwnsEmptyAndLongStrings)
{
  CS_PFIELDS *source = csoundPFieldsCreate(csound, 6);
  ASSERT_NE(source, nullptr);
  std::string longText(20000, 'x');
  source->fields[1].value.number = FL(0.123456789);
  ASSERT_EQ(csoundPFieldsSetString(csound, source, 2, ""), 0);
  ASSERT_EQ(csoundPFieldsSetString(csound, source, 3, longText.c_str()), 0);
  ASSERT_EQ(csoundPFieldsSetString(csound, source, 4, "last"), 0);
  ASSERT_EQ(csoundPFieldsSetString(csound, source, 5,
                                  csoundPFieldsString(source, 4)), 0);
  EVTBLK event = {};
  ASSERT_EQ(csoundPFieldsExport(csound, source, &event), 0);
  CS_PFIELDS *copy = csoundPFieldsImport(csound, &event);
  ASSERT_NE(copy, nullptr);
  csoundPFieldsDestroy(csound, source);
  csoundPFieldsFreeEvent(csound, &event);
  EXPECT_EQ(copy->fields[1].value.number, FL(0.123456789));
  EXPECT_STREQ(csoundPFieldsString(copy, 2), "");
  EXPECT_EQ(csoundPFieldsString(copy, 3), longText);
  EXPECT_STREQ(csoundPFieldsString(copy, 4), "last");
  EXPECT_STREQ(csoundPFieldsString(copy, 5), "last");
  EXPECT_EQ(csoundPFieldsString(copy, 6), nullptr);
  csoundPFieldsDestroy(csound, copy);
}

TEST_F(PfieldStringTests, InstanceAndNestedUdosOwnStringsUntilReset)
{
  INSDS owner = {}, firstUdo = {}, secondUdo = {};
  OPCOD_IOBUFS firstBuffers = {}, secondBuffers = {};
  firstBuffers.parent_ip = &owner;
  firstUdo.opcod_iobufs = &firstBuffers;
  secondBuffers.parent_ip = &firstUdo;
  secondUdo.opcod_iobufs = &secondBuffers;
  char strings[] = "alpha\0beta";
  cs_float values[] = {0, csoundPFieldStringCode(0), csoundPFieldStringCode(1)};
  EVTBLK event = {};
  event.p = values;
  event.pcnt = 2;
  event.scnt = 2;
  event.strarg = strings;
  ASSERT_EQ(csoundInstancePFields(csound, &owner, &event), 0);
  strings[6] = 'X'; // The caller can now change or release its event.
  EXPECT_STREQ(csoundInstanceStringArg(&secondUdo, values[2]), "beta");
  EXPECT_NE(owner.strarg, strings);
  auto *extension = owner.private_data;
  csoundResetInstancePFields(csound, &owner);
  EXPECT_EQ(owner.private_data, extension); // Room for other private members.
  EXPECT_EQ(owner.strarg, nullptr);
  EXPECT_EQ(csoundInstanceStringArg(&secondUdo, values[2]), nullptr);
  ASSERT_EQ(csoundInstancePFields(csound, &owner, &event), 0);
  EXPECT_STREQ(csoundInstanceStringArg(&owner, values[2]), "Xeta");
  EXPECT_EQ(owner.private_data, extension);
  csoundFreeInstancePrivate(csound, &owner);
  EXPECT_EQ(owner.private_data, nullptr);
}

void PfieldStringTests::checkReinit(bool realtime)
{
  if (realtime) {
#if !CSOUND_SPINLOCK_AVAILABLE || (!defined(MSVC) && !defined(HAVE_ATOMIC_BUILTIN))
    GTEST_SKIP() << "Realtime mode requires native spinlocks and atomics";
#endif
    ASSERT_EQ(csoundSetOption(csound, "--realtime --daemon"), 0);
  }
  ASSERT_EQ(csoundCompileOrc(csound, R"(
    sr = 1000
    ksmps = 10
    nchnls = 1
    instr 1
      kOnce init 0
      if kOnce == 0 then
        kOnce = 1
        reinit readString
      endif
      igoto done
    readString:
      SText strget p5
      chnset SText, "reinit"
      rireturn
    done:
    endin
  )", 0), 0);
  csoundEventString(csound, "i 1 0 .1 \"first\" \"after init\"", 0);
  ASSERT_EQ(csoundStart(csound), 0);
  for (int i = 0; i < 20; ++i) {
    if (csoundPerformKsmps(csound) != 0) break;
    if (realtime) {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      while (alloc_queue_has_pending(csound)) {
        ASSERT_LT(std::chrono::steady_clock::now(), deadline);
        csoundSleep(1);
      }
    }
  }
  EXPECT_EQ(csound->inerrcnt, 0);
  int size = csoundGetChannelDatasize(csound, "reinit");
  ASSERT_GT(size, 0);
  std::vector<char> text(size);
  csoundGetStringChannel(csound, "reinit", text.data());
  EXPECT_STREQ(text.data(), "after init");
}
TEST_F(PfieldStringTests, ReinitReadsOwnedStringAfterEventIsGone)
{
  checkReinit(false);
}

TEST_F(PfieldStringTests, RealtimeReinitReadsStringAfterQueueReleasesEvent)
{
  checkReinit(true);
}
} // namespace
