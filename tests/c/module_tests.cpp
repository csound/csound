#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "csound_module.h"
#include "gtest/gtest.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace {
class ModuleSystemTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    std::filesystem::path directory;
    std::optional<std::string> previousRc;

    void SetUp() override
    {
        csoundInitialize(CSOUNDINIT_NO_SIGNAL_HANDLER | CSOUNDINIT_NO_ATEXIT);
        if (const char *value = csoundGetEnv(nullptr, "CSOUND7RC")) previousRc = value;
        ASSERT_EQ(csoundSetGlobalEnv("CSOUND7RC", ""), 0);
        directory = std::filesystem::temp_directory_path() /
            ("csound-modules-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        ASSERT_TRUE(std::filesystem::create_directory(directory));
        csound = csoundCreate(nullptr, directory.string().c_str());
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        ASSERT_EQ(csoundSetOption(csound, "-n -d -m0"), 0);
    }

    void TearDown() override
    {
        if (csound != nullptr) csoundDestroy(csound);
        csoundSetGlobalEnv("CSOUND7RC", previousRc ? previousRc->c_str() : nullptr);
        std::error_code error;
        std::filesystem::remove_all(directory, error);
    }

    void write(const char *name, const std::string &code)
    {
        std::ofstream(directory / name) << code;
    }

    std::string path(const char *name) const
    {
        return "\"" + (directory / name).generic_string() + "\"";
    }

    int compile(const std::string &code)
    {
        return csoundCompileOrc(csound, code.c_str(), 0);
    }

    void initial(const std::string &code = "")
    {
        ASSERT_EQ(compile("sr=44100\nksmps=32\nnchnls=2\n0dbfs=1\n" + code), 0);
    }

    double channel(const char *name)
    {
        int error = 0;
        double value = csoundGetControlChannel(csound, name, &error);
        EXPECT_EQ(error, 0);
        return value;
    }
};

TEST_F(ModuleSystemTests, LiveImportPreparesAndInitializesNewDefinitions)
{
    write("library.orc", "giValue=11\nopcode ReadValue,i,0\n xout giValue\nendop\n"
                         "instr Voice\n chnset giValue,\"voice\"\nendin\n");
    initial("giValue=99\n");
    ASSERT_EQ(csoundStart(csound), 0);
    ASSERT_EQ(compile("from " + path("library.orc") + " import ReadValue,Voice\n"
                      "iValue=ReadValue()\nchnset iValue,\"result\"\n"
                      "schedule Voice,0,.01\n"), 0);
    EXPECT_EQ(channel("result"), 11);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(channel("voice"), 11);
}

TEST_F(ModuleSystemTests, SearchPathsAndCanonicalPathsShareOneInstance)
{
    write("library.orc", "giValue=11\n");
    ASSERT_EQ(csoundAddModuleSearchPath(csound, directory.string().c_str()), 0);
    initial("import \"library.orc\" as first\nimport " + path("./library.orc") +
            " as second\nfirst.giValue=99\nchnset second.giValue,\"result\"\n");
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_EQ(channel("result"), 99);
}

TEST_F(ModuleSystemTests, ParseAndCompileTreeHaveTheSameModuleLifecycle)
{
    write("library.orc", "giValue=42\n");
    std::string code = "sr=44100\nksmps=32\nnchnls=2\nfrom " + path("library.orc") +
        " import giValue\nchnset giValue,\"result\"\n";
    TREE *tree = csoundParseOrc(csound, code.c_str());
    ASSERT_NE(tree, nullptr);
    ASSERT_EQ(csoundCompileTree(csound, tree, 0), 0);
    csoundDeleteTree(csound, tree);
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_EQ(channel("result"), 42);
}

TEST_F(ModuleSystemTests, SeveralCompilationsBeforeStartInitializeEachModuleOnce)
{
    write("library.orc", "iPrevious chnget \"count\"\nchnset iPrevious+1,\"count\"\ngiValue=42\n");
    initial("import " + path("library.orc") + " as first\n");
    ASSERT_EQ(compile("import " + path("./library.orc") +
                      " as second\nchnset second.giValue,\"result\"\n"), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_EQ(channel("count"), 1);
    EXPECT_EQ(channel("result"), 42);
}

TEST_F(ModuleSystemTests, RejectedModuleCompilationCanBeRetried)
{
    initial();
    ASSERT_EQ(csoundStart(csound), 0);
    write("library.orc", "giValue=12\ninstr 99\nendin\n");
    EXPECT_NE(compile("import " + path("library.orc") + "\n"), 0);
    write("library.orc", "giValue=42\n");
    ASSERT_EQ(compile("import " + path("library.orc") +
                      "\nchnset giValue,\"result\"\n"), 0);
    EXPECT_EQ(channel("result"), 42);
}

TEST_F(ModuleSystemTests, RejectedImportDoesNotReplaceExistingBindings)
{
    write("first.orc", "giValue=11\nopcode ReadValue,i,0\n xout giValue\nendop\n");
    write("second.orc", "giValue=22\nopcode ReadValue,i,0\n xout giValue\nendop\n");
    initial("import " + path("first.orc") + " as shared\n");
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_NE(compile("import " + path("second.orc") +
                     " as shared\nUndefinedOpcode\n"), 0);
    ASSERT_EQ(compile("iValue=ReadValue()\nchnset iValue,\"value\"\n"
                      "chnset shared.giValue,\"alias\"\n"), 0);
    EXPECT_EQ(channel("value"), 11);
    EXPECT_EQ(channel("alias"), 11);
}

TEST_F(ModuleSystemTests, LiveUdoRedefinitionStaysWithinItsDefiningModule)
{
    write("library.orc", "opcode ReadValue,i,0\n xout 11\nendop\n");
    initial("import " + path("library.orc") + " as library\n"
            "opcode ReadValue,i,0\n xout 22\nendop\n");
    ASSERT_EQ(csoundStart(csound), 0);
    ASSERT_EQ(compile("opcode ReadValue,i,0\n xout 33\nendop\n"
                      "iLocal=ReadValue()\niModule=library.ReadValue()\n"
                      "chnset iLocal,\"local\"\nchnset iModule,\"module\"\n"), 0);
    EXPECT_EQ(channel("local"), 33);
    EXPECT_EQ(channel("module"), 11);
}
TEST_F(ModuleSystemTests, ImportedInstrumentAliasesWorkWithStringScheduling)
{
    write("library.orc", "instr Voice\n iCount chnget \"voice_count\"\n"
                         " chnset iCount+1,\"voice_count\"\nendin\n");
    initial("from " + path("library.orc") + " import Voice as ImportedVoice\n"
            "schedule \"ImportedVoice\",0,.01\n");
    ASSERT_EQ(csoundStart(csound), 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(channel("voice_count"), 1);
}

TEST_F(ModuleSystemTests, RejectedUdoRedefinitionLeavesRunningDefinitionUsable)
{
    initial("opcode ReadValue,i,0\n xout 11\nendop\n");
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_NE(compile("opcode ReadValue,i,0\n xout 99\nendop\nUndefinedOpcode\n"), 0);
    ASSERT_EQ(compile("iValue=ReadValue()\nchnset iValue,\"result\"\n"), 0);
    EXPECT_EQ(channel("result"), 11);
}

TEST_F(ModuleSystemTests, RebindingAnImportedInstrumentDoesNotRetireItsSource)
{
    write("first.orc", "instr Voice\n chnset 11,\"first\"\nendin\n");
    write("second.orc", "instr Voice\n chnset 22,\"second\"\nendin\n");
    initial("import " + path("first.orc") + " as first\n");
    ASSERT_EQ(csoundStart(csound), 0);
    ASSERT_EQ(compile("import " + path("second.orc") +
                      " as second\nschedule \"Voice\",0,.01\n"
                      "schedule first.Voice,0,.01\n"), 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(channel("first"), 11);
    EXPECT_EQ(channel("second"), 22);
}

TEST_F(ModuleSystemTests, OrchestraFileImportsResolveFromTheirOwnDirectory)
{
    write("library.orc", "giValue=42\n");
    write("main.orc", "sr=44100\nksmps=32\nnchnls=2\n"
                      "from \"library.orc\" import giValue\n"
                      "chnset giValue,\"result\"\n");
    write("main.sco", "f0 .01\ne\n");
    const std::string orchestra = (directory / "main.orc").string();
    const std::string score = (directory / "main.sco").string();
    const char *arguments[] = {"csound", orchestra.c_str(), score.c_str()};
    ASSERT_EQ(csoundCompile(csound, 3, arguments), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_EQ(channel("result"), 42);
}

TEST_F(ModuleSystemTests, AsyncImportsWaitForPublicationBeforeInitialization)
{
    write("library.orc", "giValue=42\nchnset 1,\"initialized\"\n"
                         "opcode ReadValue,i,0\n xout giValue\nendop\n");
    initial("chn_k \"result\",3\nchn_k \"initialized\",3\n");
    ASSERT_EQ(csoundStart(csound), 0);
    const std::string edit = "import " + path("library.orc") +
        "\niValue=ReadValue()\nchnset iValue,\"result\"\n";
    ASSERT_EQ(csoundCompileOrc(csound, edit.c_str(), 1), 0);
    EXPECT_EQ(channel("result"), 0);
    EXPECT_EQ(channel("initialized"), 0);
    for (int block=0; block<10 && channel("result")==0; block++)
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(channel("result"), 42);
    EXPECT_EQ(channel("initialized"), 1);
}

TEST_F(ModuleSystemTests, QueuedCompilationsPrepareWithTheirOwnAliasBindings)
{
    write("first.orc", "giValue=11\n");
    write("second.orc", "giValue=22\n");
    initial("chn_k \"first\",3\nchn_k \"second\",3\n");
    ASSERT_EQ(csoundStart(csound), 0);
    const std::string first = "import " + path("first.orc") +
        " as shared\nchnset shared.giValue,\"first\"\n";
    const std::string second = "import " + path("second.orc") +
        " as shared\nchnset shared.giValue,\"second\"\n";
    ASSERT_EQ(csoundCompileOrc(csound, first.c_str(), 1), 0);
    ASSERT_EQ(csoundCompileOrc(csound, second.c_str(), 1), 0);
    for (int block=0; block<10 && channel("second")==0; block++)
        ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(channel("first"), 11);
    EXPECT_EQ(channel("second"), 22);
}

TEST_F(ModuleSystemTests, RejectedNamedInstrumentDoesNotLeakIntoLaterEdits)
{
    initial();
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_NE(compile("instr RejectedVoice\nendin\nUndefinedOpcode\n"), 0);
    TREE *tree = csoundParseOrc(csound, "schedule RejectedVoice,0,.01\n");
    EXPECT_EQ(tree, nullptr);
    if (tree != nullptr) csoundDeleteTree(csound, tree);
}

TEST_F(ModuleSystemTests, DiscardingParsedImportRestoresPreviousBindings)
{
    write("first.orc", "giValue=11\nopcode ReadValue,i,0\n xout giValue\nendop\n");
    write("second.orc", "giValue=22\nopcode ReadValue,i,0\n xout giValue\nendop\n");
    initial("import " + path("first.orc") + " as shared\n");
    ASSERT_EQ(csoundStart(csound), 0);
    std::string edit = "import " + path("second.orc") + " as shared\n";
    TREE *tree = csoundParseOrc(csound, edit.c_str());
    ASSERT_NE(tree, nullptr);
    csoundDeleteTree(csound, tree);
    ASSERT_EQ(compile("iValue=ReadValue()\nchnset iValue,\"value\"\n"
                      "chnset shared.giValue,\"alias\"\n"), 0);
    EXPECT_EQ(channel("value"), 11);
    EXPECT_EQ(channel("alias"), 11);
}

TEST_F(ModuleSystemTests, StructAliasesPreserveModuleTypeIdentity)
{
    write("first.orc", "struct Point x:i,y:i\n"
          "opcode Make():Point\n p:Point init 11,12\n xout p\nendop\n");
    write("second.orc", "struct Point z:i\n"
          "opcode Make():Point\n p:Point init 22\n xout p\nendop\n");
    initial("from " + path("first.orc") + " import Point as First,Make as MakeFirst\n"
            "from " + path("second.orc") + " import Point as Second,Make as MakeSecond\n"
            "struct Wrapper point:First\n"
            "p:First=MakeFirst()\nq:Second=MakeSecond()\n"
            "w:Wrapper init p\nchnset w.point.y,\"first\"\n"
            "chnset q.z,\"second\"\n");
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_EQ(channel("first"), 12);
    EXPECT_EQ(channel("second"), 22);
}

TEST_F(ModuleSystemTests, SelectiveImportsDoNotExposePrivateStructTypes)
{
    write("library.orc", "struct Hidden x:i\ngiValue=1\n");
    initial("from " + path("library.orc") + " import giValue\n");
    TREE *tree = csoundParseOrc(csound, "value:Hidden init 1\n");
    EXPECT_EQ(tree, nullptr);
    if (tree != nullptr) csoundDeleteTree(csound, tree);
}

TEST_F(ModuleSystemTests, RejectedStructRedefinitionKeepsExistingLayout)
{
    initial("struct Point x:i,y:i\n");
    ASSERT_EQ(csoundStart(csound), 0);
    EXPECT_NE(compile("struct Point x:i,y:i,z:i\nUndefinedOpcode\n"), 0);
    ASSERT_EQ(compile("p:Point init 11,22\nchnset p.y,\"result\"\n"), 0);
    EXPECT_EQ(channel("result"), 22);
}

TEST_F(ModuleSystemTests, ParsedDefinitionsKeepTheirOwnNamespace)
{
    initial();
    ASSERT_EQ(csoundStart(csound), 0);
    TREE *first = csoundParseOrc(csound,
        "opcode ReadValue,i,0\n xout 11\nendop\n"
        "iValue=ReadValue()\nchnset iValue,\"first\"\n");
    ASSERT_NE(first, nullptr);
    TREE *second = csoundParseOrc(csound,
        "opcode ReadValue,i,0\n xout 22\nendop\n"
        "iValue=ReadValue()\nchnset iValue,\"second\"\n");
    ASSERT_NE(second, nullptr);
    ASSERT_EQ(csoundCompileTree(csound, second, 0), 0);
    csoundDeleteTree(csound, second);
    ASSERT_EQ(csoundCompileTree(csound, first, 0), 0);
    csoundDeleteTree(csound, first);
    EXPECT_EQ(channel("first"), 11);
    EXPECT_EQ(channel("second"), 22);
}

TEST_F(ModuleSystemTests, SynchronousEditDoesNotPublishQueuedUdos)
{
    initial("opcode ReadValue,i,0\n xout 11\nendop\n");
    ASSERT_EQ(csoundStart(csound), 0);
    ASSERT_EQ(csoundCompileOrc(csound,
        "opcode ReadValue,i,0\n xout 22\nendop\n", 1), 0);
    INSTRTXT *pending = csound->opcodeInfo->ip;
    ASSERT_NE(pending, nullptr);
    ASSERT_EQ(compile("chnset 1,\"unrelated\"\n"), 0);
    for (int number = 1; number <= csound->engineState.maxopcno; number++)
        EXPECT_NE(csound->engineState.instrtxtp[number], pending);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_EQ(compile("iValue=ReadValue()\nchnset iValue,\"result\"\n"), 0);
    EXPECT_EQ(channel("result"), 22);
}


}
