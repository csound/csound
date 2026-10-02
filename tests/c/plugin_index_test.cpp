/* [] on a plugin-defined type. SPDX-License-Identifier: LGPL-2.1-or-later */
#include "gtest/gtest.h"
#include <string>

// Load C++ headers before csdl.h: its _CR macro conflicts with libstdc++.
#include "csound.h"
#include "csdl.h"
#include "index_plugin_path.hpp"

namespace {
class PluginIndex : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;
  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n");
    ASSERT_EQ(csoundLoadPlugins(csound, CSOUND_TEST_INDEX_PLUGIN_DIR), 0);
  }
  void TearDown() override { csoundDestroy(csound); }
  std::string messages() {
    std::string result;
    while (csoundGetMessageCnt(csound)) {
      result += csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return result;
  }
  void run(const std::string &body, const std::string &udos = "") {
    std::string code = "sr=44100\nksmps=32\nnchnls=1\n" + udos +
      "instr 1\n" + body + "\nendin\n";
    ASSERT_EQ(csoundCompileOrc(csound, code.c_str(), 0), 0) << messages();
    csoundEventString(csound, "i 1 0 0.01\ne\n", 0);
    ASSERT_EQ(csoundStart(csound), 0) << messages();
    int result = 0;
    for (int i = 0; i < 100 && result == 0; ++i) result = csoundPerformKsmps(csound);
    ASSERT_GT(result, 0) << messages();
    EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  }
  void channel(const char *name, cs_float expected) {
    int32_t error;
    EXPECT_EQ(csoundGetControlChannel(csound, name, &error), expected);
    EXPECT_EQ(error, 0);
  }
};

// indexed_vec(n) holds 10 * i at index i.

TEST_F(PluginIndex, ReadTakesTheElementTypeFromTheTypesGetter) {
  run(R"cs(
v:IndexedVec = indexed_vec(4)
ix = v[2]
iy = v[1] + v[3] * 2
chnset ix, "element"
chnset iy, "expression"
)cs");
  channel("element", 20); channel("expression", 70);
}

TEST_F(PluginIndex, InitWriteStoresAnIValue) {
  run(R"cs(
v:IndexedVec = indexed_vec(4)
v[0] = 99
v[1] = v[2] + 1
ia = v[0]
ib = v[1]
chnset ia, "constant"
chnset ib, "expression"
)cs");
  channel("constant", 99); channel("expression", 21);
}

TEST_F(PluginIndex, KIndexWritesAndReadsEveryCycle) {
  run(R"cs(
v:IndexedVec = indexed_vec(4)
kc init 0
v[kc] = kc + 100
kv = v[kc]
chnset kv, "last"
chnset v[kc], "same"
kc = kc < 3 ? kc + 1 : 3
)cs");
  channel("last", 103); channel("same", 103);
}

TEST_F(PluginIndex, IValueWithAKIndexIsWrittenAtInit) {
  run(R"cs(
v:IndexedVec = indexed_vec(4)
kj init 2
v[kj] = 7
ix = v[2]
chnset ix, "value"
)cs");
  channel("value", 7);
}

TEST_F(PluginIndex, GetterReturningThePluginTypeFeedsAnInitOnlyOpcode) {
  // The init-only consumer must not swap the plugin's getter for the
  // built-in ##array_get_init, which would read the vector as an ARRAYDAT.
  run(R"cs(
v:IndexedVec = indexed_vec(4)
kfrom init 1
n:i = indexed_vec_len(v[kfrom][3])
chnset n, "length"
)cs");
  channel("length", 2);
}

TEST_F(PluginIndex, UdoReadsAndWritesItsArgument) {
  run(R"cs(
v:IndexedVec = indexed_vec(3)
ir = setfirst(v, 42)
chnset ir, "returned"
)cs", R"cs(
opcode setfirst(x:IndexedVec, value:i):i
  x[0] = value
  xout x[0]
endop
)cs");
  channel("returned", 42);
}

TEST_F(PluginIndex, OpcodeWritesTheElementAtTheRateItIsRead) {
  // Not an assignment: the value takes the type v[...] has when read, not
  // the type of the opcode's inputs. line has i inputs and only a k output,
  // which a k index takes; a constant index takes len's i output.
  run(R"cs(
v:IndexedVec = indexed_vec(4)
kc init 1
v[kc] line 5, 1, 5
v[2] indexed_vec_len v
kv = v[kc]
ix = v[2]
chnset kv, "k"
chnset ix, "i"
)cs");
  channel("k", 5); channel("i", 4);
}

TEST_F(PluginIndex, RegisteredStructCanProvideIndexing) {
  run(R"cs(
p:IndexedPair init 1, 2
iy = p[1]
p[0] = 9
chnset iy, "read"
chnset p.x, "written"
)cs");
  channel("read", 2); channel("written", 9);
}

TEST_F(PluginIndex, TypeWithoutASetterStillRejectsAssignment) {
  EXPECT_NE(csoundCompileOrc(csound, R"cs(
instr 1
  o:OpaqueVec = opaque_vec(2)
  o[0] = 1
endin
)cs", 0), 0);
  EXPECT_NE(messages().find("is not an array"), std::string::npos);
}
} // namespace
