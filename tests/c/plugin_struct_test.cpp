/* Public plugin interface integration tests. SPDX-License-Identifier: LGPL-2.1-or-later */
#include "gtest/gtest.h"
#include <string>

// Load C++ headers before csdl.h: its _CR macro conflicts with libstdc++.
#include "csound.h"
#include "csdl.h"
#include "struct_plugin_path.hpp"

namespace {
class PluginStruct : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;
  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n");
    ASSERT_EQ(csoundLoadPlugins(csound, CSOUND_TEST_STRUCT_PLUGIN_DIR), 0);
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
  void run(const std::string &body) {
    std::string code = "sr=44100\nksmps=32\nnchnls=1\ninstr 1\n" + body + "\nendin\n";
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

TEST_F(PluginStruct, PluginReturnsAndAcceptsItsDeclaredType) {
  run(R"cs(
note:NoteTuplet = plugin_note(60, 100)
pitch:i = plugin_note_pitch(note)
chnset pitch, "pitch"
chnset note.ivelocity, "velocity"
copy:NoteTuplet init note
chnset copy.inote, "copied"
)cs");
  channel("pitch", 60); channel("velocity", 100); channel("copied", 60);
}

TEST_F(PluginStruct, OrchestraCanConstructAndCopyArraysOfPluginTypes) {
  run(R"cs(
note:NoteTuplet init 67, 110
pitch:i = plugin_note_pitch(note)
notes:NoteTuplet[] init 2
notes[0] = note
notes[1] = plugin_note(72, 90)
copy:NoteTuplet[] = notes
chnset pitch, "pitch"
chnset lenarray(copy), "length"
chnset copy[0].ivelocity, "velocity"
chnset copy[1].inote, "last"
)cs");
  channel("pitch", 67); channel("length", 2);
  channel("velocity", 110); channel("last", 72);
}

TEST_F(PluginStruct, ExistingJsonOpcodesWorkWithPluginTypes) {
  run(R"cs(
notes:NoteTuplet[] jsonunmarshal {{[{"inote":60,"ivelocity":100},{"inote":64,"ivelocity":80}]}}
encoded:S jsonmarshal notes
again:NoteTuplet[] jsonunmarshal encoded
pitch:i = plugin_note_pitch(again[1])
chnset pitch, "pitch"
chnset again[0].ivelocity, "velocity"
)cs");
  channel("pitch", 64); channel("velocity", 100);
}

TEST_F(PluginStruct, OpcodeRejectsAnotherStructWithTheSameFields) {
  EXPECT_NE(csoundCompileOrc(csound, R"cs(
struct OtherNote inote:i, ivelocity:i
instr 1
  note:OtherNote init 60, 100
  pitch:i = plugin_note_pitch(note)
endin
)cs", 0), 0);
}

TEST_F(PluginStruct, RegistrationValidatesWithoutChangingExistingTypes) {
  const CS_TYPE *original = csound->GetType(csound, "NoteTuplet");
  CSOUND_STRUCT_MEMBER fields[] = {{"changed", "S"}};
  EXPECT_EQ(csound->RegisterStruct(csound, "NoteTuplet", fields, 1), nullptr);
  EXPECT_EQ(csound->GetType(csound, "NoteTuplet"), original);
  EXPECT_EQ(csound->RegisterStruct(csound, "i", fields, 1), nullptr);
  EXPECT_EQ(csound->RegisterStruct(csound, "namespace:Note", fields, 1), nullptr);
  EXPECT_EQ(csound->RegisterStruct(csound, "Empty", fields, 0), nullptr);
  EXPECT_EQ(csound->RegisterStruct(csound, "Missing", nullptr, 1), nullptr);
  fields[0].type = "NotRegistered";
  EXPECT_EQ(csound->RegisterStruct(csound, "BadType", fields, 1), nullptr);
  EXPECT_EQ(csound->GetType(csound, "BadType"), nullptr);
  fields[0].type = "Direct";
  EXPECT_EQ(csound->RegisterStruct(csound, "Direct", fields, 1), nullptr);
  CSOUND_STRUCT_MEMBER duplicate[] = {{"value", "i"}, {"value", "S"}};
  EXPECT_EQ(csound->RegisterStruct(csound, "Duplicate", duplicate, 2), nullptr);
  EXPECT_EQ(csound->GetType(csound, "Duplicate"), nullptr);
}

TEST_F(PluginStruct, MembersCanUseRegisteredTypesAndRecursiveArrays) {
  const CSOUND_STRUCT_MEMBER fields[] = {
    {"note", "NoteTuplet"}, {"children", "Tree[]"}
  };
  ASSERT_NE(csound->RegisterStruct(csound, "Tree", fields, 2), nullptr);
  run(R"cs(
note:NoteTuplet = plugin_note(60, 100)
children:Tree[] init 0
tree:Tree init note, children
chnset tree.note.inote, "nested"
chnset lenarray(tree.children), "children"
)cs");
  channel("nested", 60); channel("children", 0);
}

TEST_F(PluginStruct, OrchestraCannotRedefinePluginType) {
  EXPECT_NE(csoundCompileOrc(csound, "struct NoteTuplet changed:S\n", 0), 0);
  const CS_TYPE *type = csound->GetType(csound, "NoteTuplet");
  EXPECT_STREQ(static_cast<CS_VARIABLE *>(type->members->value)->varName, "inote");
}

TEST_F(PluginStruct, DefinitionsAreCopiedAndOwnedByEachInstance) {
  char fieldName[] = "value";
  char fieldType[] = "i";
  const CSOUND_STRUCT_MEMBER fields[] = {{fieldName, fieldType}};
  const CS_TYPE *type = csound->RegisterStruct(csound, "LocalType", fields, 1);
  ASSERT_NE(type, nullptr);
  fieldName[0] = 'x'; fieldType[0] = 'S';
  EXPECT_TRUE(type->userDefinedType & CS_TYPE_PLUGIN_DEFINED);
  CSOUND *other = csoundCreate(nullptr, nullptr);
  ASSERT_NE(other, nullptr);
  EXPECT_EQ(other->GetType(other, "LocalType"), nullptr);
  csoundDestroy(other);
  run("value:LocalType init 42\nchnset value.value, \"value\"\n");
  channel("value", 42);
}

TEST_F(PluginStruct, PluginTypesCanBeRegisteredAgainAfterReset) {
  csoundReset(csound);
  EXPECT_EQ(csound->GetType(csound, "NoteTuplet"), nullptr);
  csoundSetOption(csound, "-n");
  ASSERT_EQ(csoundLoadPlugins(csound, CSOUND_TEST_STRUCT_PLUGIN_DIR), 0);
  ASSERT_NE(csound->GetType(csound, "NoteTuplet"), nullptr);
  run("note:NoteTuplet = plugin_note(69, 90)\nchnset note.inote, \"note\"\n");
  channel("note", 69);
}
} // namespace
