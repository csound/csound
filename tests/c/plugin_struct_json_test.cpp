/* Public plugin interface integration tests. SPDX-License-Identifier: LGPL-2.1-or-later */
#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

// Load C++ headers before csdl.h: its _CR macro conflicts with libstdc++.
#include "csound.h"
#include "csdl.h"
#include "json_struct_plugin_path.hpp"

namespace {
class PluginStructJson : public ::testing::Test {
protected:
  CSOUND *csound = nullptr;
  const CSOUND_JSON_API *json = nullptr;
  std::filesystem::path file, directory;
  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n");
    ASSERT_EQ(csoundLoadPlugins(csound, CSOUND_TEST_JSON_PLUGIN_DIR), 0);
    json = csound->GetJsonAPI(CSOUND_JSON_API_VERSION);
    ASSERT_NE(json, nullptr);
  }
  void TearDown() override {
    csoundDestroy(csound);
    if (!file.empty()) {
      std::error_code ignored;
      std::filesystem::remove(file, ignored);
      std::filesystem::remove(directory, ignored);
    }
  }
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
  }
  void channel(const char *name, cs_float expected) {
    int32_t error;
    EXPECT_EQ(csoundGetControlChannel(csound, name, &error), expected);
    EXPECT_EQ(error, 0);
  }
  void write(const std::string &text) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    for (int attempt = 0; attempt < 100; ++attempt) {
      auto candidate = std::filesystem::temp_directory_path() /
        ("csound-plugin-json-" + std::to_string(stamp) + "-" + std::to_string(attempt));
      if (std::filesystem::create_directory(candidate)) {
        directory = candidate;
        break;
      }
    }
    ASSERT_FALSE(directory.empty());
    file = directory / "notes.json";
    std::ofstream out(file, std::ios::binary);
    out << text;
    ASSERT_TRUE(out.good());
  }
};

TEST_F(PluginStructJson, PluginRegistersGlobalTypeAndConvertsNotes) {
  run(R"cs(
notes:NoteTuplet[] plugin_notes {{[{"note":60,"velocity":100},{"note":64,"velocity":80}]}}
chnset lenarray(notes), "length"
chnset notes[0].inote, "note0"
chnset notes[1].ivelocity, "velocity1"
encoded:S plugin_jsonencode notes
again:NoteTuplet[] jsonunmarshal encoded
chnset again[1].inote, "roundtrip"
copy:NoteTuplet init again[0]
chnset copy.ivelocity, "copied"
)cs");
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  channel("length", 2); channel("note0", 60); channel("velocity1", 80);
  channel("roundtrip", 64); channel("copied", 100);
}

TEST_F(PluginStructJson, PluginReadsJsonFileAndInitializesDeclaredStruct) {
  write("[{\"note\":72,\"velocity\":90}]");
  run("notes:NoteTuplet[] plugin_notesfile \"" + file.string() + R"cs("
chnset notes[0].inote, "note"
value:NoteTuplet init 60, 100
encoded:S jsonmarshal value
restored:NoteTuplet plugin_jsondecode encoded
chnset restored.ivelocity, "velocity"
)cs");
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages();
  channel("note", 72); channel("velocity", 100);
}

TEST_F(PluginStructJson, PluginCallsUnmarshalFile) {
  write("{\"inote\":67,\"ivelocity\":110}");
  run("note:NoteTuplet plugin_jsondecodefile \"" + file.string() +
      "\"\nchnset note.inote, \"note\"\n");
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages(); channel("note", 67);
}

TEST_F(PluginStructJson, RejectsBadNoteData) {
  run(R"cs(notes:NoteTuplet[] plugin_notes {{[{"note":60,"velocity":128}]}})cs");
  EXPECT_GT(csoundErrCnt(csound), 0);
  EXPECT_NE(messages().find("integers from 0 to 127"), std::string::npos);
}
TEST_F(PluginStructJson, RejectsMissingFields) {
  run(R"cs(notes:NoteTuplet[] plugin_notes {{[{"note":60}]}})cs");
  EXPECT_GT(csoundErrCnt(csound), 0);
  EXPECT_NE(messages().find("numeric note and velocity"), std::string::npos);
}
TEST_F(PluginStructJson, RejectsInvalidJson) {
  run(R"cs(notes:NoteTuplet[] plugin_notes "[")cs");
  EXPECT_GT(csoundErrCnt(csound), 0);
  EXPECT_NE(messages().find("at byte"), std::string::npos);
}
TEST_F(PluginStructJson, EmptyNotesArray) {
  run(R"cs(notes:NoteTuplet[] plugin_notes "[]"
chnset lenarray(notes), "length")cs");
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages(); channel("length", 0);
}

TEST_F(PluginStructJson, RegistrationValidatesWithoutChangingExistingTypes) {
  const CS_TYPE *original = csound->GetType(csound, "NoteTuplet");
  CSOUND_STRUCT_MEMBER fields[] = {{"changed", "S"}};
  EXPECT_EQ(csound->RegisterStruct(csound, "NoteTuplet", fields, 1), nullptr);
  EXPECT_EQ(csound->GetType(csound, "NoteTuplet"), original);
  EXPECT_EQ(csound->RegisterStruct(csound, "i", fields, 1), nullptr);
  EXPECT_EQ(csound->RegisterStruct(csound, "namespace:Note", fields, 1), nullptr);
  fields[0].type = "NotRegistered";
  EXPECT_EQ(csound->RegisterStruct(csound, "BadType", fields, 1), nullptr);
  EXPECT_EQ(csound->GetType(csound, "BadType"), nullptr);
  CSOUND_STRUCT_MEMBER duplicate[] = {{"value", "i"}, {"value", "S"}};
  EXPECT_EQ(csound->RegisterStruct(csound, "Duplicate", duplicate, 2), nullptr);
  EXPECT_EQ(csound->GetType(csound, "Duplicate"), nullptr);
  CSOUND_STRUCT_MEMBER recursive[] = {{"note", "NoteTuplet"}, {"children", "Tree[]"}};
  ASSERT_NE(csound->RegisterStruct(csound, "Tree", recursive, 2), nullptr);
  run(R"cs(tree:Tree plugin_jsondecode {{ {"note":{"inote":60,"ivelocity":100},"children":[]} }}
chnset tree.note.inote, "nested")cs");
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages(); channel("nested", 60);
}

TEST_F(PluginStructJson, OrchestraCannotRedefinePluginType) {
  EXPECT_NE(csoundCompileOrc(csound, "struct NoteTuplet changed:S\n", 0), 0);
  const CS_TYPE *type = csound->GetType(csound, "NoteTuplet");
  EXPECT_STREQ(static_cast<CS_VARIABLE *>(type->members->value)->varName, "inote");
}

TEST_F(PluginStructJson, DefinitionsAreCopiedAndOwnedByEachInstance) {
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
  EXPECT_EQ(csoundErrCnt(csound), 0) << messages(); channel("value", 42);
}

TEST_F(PluginStructJson, PluginTypesCanBeRegisteredAgainAfterReset) {
  csoundReset(csound);
  EXPECT_EQ(csound->GetType(csound, "NoteTuplet"), nullptr);
  ASSERT_EQ(csoundLoadPlugins(csound, CSOUND_TEST_JSON_PLUGIN_DIR), 0);
  ASSERT_NE(csound->GetType(csound, "NoteTuplet"), nullptr);
}

TEST_F(PluginStructJson, DocumentsSupportValidationAndEdits) {
  EXPECT_EQ(csound->GetJsonAPI(999), nullptr);
  EXPECT_EQ(json->version, CSOUND_JSON_API_VERSION);
  EXPECT_GE(json->size, sizeof(*json));
  const char *input = R"({"name":"old","gain":1,"enabled":true,"unused":null,"items":[1,2]})";
  CSOUND_JSON_ERROR error;
  auto *doc = json->Parse(csound, input, std::strlen(input), 0, &error);
  ASSERT_NE(doc, nullptr) << error.message;
  auto *root = json->Root(doc);
  EXPECT_EQ(json->Kind(root), CSOUND_JSON_OBJECT);
  EXPECT_EQ(json->Kind(json->Member(root, "missing")), CSOUND_JSON_INVALID);
  EXPECT_EQ(json->Boolean(json->Member(root, "enabled")), 1);
  EXPECT_EQ(json->Kind(json->Member(root, "unused")), CSOUND_JSON_NULL);
  EXPECT_EQ(json->Size(json->Member(root, "items")), 2);
  EXPECT_EQ(json->Element(json->Member(root, "items"), 2), nullptr);
  EXPECT_EQ(json->SetNumber(json->Member(root, "gain"), 0.5), OK);
  EXPECT_EQ(json->SetNumber(json->Member(root, "gain"), std::numeric_limits<double>::infinity()), NOTOK);
  EXPECT_EQ(json->SetString(doc, json->Member(root, "name"), "new"), OK);
  EXPECT_STREQ(json->String(json->Member(root, "name")), "new");
  EXPECT_EQ(json->Rename(doc, root, "gain", "name"), NOTOK);
  EXPECT_EQ(json->Rename(doc, root, "name", "label"), OK);
  EXPECT_EQ(json->Remove(root, "unused"), OK);
  STRINGDAT output = {};
  EXPECT_EQ(json->Write(csound, doc, &output, 0), OK);
  EXPECT_NE(std::string(output.data).find("\"label\":\"new\""), std::string::npos);
  auto *again = json->Parse(csound, output.data, std::strlen(output.data), 0, &error);
  ASSERT_NE(again, nullptr) << error.message;
  EXPECT_EQ(json->Number(json->Member(json->Root(again), "gain")), 0.5);
  json->Free(again); json->Free(doc); csound->Free(csound, output.data);
}

TEST_F(PluginStructJson, ParserRejectsInvalidOptionsAndExcessDepth) {
  CSOUND_JSON_ERROR error;
  EXPECT_EQ(json->Parse(csound, "{}", 2, 4, &error), nullptr);
  EXPECT_NE(std::string(error.message).find("flags"), std::string::npos);
  EXPECT_EQ(json->Parse(csound, "[", 1, 0, &error), nullptr);
  EXPECT_FALSE(std::string(error.message).empty());
  std::string deep(257, '['); deep += "0"; deep += std::string(257, ']');
  EXPECT_EQ(json->Parse(csound, deep.c_str(), deep.size(), 0, &error), nullptr);
  EXPECT_STREQ(error.message, "maximum nesting depth exceeded");
  const char *comments = "[1,/* allowed */]";
  auto *doc = json->Parse(csound, comments, std::strlen(comments), 3, &error);
  ASSERT_NE(doc, nullptr) << error.message; json->Free(doc);
}
} // namespace
