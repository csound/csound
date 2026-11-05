/*
 * File:   csound_plugin_opcode_test.cpp
 * Author: vlazzarini
 *
 * Created on Nov 17, 2024
 */

#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "csound.h"
#include "csdl.h"

#ifdef CSOUND_TEST_HAS_REQUESTED_OPCODE_PLUGIN
#include "requested_opcode_plugin_path.hpp"
#endif


class PluginTests : public ::testing::Test {
public:
    PluginTests ()
    {
    }

    virtual ~PluginTests ()
    {
    }

    virtual void SetUp ()
    {
        csound = csoundCreate (NULL,NULL);
        csoundSetOption(csound, "-odac");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F(PluginTests, LoadsReadOnlyDirectoryList)
{
  // The loader must split the list without writing into the caller's string.
#ifdef WIN32
  const char *directories = "missing-plugin-dir-a;missing-plugin-dir-b";
#else
  const char *directories = "missing-plugin-dir-a:missing-plugin-dir-b";
#endif
  ASSERT_EQ(csoundLoadPlugins(csound, directories), CSOUND_SUCCESS);
}

struct OPC {
  OPDS h;
  cs_float *r,*a,*b,*c;
  cs_float ival;
};

int32_t opcode_init(CSOUND *csound, OPC *p) {
  p->ival = *p->c;
  return OK;
}

int32_t opcode_perf(CSOUND *csound, OPC *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;
  cs_float *out = p->r, *in = p->a, scal = *p->b, offs = p->ival;

  if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(cs_float));
  if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(cs_float));
  }
  for(n = offset; n < nsmps;  n++) 
    out[n] = in[n]*scal + offs;
  
  return OK;
}

int32_t opcode_deinit(CSOUND *csound, OPC *p) {
  p->ival = (cs_float) 0;
  return OK;
}


TEST_F (PluginTests, testAddOpcodeC)
{
  int32_t result;
  const char* instrument =
        "instr 1 \n"
        "a1 oscili p4, p5 \n"
        "a2 opcode_test a1, 0dbfs, 0  \n"
        "out  linen(a2,0.1,p3,0.1)   \n"
        "endin \n";
    
  csoundAppendOpcode(csound, "opcode_test", sizeof(OPC), 0,
                     "a", "aki", (SUBR) opcode_init, (SUBR) opcode_perf,
                     (SUBR) opcode_deinit);
  result = csoundCompileOrc(csound, instrument, 0);
  ASSERT_TRUE (result == 0);
  csoundEventString(csound,  "i 1 0 1 0.5 440 \n", 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  while(!result)
    result = csoundPerformKsmps(csound);
  csoundSleep(500);
}

#ifdef CSOUND_TEST_HAS_REQUESTED_OPCODE_PLUGIN
TEST_F(PluginTests, testOpcodeLibLoadsBeforeOrchestraChecks)
{
  const std::string option =
      "--opcode-lib=" CSOUND_TEST_REQUESTED_OPCODE_PLUGIN;
  const char *instrument =
      "instr 1\n"
      "  iValue requested_opcode_fixture 42\n"
      "endin\n";

  ASSERT_EQ(CSOUND_SUCCESS, csoundSetOption(csound, option.c_str()));
  ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument, 0));
}

TEST_F(PluginTests, testMissingOpcodeLibFailsOrchestraChecks)
{
  const std::string option =
      "--opcode-lib=" CSOUND_TEST_REQUESTED_OPCODE_PLUGIN ".missing";
  const char *instrument =
      "instr 1\n"
      "  iValue requested_opcode_fixture 42\n"
      "endin\n";

  ASSERT_EQ(CSOUND_SUCCESS, csoundSetOption(csound, option.c_str()));
  ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument, 0));
}

namespace {
const char *kRequestedOpcode = "requested_opcode_fixture";
const char *kOpcodeLibOption =
    "--opcode-lib=" CSOUND_TEST_REQUESTED_OPCODE_PLUGIN;

std::string drainMessages(CSOUND *csound) {
  std::string result;
  while (csoundGetMessageCnt(csound)) {
    result += csoundGetFirstMessage(csound);
    csoundPopFirstMessage(csound);
  }
  return result;
}

struct OpcodeListing {
  int32_t status;
  std::string text;
};

OpcodeListing compileWithArgs(std::vector<const char *> args) {
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  csoundCreateMessageBuffer(csound, 0);
  args.insert(args.begin(), "csound");
  const int32_t status =
      csoundCompile(csound, (int32_t) args.size(), args.data());
  OpcodeListing listing{status, drainMessages(csound)};
  csoundDestroy(csound);
  return listing;
}

void expectListsRequestedOpcode(std::vector<const char *> args) {
  const OpcodeListing listing = compileWithArgs(args);
  EXPECT_EQ(listing.status, CSOUND_EXITJMP_SUCCESS) << listing.text;
  EXPECT_NE(listing.text.find(kRequestedOpcode), std::string::npos)
      << listing.text;
  EXPECT_NE(listing.text.find(" opcodes\n"), std::string::npos)
      << listing.text;
}

class TempCsd {
public:
  explicit TempCsd(const std::string &options) {
    path_ = (std::filesystem::temp_directory_path() /
             ("csound_list_opcodes_" + std::to_string(std::random_device{}()) +
              ".csd")).string();
    std::ofstream file(path_);
    file << "<CsoundSynthesizer>\n<CsOptions>\n" << options
         << "\n</CsOptions>\n<CsInstruments>\n"
            "instr 1\n  prints \"performed\\n\"\nendin\n"
            "schedule 1, 0, 0\n"
            "</CsInstruments>\n</CsoundSynthesizer>\n";
  }
  ~TempCsd() { std::filesystem::remove(path_); }
  const char *path() const { return path_.c_str(); }
private:
  std::string path_;
};
} // namespace

TEST(ListOpcodes, IncludesOpcodeLibGivenAfterTheOption) {
  expectListsRequestedOpcode({"--list-opcodes1", kOpcodeLibOption});
}

TEST(ListOpcodes, IncludesOpcodeLibGivenBeforeTheOption) {
  expectListsRequestedOpcode({kOpcodeLibOption, "--list-opcodes1"});
}

TEST(ListOpcodes, ShortOptionIncludesOpcodeLib) {
  expectListsRequestedOpcode({"-z1", kOpcodeLibOption});
}

TEST(ListOpcodes, DefaultStyleIncludesOpcodeLib) {
  expectListsRequestedOpcode({"--list-opcodes", kOpcodeLibOption});
}

TEST(ListOpcodes, OptionsInCsdIncludeOpcodeLibAndStopBeforeCompiling) {
  const TempCsd csd(std::string(kOpcodeLibOption) + " --list-opcodes1");
  const OpcodeListing listing = compileWithArgs({"-n", csd.path()});
  EXPECT_EQ(listing.status, CSOUND_EXITJMP_SUCCESS) << listing.text;
  EXPECT_NE(listing.text.find(kRequestedOpcode), std::string::npos)
      << listing.text;
  EXPECT_EQ(listing.text.find("performed"), std::string::npos)
      << listing.text;
}

TEST(ListOpcodes, ListsOpcodesAndStructsInOneCall) {
  const OpcodeListing listing = compileWithArgs(
      {"--list-structs", "--list-opcodes1", kOpcodeLibOption});
  EXPECT_EQ(listing.status, CSOUND_EXITJMP_SUCCESS) << listing.text;
  EXPECT_NE(listing.text.find(kRequestedOpcode), std::string::npos)
      << listing.text;
  const size_t opcodes = listing.text.find(" opcodes\n");
  const size_t structs = listing.text.find(" structs\n");
  ASSERT_NE(opcodes, std::string::npos) << listing.text;
  ASSERT_NE(structs, std::string::npos) << listing.text;
  EXPECT_LT(opcodes, structs) << listing.text;
}

TEST(ListOpcodes, SetOptionListsAtOnceAndLeavesCompileRunning) {
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  csoundCreateMessageBuffer(csound, 0);
  ASSERT_EQ(csoundSetOption(csound, kOpcodeLibOption), CSOUND_SUCCESS);
  ASSERT_EQ(csoundSetOption(csound, "--list-opcodes1"), CSOUND_SUCCESS);
  const std::string listing = drainMessages(csound);
  EXPECT_NE(listing.find(kRequestedOpcode), std::string::npos) << listing;

  const TempCsd csd("");
  const char *args[] = {"csound", "-n", csd.path()};
  EXPECT_EQ(csoundCompile(csound, 3, args), CSOUND_SUCCESS)
      << drainMessages(csound);
  csoundDestroy(csound);
}
#endif

#include "plugin.h"

/** i-time plugin opcode example
    with 1 output and 1 input \n
    iout simple iin
 */
struct Simplei : csnd::Plugin<1, 1> {
  int init() {
    outargs[0] = inargs[0];
    return OK;
  }
};

/** k-rate plugin opcode example
    with 1 output and 1 input \n
    kout simple kin
 */
struct Simplek : csnd::Plugin<1, 1> {
  int kperf() {
    outargs[0] = inargs[0];
    return OK;
  }
};

/** a-rate plugin opcode example
    with 1 output and 1 input \n
    aout simple ain
 */
struct Simplea : csnd::Plugin<1, 1> {
  int aperf() {
    std::copy(inargs(0) + offset, inargs(0) + nsmps, outargs(0));
    return OK;
  }
};

namespace csnd {
void on_load(Csound *csound) {
  csnd::plugin<Simplei>(csound, "simple", "i", "i", csnd::thread::i);
  csnd::plugin<Simplek>(csound, "simple", "k", "k", csnd::thread::k);
  csnd::plugin<Simplea>(csound, "simple", "a", "a", csnd::thread::a);
}
}

TEST_F (PluginTests, testAddOpcodeCpp)
{
  int32_t result;
  const char* instrument =
        "instr 1 \n"
        "i1 simple 0 \n"
        "k2 simple 0dbfs \n"
        "a2 simple oscili(p4, p5) \n"
        "out  linen(a2*k2 + i1, 0.1,p3,0.1)  \n"
        "endin \n";
    
  csnd::on_load((csnd::Csound *)csound);
  result = csoundCompileOrc(csound, instrument, 0);
  ASSERT_TRUE (result == 0);
  csoundEventString(csound,  "i 1 0 1 0.5 550 \n", 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  while(!result)
    result = csoundPerformKsmps(csound);
  csoundSleep(500);
}
