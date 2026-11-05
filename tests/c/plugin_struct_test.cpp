/* Public plugin interface integration tests. SPDX-License-Identifier: LGPL-2.1-or-later */
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

// Load C++ headers before csdl.h: its _CR macro conflicts with libstdc++.
#include "csound.h"
#include "csdl.h"
#include "struct_plugin_path.hpp"

namespace {
std::string drainMessages(CSOUND *csound) {
  std::string result;
  while (csoundGetMessageCnt(csound)) {
    result += csoundGetFirstMessage(csound);
    csoundPopFirstMessage(csound);
  }
  return result;
}

// Default plugin directories may add types, so check the total against the
// listing itself rather than against a fixed number.
void expectTotalMatchesListing(const std::string &listing) {
  size_t count = listing.rfind("struct ", 0) == 0 ? 1 : 0;
  for (size_t at = listing.find("\nstruct "); at != std::string::npos;
       at = listing.find("\nstruct ", at + 1))
    ++count;
  EXPECT_NE(listing.find("\n" + std::to_string(count) + " structs\n"),
            std::string::npos) << listing;
}

struct StructListing {
  int status;
  std::string text;
};

// Use a new instance, so that only pluginDir and the options can load plugins.
StructListing listStructs(const std::string &pluginDir,
                          const std::vector<std::string> &options) {
  CSOUND *csound = csoundCreate(nullptr, pluginDir.c_str());
  csoundCreateMessageBuffer(csound, 0);
  std::vector<const char *> args{"csound"};
  for (const std::string &option : options)
    args.push_back(option.c_str());
  StructListing listing;
  listing.status =
      csoundCompile(csound, static_cast<int>(args.size()), args.data());
  listing.text = drainMessages(csound);
  csoundDestroy(csound);
  return listing;
}

class EmptyPluginDir {
public:
  EmptyPluginDir() {
    path_ = std::filesystem::temp_directory_path() /
            (std::string("csound_list_structs_") +
             ::testing::UnitTest::GetInstance()->current_test_info()->name());
    std::filesystem::create_directories(path_);
  }
  ~EmptyPluginDir() { std::filesystem::remove_all(path_); }
  std::string string() const { return path_.string(); }

private:
  std::filesystem::path path_;
};

const char *const kFixtureStruct = "struct NoteTuplet inote:i, ivelocity:i\n";

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
    return drainMessages(csound);
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

TEST_F(PluginStruct, ListStructsPrintsPluginTypesInOrchestraSyntax) {
  const CSOUND_STRUCT_MEMBER fields[] = {
      {"note", "NoteTuplet"}, {"notes", "NoteTuplet[]"}, {"values", "k[]"}};
  ASSERT_NE(csound->RegisterStruct(csound, "ListedGroup", fields, 3), nullptr);
  messages();
  const char *args[] = {"csound", "--list-structs"};
  csoundCompile(csound, 2, args);
  const std::string listing = messages();
  EXPECT_NE(listing.find("struct NoteTuplet inote:i, ivelocity:i\n"),
            std::string::npos) << listing;
  EXPECT_NE(listing.find("struct ListedGroup note:NoteTuplet, "
                         "notes:NoteTuplet[], values:k[]\n"),
            std::string::npos) << listing;
  expectTotalMatchesListing(listing);
}

TEST_F(PluginStruct, ListStructsStopsBeforeCompilingAFile) {
  const std::filesystem::path file =
      std::filesystem::temp_directory_path() / "csound_list_structs_test.csd";
  {
    std::ofstream csd(file);
    csd << "<CsoundSynthesizer>\n<CsInstruments>\n"
           "struct FromFile value:i\n"
           "instr 1\n  prints \"performed\\n\"\nendin\n"
           "</CsInstruments>\n<CsScore>\ni 1 0 0\n</CsScore>\n"
           "</CsoundSynthesizer>\n";
  }
  messages();
  const std::string path = file.string();
  const char *args[] = {"csound", "-n", "--list-structs", path.c_str()};
  csoundCompile(csound, 4, args);
  const std::string listing = messages();
  std::filesystem::remove(file);
  EXPECT_NE(listing.find("struct NoteTuplet inote:i, ivelocity:i\n"),
            std::string::npos) << listing;
  EXPECT_EQ(listing.find("FromFile"), std::string::npos) << listing;
  EXPECT_EQ(listing.find("performed"), std::string::npos) << listing;
  expectTotalMatchesListing(listing);
}

TEST(PluginStructListing, LoadsDefaultPluginsAndReportsSuccess) {
  const StructListing listing =
      listStructs(CSOUND_TEST_STRUCT_PLUGIN_DIR, {"--list-structs"});
  EXPECT_EQ(listing.status, CSOUND_EXITJMP_SUCCESS) << listing.text;
  EXPECT_NE(listing.text.find(kFixtureStruct), std::string::npos)
      << listing.text;
  expectTotalMatchesListing(listing.text);
}

TEST(PluginStructListing, AppliesPluginDirectoryGivenAfterTheOption) {
  const EmptyPluginDir empty;
  const std::string dir = empty.string();
  EXPECT_EQ(listStructs(dir, {"--list-structs"}).text.find(kFixtureStruct),
            std::string::npos);
  const StructListing listing = listStructs(
      dir, {"--list-structs",
            std::string("--opcode-dir=") + CSOUND_TEST_STRUCT_PLUGIN_DIR});
  EXPECT_EQ(listing.status, CSOUND_EXITJMP_SUCCESS) << listing.text;
  EXPECT_NE(listing.text.find(kFixtureStruct), std::string::npos)
      << listing.text;
  expectTotalMatchesListing(listing.text);
}

TEST(PluginStructListing, AppliesPluginLibraryGivenAfterTheOption) {
  std::string library;
  for (const auto &entry : std::filesystem::directory_iterator(
           CSOUND_TEST_STRUCT_PLUGIN_DIR)) {
    const std::string extension = entry.path().extension().string();
    if (extension == ".so" || extension == ".dylib" || extension == ".dll")
      library = entry.path().string();
  }
  ASSERT_FALSE(library.empty());
  const EmptyPluginDir empty;
  const StructListing listing = listStructs(
      empty.string(), {"--list-structs", "--opcode-lib=" + library});
  EXPECT_EQ(listing.status, CSOUND_EXITJMP_SUCCESS) << listing.text;
  EXPECT_NE(listing.text.find(kFixtureStruct), std::string::npos)
      << listing.text;
  expectTotalMatchesListing(listing.text);
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
