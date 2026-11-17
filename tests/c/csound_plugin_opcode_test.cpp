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
#include "csound_compiler.h"  // before csdl.h, which hides its host calls
#include "csdl.h"

#ifdef CSOUND_TEST_HAS_REQUESTED_OPCODE_PLUGIN
#include "create_error_plugin_path.hpp"
#include "requested_opcode_plugin_path.hpp"
#include "failing_init_plugin_path.hpp"
#include "failing_create_plugin_path.hpp"
#include "jumping_init_plugin_path.hpp"
#include "hook_plugin_path.hpp"
#endif

#ifdef CSOUND_TEST_HAS_TWO_COPIES
#include <unistd.h>
#include "two_copies.hpp"
#endif

#ifndef WIN32
#include <dlfcn.h>
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

namespace {
const char *kFailingInitOption =
    "--opcode-lib=" CSOUND_TEST_FAILING_INIT_PLUGIN;
} // namespace

TEST(ListOpcodes, SetOptionReportsPluginThatFailsToStart) {
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  csoundCreateMessageBuffer(csound, 0);
  ASSERT_EQ(csoundSetOption(csound, kFailingInitOption), CSOUND_SUCCESS);
  EXPECT_NE(csoundSetOption(csound, "--list-opcodes1"), CSOUND_SUCCESS);
  const std::string listing = drainMessages(csound);
  EXPECT_NE(listing.find("Error starting module"), std::string::npos)
      << listing;
  EXPECT_EQ(listing.find(" opcodes\n"), std::string::npos) << listing;
  csoundDestroy(csound);
}

TEST(ListOpcodes, CompileReportsPluginThatFailsToStart) {
  const OpcodeListing listing =
      compileWithArgs({"--list-opcodes1", kFailingInitOption});
  EXPECT_NE(listing.status, CSOUND_SUCCESS) << listing.text;
  EXPECT_NE(listing.status, CSOUND_EXITJMP_SUCCESS) << listing.text;
  EXPECT_EQ(listing.text.find(" opcodes\n"), std::string::npos)
      << listing.text;
}

// Started before any orchestra, Csound loads and starts the plugins in
// csoundStart, which jumped on a failure there before it owned exitjmp: into
// the frame of the csoundSetOption that had returned.
namespace {
void expectStartToFail(const std::string &option, const char *said) {
  csoundSetGlobalEnv("CSOUND7RC", "");  // no rc file of the user's
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  csoundSetGlobalEnv("CSOUND7RC", nullptr);
  csoundCreateMessageBuffer(csound, 0);
  csoundSetHostAudioIO(csound);
  EXPECT_EQ(csoundSetOption(csound, option.c_str()), CSOUND_SUCCESS);
  EXPECT_EQ(csoundStart(csound), CSOUND_ERROR);
  const std::string messages = drainMessages(csound);
  EXPECT_NE(messages.find(said), std::string::npos) << messages;
  EXPECT_EQ(csoundPerformKsmps(csound), CSOUND_ERROR);
  csoundDestroy(csound);
}
} // namespace

TEST(PluginThatFailsToStart, FailsCsoundStart) {
  expectStartToFail(kFailingInitOption, "Error starting module");
}

// In the plugin directory, the failure stops the directory's scan instead.
TEST(PluginThatFailsToStart, InThePluginDirectoryFailsCsoundStart) {
  const std::string directory =
      std::filesystem::path(CSOUND_TEST_FAILING_INIT_PLUGIN)
          .parent_path().string();
  expectStartToFail((sizeof(cs_float) == sizeof(float)
                         ? "--env:OPCODE7DIR=" : "--env:OPCODE7DIR64=") +
                        directory,
                    "Error starting module");
}

// A library that does not load stops Csound as it is loaded. A .wasm library
// that is not there does not load in any build, with Wasmtime or without.
TEST(PluginThatFailsToStart, ALibraryThatDoesNotLoadFailsCsoundStart) {
  expectStartToFail("--opcode-lib=no-such-library.wasm", "error loading");
}

namespace {
std::string directoryOf(const char *plugin) {
  return std::filesystem::path(plugin).parent_path().string();
}

// A path in an option for csoundSetOption(), which splits an option at the
// spaces outside quotes and drops the quotes: a build directory may have a
// space in its name.
std::string quoted(const std::string &path) {
  return "\"" + path + "\"";
}

std::string searchPathOption(const std::string &path) {
  return (sizeof(cs_float) == sizeof(float) ? "--env:OPCODE7DIR="
                                            : "--env:OPCODE7DIR64=") +
         quoted(path);
}

// Sets a variable of the process environment, which the loader reads with
// getenv(). MSVC takes no directive inside a test macro's argument, so the
// platform's part is done here.
void setProcessEnv(const char *name, const char *value) {
#ifdef WIN32
  _putenv_s(name, value);
#else
  setenv(name, value, 1);
#endif
}

void unsetProcessEnv(const char *name) {
#ifdef WIN32
  _putenv_s(name, "");  // an empty value removes it
#else
  unsetenv(name);
#endif
}

// The plugin directory's scan also takes CS_USER_PLUGINDIR, which it reads
// with getenv(), or the default user plugin directory when that is not set.
// For the life of a test, it names a directory that is not there, so only the
// fixtures' directories are scanned. Not empty: on Windows, _putenv_s()
// removes a variable given an empty value.
class NoUserPluginDirectory {
public:
  NoUserPluginDirectory() {
    const char *value = getenv(kName);
    saved_ = value != nullptr;
    if (saved_)
      value_ = value;
    setProcessEnv(kName, "no-such-user-plugin-directory");
  }
  ~NoUserPluginDirectory() {
    if (saved_)
      setProcessEnv(kName, value_.c_str());
    else
      unsetProcessEnv(kName);
  }
  NoUserPluginDirectory(const NoUserPluginDirectory &) = delete;
  NoUserPluginDirectory &operator=(const NoUserPluginDirectory &) = delete;

private:
  static constexpr const char *kName = "CS_USER_PLUGINDIR";
  bool saved_;
  std::string value_;
};

// An instance that reads no rc file of the user's, with a message buffer.
CSOUND *createWithoutRcFile() {
  csoundSetGlobalEnv("CSOUND7RC", "");
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  csoundSetGlobalEnv("CSOUND7RC", nullptr);
  csoundCreateMessageBuffer(csound, 0);
  return csound;
}
} // namespace

// A plugin loaded again links no new module, so nothing is started. Its
// directory loaded a second time started the newest module instead, here one
// that fails to start.
TEST(PluginLoadedAgain, StartsNoOtherPlugin) {
  const NoUserPluginDirectory noUserPlugins;
  const std::string requested =
      directoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN);
  CSOUND *csound = createWithoutRcFile();
  ASSERT_EQ(csoundSetOption(csound, searchPathOption(requested).c_str()),
            CSOUND_SUCCESS);
  csoundLoadPlugins(csound,
                    directoryOf(CSOUND_TEST_FAILING_INIT_PLUGIN).c_str());
  drainMessages(csound);
  EXPECT_EQ(csoundLoadPlugins(csound, requested.c_str()), CSOUND_SUCCESS);
  const std::string messages = drainMessages(csound);
  EXPECT_EQ(messages.find("Error starting module"), std::string::npos)
      << messages;
  csoundDestroy(csound);
}

// The directory's scan took a plugin that fails to start for a file that is
// no plugin, and csoundLoadPlugins() returned CSOUND_SUCCESS.
TEST(PluginThatFailsToStart, FailsCsoundLoadPlugins) {
  const NoUserPluginDirectory noUserPlugins;
  CSOUND *csound = createWithoutRcFile();
  ASSERT_EQ(csoundSetOption(csound,
                            searchPathOption(directoryOf(
                                CSOUND_TEST_REQUESTED_OPCODE_PLUGIN)).c_str()),
            CSOUND_SUCCESS);
  EXPECT_EQ(csoundLoadPlugins(
                csound, directoryOf(CSOUND_TEST_FAILING_INIT_PLUGIN).c_str()),
            CSOUND_ERROR);
  const std::string messages = drainMessages(csound);
  EXPECT_NE(messages.find("Error starting module"), std::string::npos)
      << messages;
  csoundDestroy(csound);
}

// A plugin whose csoundModuleCreate() fails was still linked, so Csound
// started it and destroyed it. The fixture aborts if either happens, so each
// route runs in a process of its own.
namespace {
void expectNeitherStartedNorDestroyed(
    const std::vector<std::string> &options) {
  const NoUserPluginDirectory noUserPlugins;
  EXPECT_EXIT(
      {
        csoundSetGlobalEnv("CSOUND7RC", "");  // no rc file of the user's
        CSOUND *csound = csoundCreate(nullptr, nullptr);
        csoundSetGlobalEnv("CSOUND7RC", nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetHostAudioIO(csound);
        for (const std::string &option : options)
          csoundSetOption(csound, option.c_str());
        csoundStart(csound);
        csoundDestroy(csound);
        std::exit(0);
      },
      ::testing::ExitedWithCode(0), "");
}
} // namespace

TEST(PluginThatFailsToLoad, InThePluginDirectoryIsNeitherStartedNorDestroyed) {
  expectNeitherStartedNorDestroyed(
      {searchPathOption(directoryOf(CSOUND_TEST_FAILING_CREATE_PLUGIN))});
}

TEST(PluginThatFailsToLoad, ThroughOpcodeLibIsNeitherStartedNorDestroyed) {
  expectNeitherStartedNorDestroyed(
      {searchPathOption(directoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN)),
       "--opcode-lib=" + quoted(CSOUND_TEST_FAILING_CREATE_PLUGIN)});
}

TEST(PluginThatFailsToLoad, ThroughOpcodeDirIsNeitherStartedNorDestroyed) {
  expectNeitherStartedNorDestroyed(
      {searchPathOption(directoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN)),
       "--opcode-dir=" +
           quoted(directoryOf(CSOUND_TEST_FAILING_CREATE_PLUGIN))});
}

// What a module whose csoundModuleCreate() failed left behind may call into
// its library, here the fixture's keyboard callback, which a reset keeps. The
// library was closed at the reset, or, once a failed module was no longer
// linked, never. It now stays open until csoundDestroy() has dropped the
// callbacks, and closes then. The fixture's csoundModuleInit() and
// csoundModuleDestroy() abort if called.
namespace {
// Whether the library is loaded, as dlopen() says; not checked on Windows.
bool loadedIs(const char *library, bool loaded) {
#ifdef WIN32
  (void) library;
  (void) loaded;
  return true;
#else
  void *handle = dlopen(library, RTLD_NOW | RTLD_NOLOAD);
  if (handle != nullptr)
    dlclose(handle);
  return (handle != nullptr) == loaded;
#endif
}

// sensekey reads stdin too, and makes a terminal non-canonical.
bool keepStdinOffTheTerminal() {
#ifdef WIN32
  return true;
#else
  return freopen("/dev/null", "r", stdin) != nullptr;
#endif
}

const char *kSenseKeyInstrument =
    "instr 1\n"
    "  kKey sensekey\n"
    "endin\n"
    "schedule 1, 0, 1\n";
} // namespace

TEST(PluginThatFailsToLoad, KeepsItsLibraryUntilCsoundDestroy) {
  const NoUserPluginDirectory noUserPlugins;
  const std::string searchPath = searchPathOption("no-such-plugin-directory");
  const std::string directory =
      directoryOf(CSOUND_TEST_FAILING_CREATE_PLUGIN);
  EXPECT_EXIT(
      {
        if (!loadedIs(CSOUND_TEST_FAILING_CREATE_PLUGIN, false))
          std::exit(2);
        if (!keepStdinOffTheTerminal())
          std::exit(8);
        csoundSetGlobalEnv("CSOUND7RC", "");  // across the reset too
        CSOUND *csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, searchPath.c_str());
        if (csoundLoadPlugins(csound, directory.c_str()) != CSOUND_ERROR)
          std::exit(3);
        csoundReset(csound);
        if (!loadedIs(CSOUND_TEST_FAILING_CREATE_PLUGIN, true))
          std::exit(4);
        csoundSetOption(csound, searchPath.c_str());
        csoundSetOption(csound, "-n");
        if (csoundCompileOrc(csound, kSenseKeyInstrument, 0) != CSOUND_SUCCESS ||
            csoundStart(csound) != CSOUND_SUCCESS)
          std::exit(5);
        csoundPerformKsmps(csound);
        if (csoundGetControlChannel(csound, "failing_create_calls",
                                    nullptr) < 1)
          std::exit(6);
        csoundDestroy(csound);
        if (!loadedIs(CSOUND_TEST_FAILING_CREATE_PLUGIN, false))
          std::exit(7);
        std::exit(0);
      },
      ::testing::ExitedWithCode(0), "");
}

// --opcode-dir went on past a plugin that does not load or does not start,
// where --opcode-lib and the plugin directory stop. A process of its own, as
// the failing-create fixture aborts if it is destroyed.
namespace {
void expectOpcodeDirToFail(const char *plugin) {
  const NoUserPluginDirectory noUserPlugins;
  const std::string searchPath =
      searchPathOption(directoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN));
  const std::string option = "--opcode-dir=" + quoted(directoryOf(plugin));
  EXPECT_EXIT(
      {
        CSOUND *csound = createWithoutRcFile();
        if (csoundSetOption(csound, searchPath.c_str()) != CSOUND_SUCCESS)
          std::exit(2);
        const bool refused =
            csoundSetOption(csound, option.c_str()) != CSOUND_SUCCESS;
        const std::string messages = drainMessages(csound);
        csoundDestroy(csound);
        if (!refused ||
            messages.find("a plugin failed to load or start") ==
                std::string::npos) {
          fputs(messages.c_str(), stderr);
          std::exit(1);
        }
        std::exit(0);
      },
      ::testing::ExitedWithCode(0), "");
}
} // namespace

TEST(PluginThatFailsToLoad, StopsOpcodeDir) {
  expectOpcodeDirToFail(CSOUND_TEST_FAILING_CREATE_PLUGIN);
}

TEST(PluginThatFailsToStart, StopsOpcodeDir) {
  expectOpcodeDirToFail(CSOUND_TEST_FAILING_INIT_PLUGIN);
}

// Called first, csoundLoadPlugins() scans the plugin directory, which jumps
// when a plugin there fails: with no frame for it, the host crashed.
namespace {
void expectLoadPluginsFirstToFail(const char *plugin) {
  const NoUserPluginDirectory noUserPlugins;
  const char *variable =
      sizeof(cs_float) == sizeof(float) ? "OPCODE7DIR" : "OPCODE7DIR64";
  const std::string directory = directoryOf(plugin);
  EXPECT_EXIT(
      {
        csoundSetGlobalEnv(variable, directory.c_str());
        CSOUND *csound = csoundCreate(nullptr, nullptr);
        csoundSetGlobalEnv(variable, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        const int32_t loaded = csoundLoadPlugins(
            csound, directoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN).c_str());
        csoundDestroy(csound);
        std::exit(loaded == CSOUND_ERROR ? 0 : 1);
      },
      ::testing::ExitedWithCode(0), "");
}
} // namespace

TEST(PluginThatFailsToLoad, InThePluginDirectoryFailsLoadPluginsCalledFirst) {
  expectLoadPluginsFirstToFail(CSOUND_TEST_FAILING_CREATE_PLUGIN);
}

TEST(PluginThatFailsToStart, InThePluginDirectoryFailsLoadPluginsCalledFirst) {
  expectLoadPluginsFirstToFail(CSOUND_TEST_FAILING_INIT_PLUGIN);
}

// A module of the plugin directory whose csoundModuleInit() jumped, as one
// that calls csound->Die() does, left the scan marked as loaded, so a second
// csoundLoadPlugins() succeeded on modules that had not all started. It now
// fails as the first, as when csoundModuleInit() returns an error.
TEST(PluginThatFailsToStart, ByAJumpFailsEveryLaterLoad) {
  const NoUserPluginDirectory noUserPlugins;
  const char *variable =
      sizeof(cs_float) == sizeof(float) ? "OPCODE7DIR" : "OPCODE7DIR64";
  const std::string directory = directoryOf(CSOUND_TEST_JUMPING_INIT_PLUGIN);
  const std::string requested =
      directoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN);
  EXPECT_EXIT(
      {
        csoundSetGlobalEnv(variable, directory.c_str());
        CSOUND *csound = createWithoutRcFile();
        csoundSetGlobalEnv(variable, nullptr);
        if (csoundLoadPlugins(csound, requested.c_str()) != CSOUND_ERROR)
          std::exit(2);
        if (csoundLoadPlugins(csound, requested.c_str()) != CSOUND_ERROR)
          std::exit(3);
        csoundDestroy(csound);
        std::exit(0);
      },
      ::testing::ExitedWithCode(0), "");
}

// The scans counted the rest of the search path in the length of a plugin's
// path, so a long search path skipped the plugins of its first directories
// as "path name too long".
namespace {
std::string followedByOtherDirectories(const std::string &directory,
                                       int count = 48) {
#ifdef WIN32
  const char separator = ';';
#else
  const char separator = ':';
#endif
  std::string path = directory;
  for (int i = 0; i < count; i++)
    path += separator + std::string("no-such-plugin-directory-") +
            std::to_string(i);
  return path;
}

const char *kRequestedInstrument =
    "instr 1\n"
    "  iValue requested_opcode_fixture 42\n"
    "endin\n";
} // namespace

TEST(LongPluginSearchPath, FindsThePluginsOfItsFirstDirectory) {
  const NoUserPluginDirectory noUserPlugins;
  CSOUND *csound = createWithoutRcFile();
  ASSERT_EQ(csoundSetOption(csound,
                            searchPathOption(followedByOtherDirectories(
                                directoryOf(
                                    CSOUND_TEST_REQUESTED_OPCODE_PLUGIN)))
                                .c_str()),
            CSOUND_SUCCESS);
  EXPECT_EQ(csoundCompileOrc(csound, kRequestedInstrument, 0), CSOUND_SUCCESS)
      << drainMessages(csound);
  csoundDestroy(csound);
}

TEST(LongPluginSearchPath, ThroughLoadPluginsFindsThePluginsOfItsFirstDirectory) {
  const NoUserPluginDirectory noUserPlugins;
  CSOUND *csound = createWithoutRcFile();
  ASSERT_EQ(csoundSetOption(
                csound, searchPathOption("no-such-plugin-directory").c_str()),
            CSOUND_SUCCESS);
  EXPECT_EQ(csoundLoadPlugins(csound,
                              followedByOtherDirectories(directoryOf(
                                  CSOUND_TEST_REQUESTED_OPCODE_PLUGIN))
                                  .c_str()),
            CSOUND_SUCCESS);
  EXPECT_EQ(csoundCompileOrc(csound, kRequestedInstrument, 0), CSOUND_SUCCESS)
      << drainMessages(csound);
  csoundDestroy(csound);
}

// With a user plugin directory, the scan joined the search path in 2048
// bytes, and a longer path lost its end: the user directory's plugins with it.
TEST(LongPluginSearchPath, KeepsTheUserPluginDirectory) {
  const std::string user = directoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN);
  const std::string option = searchPathOption(
      followedByOtherDirectories("no-such-plugin-directory", 96));
  EXPECT_EXIT(
      {
        setProcessEnv("CS_USER_PLUGINDIR", user.c_str());
        CSOUND *csound = createWithoutRcFile();
        if (csoundSetOption(csound, option.c_str()) != CSOUND_SUCCESS)
          std::exit(2);
        const int32_t compiled =
            csoundCompileOrc(csound, kRequestedInstrument, 0);
        csoundDestroy(csound);
        std::exit(compiled == CSOUND_SUCCESS ? 0 : 1);
      },
      ::testing::ExitedWithCode(0), "");
}

#ifdef CSOUND_TEST_HAS_TWO_COPIES
// One plugin in two directories, as a build tree beside an installed Csound
// has it. Csound loads both, and they share the instance's variables. Each
// run is a process of its own, because a double free ends it.
class TwoCopiesOfAPlugin : public ::testing::TestWithParam<const char *> {
protected:
  std::string directory(const char *copy) const {
    return std::string(CSOUND_TEST_TWO_COPIES_DIR "/") + GetParam() + "/" + copy;
  }

  void expectCleanRun(const std::vector<std::string> &options) {
    EXPECT_EXIT(
        {
          CSOUND *csound = csoundCreate(nullptr, nullptr);
          csoundCreateMessageBuffer(csound, 0);
          csoundSetOption(csound, "-n");
          for (const std::string &option : options)
            csoundSetOption(csound, option.c_str());
          const int32_t status =
              csoundCompileOrc(csound, "instr 1\nendin\n", 0);
          const std::string messages = drainMessages(csound);
          csoundDestroy(csound);
          if (status != CSOUND_SUCCESS ||
              messages.find("pre-initialisation") != std::string::npos) {
            fputs(messages.c_str(), stderr);
            _exit(1);
          }
          _exit(0);
        },
        ::testing::ExitedWithCode(0), "");
  }

  static std::string searchPath(const std::string &path) {
    return (sizeof(cs_float) == sizeof(float)
        ? "--env:OPCODE7DIR=" : "--env:OPCODE7DIR64=") + path;
  }
};

TEST_P(TwoCopiesOfAPlugin, OnTheSearchPath) {
  expectCleanRun({searchPath(directory("first") + ":" + directory("second"))});
}

TEST_P(TwoCopiesOfAPlugin, ThroughOpcodeDir) {
  expectCleanRun({searchPath(directory("first")),
                  "--opcode-dir=" + directory("second")});
}

TEST_P(TwoCopiesOfAPlugin, ThroughOpcodeLib) {
  expectCleanRun({searchPath(directory("first")),
                  "--opcode-lib=" + directory("second") + "/lib" + GetParam() +
                      ".so"});
}

INSTANTIATE_TEST_SUITE_P(Modules, TwoCopiesOfAPlugin,
                         ::testing::Values(CSOUND_TEST_TWO_COPY_PLUGINS));
#endif

// A host lists or chooses modules first, with csoundGetModule(),
// csoundSetRTAudioModule() or csoundSetMIDIModule(), or parses with
// csoundParseOrc(), and the call scans the plugin directory, which jumps when
// a plugin there fails to load or start: with nothing owning exitjmp, the
// host died with signal 11. Each test runs in a process of its own.
namespace {
// The scan reads CS_USER_PLUGINDIR with getenv(), and takes the default user
// plugin directory when it is not set: one that is not there keeps the
// machine's plugins out. MSVC takes no directive inside a test macro's
// argument, so this is a function.
void scanNoUserPluginDirectory() {
#ifdef WIN32
  _putenv_s("CS_USER_PLUGINDIR", "no-such-user-plugin-directory");
#else
  setenv("CS_USER_PLUGINDIR", "no-such-user-plugin-directory", 1);
#endif
}

// In a test's own process: an instance whose plugin directory is the one the
// plugin is in, that reads no rc file of the user's, and to which no other
// call has been made.
CSOUND *createScanningTheDirectoryOf(const char *plugin) {
  const std::string directory =
      std::filesystem::path(plugin).parent_path().string();
  scanNoUserPluginDirectory();
  csoundSetGlobalEnv(sizeof(cs_float) == sizeof(float) ? "OPCODE7DIR"
                                                       : "OPCODE7DIR64",
                     directory.c_str());
  csoundSetGlobalEnv("CSOUND7RC", "");
  CSOUND *csound = csoundCreate(nullptr, nullptr);
  csoundCreateMessageBuffer(csound, 0);
  return csound;
}

// Each makes the call, and returns CSOUND_ERROR if it failed as it should.
// The setters return nothing, and csoundStart() must fail after them.
int32_t getModule(CSOUND *csound) {
  char *name, *type;
  return csoundGetModule(csound, 0, &name, &type);
}

int32_t start(CSOUND *csound) {
  csoundSetOption(csound, "-n");
  return csoundStart(csound);
}

int32_t setRTAudioModule(CSOUND *csound) {
  csoundSetRTAudioModule(csound, "no-such-module");
  return start(csound);
}

int32_t setMIDIModule(CSOUND *csound) {
  csoundSetMIDIModule(csound, "no-such-module");
  return start(csound);
}

int32_t parseOrc(CSOUND *csound) {
  TREE *tree = csoundParseOrc(csound, "instr 1\nendin\n");
  if (tree == nullptr)
    return CSOUND_ERROR;
  csoundDeleteTree(csound, tree);
  return CSOUND_SUCCESS;
}

// csoundCompileOrc() parses through csoundParseOrc(), and took the jump
// already, but returned its raw value, 255.
int32_t compileOrc(CSOUND *csound) {
  return csoundCompileOrc(csound, "instr 1\nendin\n", 0);
}

void expectCalledFirstToFail(const char *plugin, int32_t (*call)(CSOUND *)) {
  EXPECT_EXIT(
      {
        CSOUND *csound = createScanningTheDirectoryOf(plugin);
        const int32_t result = call(csound);
        csoundDestroy(csound);
        std::exit(result == CSOUND_ERROR ? 0 : 1);
      },
      ::testing::ExitedWithCode(0), "");
}
} // namespace

TEST(PluginThatFailsToLoad, InThePluginDirectoryFailsGetModuleCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_CREATE_ERROR_PLUGIN, getModule);
}

TEST(PluginThatFailsToStart, InThePluginDirectoryFailsGetModuleCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_FAILING_INIT_PLUGIN, getModule);
}

TEST(PluginThatFailsToLoad,
     InThePluginDirectoryFailsTheStartAfterSetRTAudioModuleCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_CREATE_ERROR_PLUGIN, setRTAudioModule);
}

TEST(PluginThatFailsToStart,
     InThePluginDirectoryFailsTheStartAfterSetRTAudioModuleCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_FAILING_INIT_PLUGIN, setRTAudioModule);
}

TEST(PluginThatFailsToLoad,
     InThePluginDirectoryFailsTheStartAfterSetMIDIModuleCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_CREATE_ERROR_PLUGIN, setMIDIModule);
}

TEST(PluginThatFailsToStart,
     InThePluginDirectoryFailsTheStartAfterSetMIDIModuleCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_FAILING_INIT_PLUGIN, setMIDIModule);
}

TEST(PluginThatFailsToLoad, InThePluginDirectoryFailsParseOrcCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_CREATE_ERROR_PLUGIN, parseOrc);
}

TEST(PluginThatFailsToStart, InThePluginDirectoryFailsParseOrcCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_FAILING_INIT_PLUGIN, parseOrc);
}

TEST(PluginThatFailsToLoad, InThePluginDirectoryFailsCompileOrcCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_CREATE_ERROR_PLUGIN, compileOrc);
}

TEST(PluginThatFailsToStart, InThePluginDirectoryFailsCompileOrcCalledFirst) {
  expectCalledFirstToFail(CSOUND_TEST_FAILING_INIT_PLUGIN, compileOrc);
}

// Once the plugins are loaded, the setters start them again, and a plugin
// that fails to start, here one loaded after the scan, jumped there too.
namespace {
void expectCalledAfterTheScanToFail(int32_t (*call)(CSOUND *)) {
  const std::string failing =
      std::filesystem::path(CSOUND_TEST_FAILING_INIT_PLUGIN)
          .parent_path().string();
  EXPECT_EXIT(
      {
        CSOUND *csound =
            createScanningTheDirectoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN);
        csoundLoadPlugins(csound, failing.c_str());
        const int32_t result = call(csound);
        csoundDestroy(csound);
        std::exit(result == CSOUND_ERROR ? 0 : 1);
      },
      ::testing::ExitedWithCode(0), "");
}
} // namespace

TEST(PluginThatFailsToStart, LoadedAfterTheScanFailsTheStartAfterSetRTAudioModule) {
  expectCalledAfterTheScanToFail(setRTAudioModule);
}

TEST(PluginThatFailsToStart, LoadedAfterTheScanFailsTheStartAfterSetMIDIModule) {
  expectCalledAfterTheScanToFail(setMIDIModule);
}

// A compile can start the plugins before csoundStart(), here through an
// --opcode-lib in <CsOptions>, and csoundStart() then does not start them
// again: a plugin that failed to start as a setter started them again went
// unreported, as the setters return nothing.
namespace {
void expectCalledAfterACompileToFail(int32_t (*call)(CSOUND *)) {
  const std::string failing =
      std::filesystem::path(CSOUND_TEST_FAILING_INIT_PLUGIN)
          .parent_path().string();
  const std::string csd =
      "<CsoundSynthesizer>\n<CsOptions>\n-n --opcode-lib=\"" +
      std::string(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN) +
      "\"\n</CsOptions>\n<CsInstruments>\ninstr 1\nendin\n"
      "</CsInstruments>\n</CsoundSynthesizer>\n";
  EXPECT_EXIT(
      {
        CSOUND *csound =
            createScanningTheDirectoryOf(CSOUND_TEST_REQUESTED_OPCODE_PLUGIN);
        if (csoundCompileCSD(csound, csd.c_str(), 1, 0) != CSOUND_SUCCESS)
          std::exit(2);
        csoundLoadPlugins(csound, failing.c_str());
        const int32_t result = call(csound);
        csoundDestroy(csound);
        std::exit(result == CSOUND_ERROR ? 0 : 1);
      },
      ::testing::ExitedWithCode(0), "");
}
} // namespace

TEST(PluginThatFailsToStart, LoadedAfterACompileFailsTheStartAfterSetRTAudioModule) {
  expectCalledAfterACompileToFail(setRTAudioModule);
}

TEST(PluginThatFailsToStart, LoadedAfterACompileFailsTheStartAfterSetMIDIModule) {
  expectCalledAfterACompileToFail(setMIDIModule);
}

// A plugin whose csoundModuleCreate() or csoundModuleInit() evaluates code
// before the start, with csoundEvalCode(), and the code jumps (exitnow): the
// plugin stops there and its load fails, as when it jumps itself. The jump
// went to an init pass's landing inside the call that loaded the plugin,
// the plugin went on, and the load succeeded. Each runs in a process of its
// own.
namespace {
bool evaluatedOn = false;  // the plugin went on after the code it evaluated

void evaluateAJump(CSOUND *csound) {
  csoundEvalCode(csound, "exitnow 3\n");
  evaluatedOn = true;
}

void hookPlugin(CSOUND *csound, const char *where,
                void (*hook)(CSOUND *) = evaluateAJump) {
  typedef void (*Hook)(CSOUND *);
  csoundCreateGlobalVariable(csound, where, sizeof(Hook));
  *static_cast<Hook *>(csoundQueryGlobalVariable(csound, where)) = hook;
}

// Exits 0 if the call fails and the plugin stops at the jump.
void expectAnEvaluatedJumpToFail(const char *where, bool scanned,
                                 int32_t (*call)(CSOUND *)) {
  EXPECT_EXIT(
      {
        CSOUND *csound = scanned
                             ? createScanningTheDirectoryOf(CSOUND_TEST_HOOK_PLUGIN)
                             : createWithoutRcFile();
        hookPlugin(csound, where);
        const int32_t result = call(csound);
        csoundDestroy(csound);
        std::exit(result == CSOUND_SUCCESS ? 1 : evaluatedOn ? 2 : 0);
      },
      ::testing::ExitedWithCode(0), "");
}

int32_t loadHookPlugin(CSOUND *csound) {
  return csoundLoadPlugins(csound,
                           directoryOf(CSOUND_TEST_HOOK_PLUGIN).c_str());
}
} // namespace

TEST(PluginThatEvaluatesAJump, InItsCreateFailsCsoundLoadPlugins) {
  const NoUserPluginDirectory noUserPlugins;
  expectAnEvaluatedJumpToFail("hook_plugin_create", false, loadHookPlugin);
}

TEST(PluginThatEvaluatesAJump, InItsInitFailsCsoundLoadPlugins) {
  const NoUserPluginDirectory noUserPlugins;
  expectAnEvaluatedJumpToFail("hook_plugin_init", false, loadHookPlugin);
}

TEST(PluginThatEvaluatesAJump, InItsInitInThePluginDirectoryFailsParseOrc) {
  expectAnEvaluatedJumpToFail("hook_plugin_init", true, parseOrc);
}

// Loaded by a compile, through --opcode-lib, the jump is the compile's, as a
// jump in its global code is: the plugin goes on, and the compile gives the
// jump. It gave 0.
TEST(PluginThatEvaluatesAJump, InItsInitLoadedByACompileIsTheCompiles) {
  const NoUserPluginDirectory noUserPlugins;
  const std::string option =
      std::string("--opcode-lib=") + CSOUND_TEST_HOOK_PLUGIN;
  EXPECT_EXIT(
      {
        CSOUND *csound = createWithoutRcFile();
        hookPlugin(csound, "hook_plugin_init");
        csoundSetOption(csound, option.c_str());
        const int32_t result = compileOrc(csound);
        csoundDestroy(csound);
        std::exit(result == CSOUND_EXITJMP_SUCCESS - 3 && evaluatedOn ? 0 : 1);
      },
      ::testing::ExitedWithCode(0), "");
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
