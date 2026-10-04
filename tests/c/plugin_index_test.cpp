/* [] on a plugin-defined type. SPDX-License-Identifier: LGPL-2.1-or-later
   The rules these tests pin are written up in docs/plugin-indexing.md. */
#include "gtest/gtest.h"
#include <functional>
#include <string>
#include <vector>

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
  static std::string orchestra(const std::string &body, const std::string &udos) {
    return "sr=44100\nksmps=32\nnchnls=1\n" + udos + "instr 1\n" + body + "\nendin\n";
  }
  void start(const std::string &body, const std::string &udos, const char *score) {
    ASSERT_EQ(csoundCompileOrc(csound, orchestra(body, udos).c_str(), 0), 0) << messages();
    csoundEventString(csound, score, 0);
    ASSERT_EQ(csoundStart(csound), 0) << messages();
  }
  // Runs one short note to its end.
  void run(const std::string &body, const std::string &udos = "") {
    start(body, udos, "i 1 0 0.01\ne\n");
    int result = 0;
    for (int i = 0; i < 100 && result == 0; ++i) result = csoundPerformKsmps(csound);
    ASSERT_GT(result, 0) << messages();
    EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  }
  // Runs a long note for the given number of control blocks, calling check
  // after each one with the block's number, from 0.
  void runBlocks(const std::string &body, int blocks,
                 const std::function<void(int)> &check, const std::string &udos = "") {
    start(body, udos, "i 1 0 10\n");
    for (int block = 0; block < blocks; ++block) {
      ASSERT_EQ(csoundPerformKsmps(csound), 0) << messages();
      SCOPED_TRACE("control block " + std::to_string(block));
      check(block);
    }
    EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  }
  void expectCompileError(const std::string &body, const std::string &expected,
                          const std::string &udos = "") {
    EXPECT_NE(csoundCompileOrc(csound, orchestra(body, udos).c_str(), 0), 0);
    std::string text = messages();
    EXPECT_NE(text.find(expected), std::string::npos) << text;
  }
  // Compiles, then expects a run-time error with the given text.
  void expectRunError(const std::string &body, const std::string &expected) {
    start(body, "", "i 1 0 0.01\ne\n");
    int result = 0;
    for (int i = 0; i < 100 && result == 0; ++i) result = csoundPerformKsmps(csound);
    std::string text = messages();
    EXPECT_NE(text.find(expected), std::string::npos) << text;
  }
  cs_float value(const char *name) {
    int32_t error;
    cs_float result = csoundGetControlChannel(csound, name, &error);
    EXPECT_EQ(error, 0) << name;
    return result;
  }
  void channel(const char *name, cs_float expected) {
    EXPECT_EQ(value(name), expected) << name;
  }
};

// indexed_vec(n) holds 10 * i at index i.

/* ---- Reads and writes ---- */

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

/* ---- Rates, block by block ---- */

TEST_F(PluginIndex, ConstantIndexReadFollowsKRateWrites) {
  // As for a k array: a perf-time use reads the element every block.
  runBlocks(R"cs(
v:IndexedVec = indexed_vec(4)
kc init 0
kc += 1
v[0] = kc
kvalue = v[0]
chnset kvalue, "value"
)cs", 5, [&](int block) { channel("value", block + 1); });
}

TEST_F(PluginIndex, OpcodeWithIAndKVersionsTakesTheIOneForIIndices) {
  // With i indices the element is typed by its init-time read, so chnset
  // resolves to its i version and sends the init value once; a k variable
  // or a k index makes it follow.
  runBlocks(R"cs(
v:IndexedVec = indexed_vec(4)
kc init 0
kc += 1
v[0] = kc
chnset v[0], "constant"
kvalue = v[0]
chnset kvalue, "variable"
kzero init 0
chnset v[kzero], "kindex"
)cs", 4, [&](int block) {
    channel("constant", 0);
    channel("variable", block + 1);
    channel("kindex", block + 1);
  });
}

TEST_F(PluginIndex, TypeWithOnlyAnIReadIsReadAtInitInAPerfContext) {
  // IndexedPair has no k read: kv = p[1] uses the i read, at init time.
  run(R"cs(
p:IndexedPair init 1, 2
kv = p[1]
chnset kv, "value"
)cs");
  channel("value", 2);
}

TEST_F(PluginIndex, AssignmentWithAKIndexWritesWhereTheIndexIsEachBlock) {
  // Block b writes 100 + b at b % 4; the other elements keep their values.
  std::vector<cs_float> expected = {0, 10, 20, 30};
  runBlocks(R"cs(
v:IndexedVec = indexed_vec(4)
kb init 0
kidx = kb % 4
v[kidx] = 100 + kb
k0 init 0
k1 init 1
k2 init 2
k3 init 3
chnset v[k0], "e0"
chnset v[k1], "e1"
chnset v[k2], "e2"
chnset v[k3], "e3"
kb += 1
)cs", 6, [&](int block) {
    expected[block % 4] = 100 + block;
    channel("e0", expected[0]); channel("e1", expected[1]);
    channel("e2", expected[2]); channel("e3", expected[3]);
  });
}

TEST_F(PluginIndex, AssignmentWithAKIndexRunsAtPerfTimeOnly) {
  // As kvar = 7: a k index makes the write k-rate, so the init-time read
  // still sees the original element.
  run(R"cs(
v:IndexedVec = indexed_vec(4)
kj init 2
v[kj] = 7
ix = v[2]
kx = v[kj]
chnset ix, "init"
chnset kx, "perf"
)cs");
  channel("init", 20); channel("perf", 7);
}

TEST_F(PluginIndex, AssignmentOfAKValueRunsAtPerfTimeOnly) {
  run(R"cs(
v:IndexedVec = indexed_vec(4)
kval init 5
v[0] = kval
ix = v[0]
kzero init 0
kx = v[kzero]
chnset ix, "init"
chnset kx, "perf"
)cs");
  channel("init", 0); channel("perf", 5);
}

TEST_F(PluginIndex, IRateAssignmentRunsOnceAtInit) {
  // v[1] = 5 is not repeated: the perf-time increments accumulate on it.
  runBlocks(R"cs(
v:IndexedVec = indexed_vec(4)
v[1] = 5
kone init 1
v[kone] = v[kone] + 1
chnset v[kone], "e1"
)cs", 4, [&](int block) { channel("e1", 5 + block + 1); });
}

TEST_F(PluginIndex, InitWritesOnceAcrossBlocks) {
  // kj moves to 3 at perf time: an init write repeated then would reach v[3].
  runBlocks(R"cs(
v:IndexedVec = indexed_vec(4)
kj init 2
v[kj] init 7
kj = 3
k2 init 2
k3 init 3
chnset v[k2], "e2"
chnset v[k3], "e3"
)cs", 4, [&](int) { channel("e2", 7); channel("e3", 30); });
}

/* ---- Overload selection ---- */

TEST_F(PluginIndex, RegistrationOrderDoesNotChangeTheChosenEntries) {
  // ReversedVec registers IndexedVec's entries in reverse order.
  for (const char *type : {"IndexedVec", "ReversedVec"}) {
    SCOPED_TRACE(type);
    TearDown(); SetUp();
    std::string t = type;
    std::string make = t == "IndexedVec" ? "indexed_vec" : "reversed_vec";
    runBlocks("v:" + t + " = " + make + R"cs((4)
v[0] = 99
ix = v[0]
iy = v[2]
kj init 3
v[kj] init 7
iz = v[3]
kc init 0
kc += 1
v[1] = kc
kv = v[1]
chnset ix, "ix"
chnset iy, "iy"
chnset iz, "iz"
chnset kv, "kv"
)cs", 3, [&](int block) {
      channel("ix", 99); channel("iy", 20); channel("iz", 7);
      channel("kv", block + 1);
    });
  }
}

TEST_F(PluginIndex, EntriesWithTheSameSignatureAreAmbiguous) {
  // Only the init-time read meets the two i getters; the k getter, which
  // also takes an i index, ranks below them there.
  expectCompileError(R"cs(
a:AmbiguousVec = ambiguous_vec(2)
ix = a[0]
)cs", "ambiguous ##array_get entries of type AmbiguousVec: 2 take arg types AmbiguousVec, i");
}

TEST_F(PluginIndex, AnUnambiguousAccessOfTheSameTypeCompiles) {
  run(R"cs(
a:AmbiguousVec = ambiguous_vec(2)
kidx init 1
kx = a[kidx]
chnset kx, "value"
)cs");
  channel("value", 10);
}

TEST_F(PluginIndex, SettersWithTheSameSignatureAreAmbiguous) {
  expectCompileError(R"cs(
a:AmbiguousVec = ambiguous_vec(2)
kidx init 0
a[kidx] = 1
)cs", "ambiguous ##array_set entries of type AmbiguousVec: 2 take arg types AmbiguousVec, k, k");
}

TEST_F(PluginIndex, ValueTypeWithoutASetterIsRejected) {
  expectCompileError(R"cs(
v:IndexedVec = indexed_vec(4)
v[0] = "text"
)cs", "no ##array_set entry of type IndexedVec takes arg types IndexedVec, S, i");
}

TEST_F(PluginIndex, IndexTypeWithoutAnEntryIsRejected) {
  expectCompileError(R"cs(
v:IndexedVec = indexed_vec(4)
ix = v["a"]
)cs", "no ##array_get entry of type IndexedVec takes arg types IndexedVec, S");
}

TEST_F(PluginIndex, SetterTakesItsValueButNotTheIndexCount) {
  // The setters take one index; two match the value but not the indices.
  expectCompileError(R"cs(
v:IndexedVec = indexed_vec(4)
v[0][1] = 5
)cs", "no ##array_set entry of type IndexedVec takes arg types IndexedVec, i, i, i");
}

TEST_F(PluginIndex, MoreIndicesThanAnyGetterTakesAreRejected) {
  expectCompileError(R"cs(
v:IndexedVec = indexed_vec(4)
ix = v[0][1][2]
)cs", "no ##array_get entry of type IndexedVec takes arg types IndexedVec, i, i, i");
}

/* ---- Multiple indices and ownership ---- */

TEST_F(PluginIndex, TwoIndicesAreOneAccessThatReturnsACopy) {
  // v[1][3] is the plugin's slice [1, 3), not v[1] indexed again; writing
  // to the slice leaves v unchanged, at init and at perf time.
  runBlocks(R"cs(
v:IndexedVec = indexed_vec(4)
k1 init 1
kz init 0
s:IndexedVec = v[k1][3]
n:i = indexed_vec_len(s)
s[kz] = 999
chnset n, "length"
chnset s[kz], "slice"
chnset v[k1], "original"
)cs", 3, [&](int) {
    channel("length", 2); channel("slice", 999); channel("original", 10);
  });
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

/* ---- Invalid access ---- */

TEST_F(PluginIndex, FractionalIndexIsTruncatedByThePlugin) {
  run(R"cs(
v:IndexedVec = indexed_vec(4)
ix = v[1.7]
chnset ix, "value"
)cs");
  channel("value", 10);
}

// Lines count from the start of the orchestra: sr, ksmps, nchnls and instr
// come first, so the body's first statement is on line 6.
TEST_F(PluginIndex, NegativeIndexIsAnInitError) {
  expectRunError(R"cs(
v:IndexedVec = indexed_vec(4)
ix = v[-1]
)cs", "INIT ERROR in instr 1 (opcode ##array_get.IVi) line 7: IndexedVec read out of range");
}

TEST_F(PluginIndex, OutOfRangeWriteIsAnInitError) {
  expectRunError(R"cs(
v:IndexedVec = indexed_vec(4)
v[9] = 1
)cs", "INIT ERROR in instr 1 (opcode ##array_set.IVi) line 7: IndexedVec write out of range");
}

TEST_F(PluginIndex, OutOfRangeReadIsAPerfError) {
  expectRunError(R"cs(
v:IndexedVec = indexed_vec(4)
kidx init 2
kx = v[kidx]
kidx += 1
)cs", "IndexedVec read out of range");
  EXPECT_GT(csoundErrCnt(csound), 0);
}

TEST_F(PluginIndex, NegativeKIndexWriteIsAPerfError) {
  expectRunError(R"cs(
v:IndexedVec = indexed_vec(4)
kidx init 1
v[kidx] = 1
kidx -= 1
)cs", "IndexedVec write out of range");
  EXPECT_GT(csoundErrCnt(csound), 0);
}

TEST_F(PluginIndex, TypeWithoutAGetterRejectsReading) {
  expectCompileError(R"cs(
o:OpaqueVec = opaque_vec(2)
ix = o[0]
)cs", "variable o of type OpaqueVec cannot be read with []");
}

TEST_F(PluginIndex, TypeWithoutASetterStillRejectsAssignment) {
  expectCompileError(R"cs(
o:OpaqueVec = opaque_vec(2)
o[0] = 1
)cs", "is not an array");
}

TEST_F(PluginIndex, TypeWithoutAnInitWriterRejectsInit) {
  expectCompileError(R"cs(
p:IndexedPair init 1, 2
p[0] init 3
)cs", "no ##array_init entry of type IndexedPair");
}

/* ---- Other expressions ---- */

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

TEST_F(PluginIndex, OperatorsResolveThroughTheTypesEntries) {
  // + is not special-cased: it is the "##add" entry the plugin registers.
  run(R"cs(
v:IndexedVec = indexed_vec(4)
w:IndexedVec = v + v
ix = w[3]
chnset ix, "sum"
)cs");
  channel("sum", 60);
}

TEST_F(PluginIndex, IndexingAStructMemberIsNotSupported) {
  expectCompileError(R"cs(
v:IndexedVec = indexed_vec(4)
r:Holder init v
ix = r.vec[1]
)cs", "non-array type for struct member access", "struct Holder vec:IndexedVec\n");
}

TEST_F(PluginIndex, IndexingACallResultIsNotSupported) {
  expectCompileError(R"cs(
ix = indexed_vec(4)[2]
)cs", "non-array type for function indexed_vec");
}

const char *countingIndex = R"cs(
gicount init 0
gkcount init 0
opcode ione():i
  gicount += 1
  ivalue = 1
  xout ivalue
endop
opcode kone():k
  gkcount += 1
  kvalue = 1
  xout kvalue
endop
)cs";

TEST_F(PluginIndex, IndexExpressionRunsOncePerAccess) {
  // Two accesses at init time, three at perf time in every block.
  runBlocks(R"cs(
v:IndexedVec = indexed_vec(4)
v[ione()] = v[ione()] + 1
iinit = v[1]
v[kone()] = v[kone()] + 1
kv = v[kone()]
chnset gicount, "icount"
chnset iinit, "init"
chnset gkcount, "kcount"
chnset kv, "perf"
)cs", 3, [&](int block) {
    channel("icount", 2); channel("init", 11);
    channel("kcount", 3 * (block + 1)); channel("perf", 11 + block + 1);
  }, countingIndex);
}
} // namespace
