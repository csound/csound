#include "gtest/gtest.h"
#include <cstdint>
#include <string>
#include "csound.h"
#include "csdl.h"
#include "opcode.h"

namespace {
const char *header = "sr = 1000\nksmps = 10\n0dbfs = 1\n";

int forcedAcquireCalls = 0;

void forcedAcquireHook(CSOUND *csound, OPCODEOBJ *src, OPDS *) {
  if(++forcedAcquireCalls != 1) return;
  opcode_set_acquire_hook(nullptr);
  opcode_dataspace_release(csound, src, 0);
  opcode_dataspace_reclaim(csound);
}

struct AcquireHookGuard {
  ~AcquireHookGuard() { opcode_set_acquire_hook(nullptr); }
};

struct AlignedProbe {
  OPDS h;
  cs_float *result, *trigger;
  alignas(16) cs_float payload[4];
};

uintptr_t alignedProbeAddress = 0;

int32_t aligned_probe_perform(CSOUND *csound, AlignedProbe *p) {
  (void) csound;
  alignedProbeAddress = reinterpret_cast<uintptr_t>(p);
  *p->result = (alignedProbeAddress & 15u) == 0 ? 1.0f : 0.0f;
  return OK;
}

class OpcodeRefcountTests : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;

  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
  }

  void TearDown() override { csoundDestroy(csound); }

  std::string messages() {
    std::string text;
    while (csoundGetMessageCnt(csound)) {
      text += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return text;
  }

  int perform(const std::string &orchestra, const std::string &score,
              int maxBlocks = 200) {
    std::string code = std::string(header) + orchestra;
    EXPECT_EQ(csoundCompileOrc(csound, code.c_str(), 0), 0) << messages();
    csoundEventString(csound, score.c_str(), 0);
    EXPECT_EQ(csoundStart(csound), 0) << messages();
    int result = 0;
    for (int i = 0; i < maxBlocks && result == 0; ++i)
      result = csoundPerformKsmps(csound);
    return result;
  }
};

TEST_F(OpcodeRefcountTests, AliasSurvivesDelete) {
  perform("shared@global:Opcode = create(opcoderef(\"lincos\"))\n"
          "alias@global:Opcode = shared\n"
          "instr 1\n"
          "  result:k run alias, 0.5, 0, 10\n"
          "  printks \"alias result: %g\\n\", 1, result\n"
          "endin\n"
          "instr 2\n"
          "  delete(shared)\n"
          "endin\n",
          "i 1 0 0.3\ni 2 0.1 0.01\ne\n");
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  EXPECT_NE(messages().find("alias result:"), std::string::npos);
}

TEST_F(OpcodeRefcountTests, ArrayTemporariesDoNotReleaseArrayOwners) {
  perform("objects@global:Opcode[] = "
          "[create(opcoderef(\"lincos\")), create(opcoderef(\"lincos\"))]\n"
          "instr 1\n"
          "  result:k run objects[1], 0.5, 0, 10\n"
          "  printks \"array temporary result: %g\\n\", 1, result\n"
          "endin\n"
          "instr 2\n"
          "  delete(objects)\n"
          "endin\n",
          "i 1 0 0.2\ni 2 0.25 0.01\ne\n");
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  EXPECT_NE(messages().find("array temporary result:"), std::string::npos);
}

TEST_F(OpcodeRefcountTests, ArrayCanBeReplacedBeforeUse) {
  perform("instr 1\n"
          "  objects:Opcode[] = create(opcoderef(\"lincos\"), 2)\n"
          "  objects = create(opcoderef(\"lincos\"), 1)\n"
          "  result:k run objects[0], 0.5, 0, 10\n"
          "  printks \"array replace result: %g\\n\", 1, result\n"
          "endin\n",
          "i 1 0 0.2\ne\n");
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  EXPECT_NE(messages().find("array replace result:"), std::string::npos);
}

TEST_F(OpcodeRefcountTests, SharedAliasesRebindArguments) {
  perform("opcode Twice(x:k):k\n"
          "  xout x * 2\n"
          "endop\n"
          "instr 1\n"
          "  shared:Opcode = create(opcoderef(\"Twice\"))\n"
          "  alias:Opcode = shared\n"
          "  a:k run shared, 1\n"
          "  b:k run alias, 3\n"
          "  if abs(a-2) > 0.01 || abs(b-6) > 0.01 then\n"
          "    printks \"shared run wrong: %g %g\\n\", 1, a, b\n"
          "  else\n"
          "    printks \"shared run correct: %g %g\\n\", 1, a, b\n"
          "  endif\n"
          "  turnoff\n"
          "endin\n",
          "i 1 0 0.2\ne\n");
  const std::string output = messages();
  EXPECT_EQ(csoundErrCnt(csound), 0) << output;
  EXPECT_NE(output.find("shared run correct:"), std::string::npos) << output;
}

TEST_F(OpcodeRefcountTests, CreateOnSharedAliasPreservesOriginal) {
  perform("opcode Twice(x:k):k\n"
          "  xout x * 2\n"
          "endop\n"
          "instr 1\n"
          "  shared:Opcode = create(opcoderef(\"Twice\"))\n"
          "  alias:Opcode = shared\n"
          "  alias = create(opcoderef(\"Twice\"))\n"
          "  a:k run shared, 1\n"
          "  b:k run alias, 3\n"
          "  if abs(a-2) > 0.01 || abs(b-6) > 0.01 then\n"
          "    printks \"create shared wrong: %g %g\\n\", 1, a, b\n"
          "  else\n"
          "    printks \"create shared correct: %g %g\\n\", 1, a, b\n"
          "  endif\n"
          "  turnoff\n"
          "endin\n",
          "i 1 0 0.2\ne\n");
  const std::string output = messages();
  EXPECT_EQ(csoundErrCnt(csound), 0) << output;
  EXPECT_NE(output.find("create shared correct:"), std::string::npos) << output;
}

TEST_F(OpcodeRefcountTests, EndedUdoInstanceFailsClearly) {
  perform("opcode Twice(x:k):k\n"
          "  xout x * 2\n"
          "endop\n"
          "opcode MakeObj(x:i):Opcode\n"
          "  object:Opcode = create(opcoderef(\"Twice\"))\n"
          "  unused:k init object, 1\n"
          "  xout object\n"
          "endop\n"
          "instr 1\n"
          "  saved@global:Opcode = MakeObj(1)\n"
          "endin\n"
          "instr 2\n"
          "  result:k run saved, 3\n"
          "endin\n",
          "i 1 0 0.05\ni 2 0.1 0.1\ne\n");
  const std::string output = messages();
  EXPECT_GT(csoundErrCnt(csound), 0) << output;
  EXPECT_NE(output.find("opcode object UDO instance has ended"),
             std::string::npos) << output;
}

TEST_F(OpcodeRefcountTests, RegisteredOpcodeDataspaceIs16ByteAligned) {
  alignedProbeAddress = 0;
  csoundAppendOpcode(csound, "aligned_dataspace_probe", sizeof(AlignedProbe), 0,
                     "k", "i", (SUBR) aligned_probe_perform,
                     (SUBR) aligned_probe_perform, NULL);
  perform("instr 1\n"
          "  probe:Opcode = create(opcoderef(\"aligned_dataspace_probe\"))\n"
          "  result:k run probe, 1\n"
          "  printks \"aligned probe: %g\\n\", 0, result\n"
          "  turnoff\n"
          "endin\n",
          "i 1 0 0.05\ne\n");
  const std::string output = messages();
  EXPECT_EQ(csoundErrCnt(csound), 0) << output;
  EXPECT_NE(output.find("aligned probe: 1"), std::string::npos) << output;
  EXPECT_EQ(alignedProbeAddress & 15u, 0u);
}

TEST(OpcodeRetiredQueueTests, RetiredNodesDoNotOutliveInstance) {
  CSOUND *first = csoundCreate(nullptr, nullptr);
  ASSERT_NE(first, nullptr);
  csoundCreateMessageBuffer(first, 0);
  ASSERT_EQ(csoundSetOption(first, "-n -d -m0"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundCompileOrc(first,
    (std::string(header) +
     "instr 1\n"
     "  obj:Opcode = create(opcoderef(\"lincos\"))\n"
     "  result:k run obj, 0.5, 0, 10\n"
     "endin\n").c_str(), 0), 0);
  csoundEventString(first, "i 1 0 1", 0);
  ASSERT_EQ(csoundStart(first), 0);
  ASSERT_EQ(csoundPerformKsmps(first), 0);
  csoundDestroy(first);

  CSOUND *second = csoundCreate(nullptr, nullptr);
  ASSERT_NE(second, nullptr);
  csoundCreateMessageBuffer(second, 0);
  ASSERT_EQ(csoundSetOption(second, "-n -d -m0"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundCompileOrc(second,
    (std::string(header) +
     "instr 1\n"
     "  dummy:k init 0\n"
     "endin\n").c_str(), 0), 0);
  csoundEventString(second, "i 1 0 1", 0);
  ASSERT_EQ(csoundStart(second), 0);
  ASSERT_EQ(csoundPerformKsmps(second), 0);
  csoundDestroy(second);
}

TEST(OpcodeRetiredQueueTests, ReclaimDefersUntilAcquireReaderFinishes) {
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  ASSERT_NE(csound, nullptr);
  csoundCreateMessageBuffer(csound, 0);
  ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundCompileOrc(csound,
    (std::string(header) +
     "instr 1\n"
     "  shared@global:Opcode = create(opcoderef(\"lincos\"))\n"
     "endin\n"
     "instr 2\n"
     "  local:Opcode = shared\n"
     "endin\n").c_str(), 0), 0);
  csoundEventString(csound, "i 1 0 0.01\ni 2 0.05 0.01\ne\n", 0);
  ASSERT_EQ(csoundStart(csound), 0);
  ASSERT_EQ(csoundPerformKsmps(csound), 0);

  forcedAcquireCalls = 0;
  AcquireHookGuard guard;
  opcode_set_acquire_hook(forcedAcquireHook);
  for(int i = 0; i < 20 && forcedAcquireCalls == 0; i++) {
    int result = csoundPerformKsmps(csound);
    ASSERT_TRUE(result == 0 || result == 1 || result == 2) << result;
  }
  EXPECT_EQ(forcedAcquireCalls, 1);
  for(int i = 0; i < 5; i++)
    if(csoundPerformKsmps(csound) != 0) break;
  csoundDestroy(csound);
}
}

