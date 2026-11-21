#define __BUILDING_LIBCSOUND

#include <stdio.h>
#include <string.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "csound.h"
#include "csoundCore.h"

extern "C" {
    extern CORFIL *corfile_create_r(CSOUND *csound, const char *text);
    extern void corfile_rm(CSOUND *csound, CORFIL **ff);
}

/* Engine/memalloc.c's block header: a Debug build defines BETA, which turns
   MEMDEBUG on. */
struct TrackedBlock {
#if defined(BETA) || defined(MEMDEBUG)
    int32_t magic;
    void *ptr;
#endif
    TrackedBlock *prv, *nxt;
};

/* The blocks Csound holds until a reset. */
static size_t trackedBlocks (const CSOUND *csound)
{
    size_t n = 0;
    for (auto *b = static_cast<TrackedBlock *> (csound->memalloc_db); b;
         b = b->nxt)
        n++;
    return n;
}

class OptionParserTests : public ::testing::Test {
public:
    OptionParserTests ()
    {
    }

    virtual ~OptionParserTests ()
    {
    }

    virtual void SetUp ()
    {
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (NULL, NULL);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (OptionParserTests, testParseSimpleEnvVar)
{
    const char option_str[] = "--env:FOO1=bar1 --env:FOO2=bar2\n";
    const char *value;
    CORFIL *cf = corfile_create_r(csound, option_str);
    read_options(csound, cf, 0);

    value = csoundGetEnv(csound, "FOO1");
    ASSERT_TRUE(value != NULL);
    ASSERT_TRUE(strcmp(value, "bar1") == 0);

    value = csoundGetEnv(csound, "FOO2");
    ASSERT_TRUE(value != NULL);
    ASSERT_TRUE(strcmp(value, "bar2") == 0);

    corfile_rm(csound, &cf);
}

TEST_F (OptionParserTests, testParseQuotedEnvVar)
{
    const char option_str[] = "--env:FOO1=\"bar baz\"\n";
    const char *value;
    CORFIL *cf = corfile_create_r(csound, option_str);
    read_options(csound, cf, 0);

    value = csoundGetEnv(csound, "FOO1");
    ASSERT_TRUE(value != NULL);
    ASSERT_TRUE(strcmp(value, "bar baz") == 0);

    corfile_rm(csound, &cf);
}

TEST_F (OptionParserTests, testEmptyOptionSetsNothing)
{
    /* Three calls, as argdecode() takes two names and dies of the third. */
    EXPECT_EQ (csoundSetOption (csound, ""), 0);
    EXPECT_EQ (csoundSetOption (csound, "   "), 0);
    EXPECT_EQ (csoundSetOption (csound, ""), 0);
    EXPECT_EQ (csound->orchname, nullptr);
    EXPECT_EQ (csound->scorename, nullptr);
}

TEST_F (OptionParserTests, testEmptyOptionLeavesRcFilesUnread)
{
    /* The fixture's own option has read them, so a new instance. */
    CSOUND *fresh = csoundCreate (NULL, NULL);
    csoundCreateMessageBuffer (fresh, 0);
    EXPECT_EQ (csoundSetOption (fresh, ""), 0);
    EXPECT_EQ (fresh->options_checked, 0);
    csoundDestroy (fresh);
}

TEST_F (OptionParserTests, testQuotedFirstArgumentKeepsItsText)
{
    EXPECT_EQ (csoundSetOption (csound, "\"my piece.csd\""), 0);
    EXPECT_STREQ (csound->orchname, "my piece.csd");
}

TEST_F (OptionParserTests, testQuotedFlagIsAFlag)
{
    EXPECT_EQ (csoundSetOption (csound, "\"-n\" -d"), 0);
    EXPECT_EQ (csound->oparms->sfwrite, 0);
    EXPECT_EQ (csound->oparms->displays, 0);
    EXPECT_EQ (csound->orchname, nullptr);
}

TEST_F (OptionParserTests, testQuotedArgumentAfterASpaceIsOneArgument)
{
    EXPECT_EQ (csoundSetOption (csound, "-o \"a b.wav\""), 0);
    EXPECT_STREQ (csound->oparms->outfilename, "a b.wav");
    EXPECT_EQ (csound->orchname, nullptr);
}

TEST_F (OptionParserTests, testOptionAfterAQuotedArgumentIsItsOwn)
{
    EXPECT_EQ (csoundSetOption (csound, "-o \"a.wav\" -d"), 0);
    EXPECT_STREQ (csound->oparms->outfilename, "a.wav");
    EXPECT_EQ (csound->oparms->displays, 0);
}

TEST_F (OptionParserTests, testUnquotedAndAttachedFormsSplitAsBefore)
{
    EXPECT_EQ (csoundSetOption (csound, "  -o\"a b.wav\"   -d  "), 0);
    EXPECT_STREQ (csound->oparms->outfilename, "a b.wav");
    EXPECT_EQ (csound->oparms->displays, 0);
    EXPECT_EQ (csoundSetOption (csound, "--env:FOO3=\"bar baz\" orc sco"), 0);
    EXPECT_STREQ (csoundGetEnv (csound, "FOO3"), "bar baz");
    EXPECT_STREQ (csound->orchname, "orc");
    EXPECT_STREQ (csound->scorename, "sco");
}

TEST_F (OptionParserTests, testNullOptionIsRefused)
{
    EXPECT_NE (csoundSetOption (csound, NULL), 0);
}

TEST_F (OptionParserTests, testClusterEndingInADashOrPlusStaysInItsArgument)
{
    /* Each is the last argument, so a read past its end leaves the copy
       argdecode() makes, which AddressSanitizer reports. */
    EXPECT_EQ (csoundSetOption (csound, "-d-"), 0);
    EXPECT_EQ (csound->oparms->displays, 0);
    EXPECT_EQ (csoundSetOption (csound, "-n+"), 0);
    EXPECT_EQ (csound->oparms->sfwrite, 0);
}

TEST_F (OptionParserTests, testOptionKeepsOnlyTheDecodedArguments)
{
    /* One block: argdecode()'s copy of the arguments, which the options it
       sets point into. */
    for (const char *option : {"-d", "-d -m0 -n"}) {
        const size_t before = trackedBlocks (csound);
        EXPECT_EQ (csoundSetOption (csound, option), 0);
        EXPECT_EQ (trackedBlocks (csound) - before, 1u) << option;
    }
}

TEST_F (OptionParserTests, testRefusedOptionKeepsOnlyTheDecodedArguments)
{
    /* argdecode() refuses -% and leaves by a long jump. */
    const size_t before = trackedBlocks (csound);
    EXPECT_NE (csoundSetOption (csound, "-d -m0 -%"), 0);
    EXPECT_EQ (trackedBlocks (csound) - before, 1u);
}

TEST_F (OptionParserTests, testJumpAfterTheDecodeFreesNothingTwice)
{
    /* -z lists the opcodes once the options are decoded, and a .wasm library
       that does not load stops that with csoundDie(), a long jump. */
    EXPECT_NE (csoundSetOption (csound, "--opcode-lib=missing.wasm -z"), 0);
    std::string messages;
    for (; csoundGetMessageCnt (csound) > 0; csoundPopFirstMessage (csound))
        messages += csoundGetFirstMessage (csound);
    EXPECT_NE (messages.find ("error loading 'missing.wasm'"),
               std::string::npos);
}

TEST_F (OptionParserTests, testIndirectionFileOptionsAreRead)
{
    /* The file names a second one, which is read too: not a cycle. */
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path () /
        ("csound-options-" + std::to_string (std::random_device {} ()));
    const fs::path second = file.string () + "-second";
    std::ofstream (file) << "--env:FROM_AT_FILE=1 -@ \"" << second.string ()
                         << "\"\n";
    std::ofstream (second) << "--env:FROM_AT_SECOND=1\n";
    const std::string option = "-@ \"" + file.string () + "\"";
    EXPECT_EQ (csoundSetOption (csound, option.c_str ()), 0);
    fs::remove (second);
    fs::remove (file);
    EXPECT_STREQ (csoundGetEnv (csound, "FROM_AT_FILE"), "1");
    EXPECT_STREQ (csoundGetEnv (csound, "FROM_AT_SECOND"), "1");
}

TEST_F (OptionParserTests, testIndirectionFileThatNamesItselfIsRefused)
{
    /* Refused under any name: as given, through . or .., in capitals on
       Windows, a hard link and a symbolic one. Each read appends to
       FROM_AT_CHAIN, and the file is read once under any of them, but for
       a link on Windows, which compares resolved names there. */
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path () /
        ("csound-options-" + std::to_string (std::random_device {} ()));
    const fs::path dir = file.parent_path ();
    const fs::path hard = file.string () + "-hard";
    const fs::path link = file.string () + "-link";
    std::ofstream (file) << "\n";
#ifdef _WIN32
    const fs::path real = dir;
#else
    const fs::path real = fs::canonical (dir); /* .. as the system takes it */
#endif
    std::vector<fs::path> names = {
        file, dir / "." / file.filename (),
        real / ".." / real.filename () / file.filename ()};
#ifdef _WIN32
    std::string capitals = file.filename ().string ();
    for (char &c : capitals)
        c = (char) toupper ((unsigned char) c);
    names.push_back (dir / capitals);
#endif
    std::error_code error; /* a file system without links skips them */
    fs::create_hard_link (file, hard, error);
    if (!error)
        names.push_back (hard);
#ifndef _WIN32
    fs::create_symlink (file, link, error);
    if (!error)
        names.push_back (link);
#endif
    const std::string option = "-@ \"" + file.string () + "\"";
    for (const fs::path &name : names) {
        std::ofstream (file) << "--env:FROM_AT_CHAIN+=x -@ \""
                             << name.string () << "\"\n";
        CSOUND *own = csoundCreate (NULL, NULL);
        csoundCreateMessageBuffer (own, 0);
        EXPECT_NE (csoundSetOption (own, option.c_str ()), 0) << name;
        std::string messages;
        for (; csoundGetMessageCnt (own) > 0; csoundPopFirstMessage (own))
            messages += csoundGetFirstMessage (own);
        EXPECT_NE (messages.find ("is already being read"), std::string::npos)
            << name;
#ifdef _WIN32
        if (name != hard)
#endif
            EXPECT_STREQ (csoundGetEnv (own, "FROM_AT_CHAIN"), "x") << name;
        csoundDestroy (own);
    }
    fs::remove (link, error);
    fs::remove (hard, error);
    fs::remove (file);
}

TEST_F (OptionParserTests, testRefusedIndirectionFileIsClosed)
{
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path () /
        ("csound-options-" + std::to_string (std::random_device {} ()));
    const std::string option = "-@ \"" + file.string () + "\"";
    std::ofstream (file) << "-%\n";
    EXPECT_NE (csoundSetOption (csound, option.c_str ()), 0);
    /* Corrected, the same file is read again. */
    std::ofstream (file) << "--env:FROM_AT_FILE_AGAIN=1\n";
    EXPECT_EQ (csoundSetOption (csound, option.c_str ()), 0);
    fs::remove (file);
    EXPECT_STREQ (csoundGetEnv (csound, "FROM_AT_FILE_AGAIN"), "1");
}

/* Read for a CSD's options, the same file fails the compile: the frame that
   reads it closes it and passes the jump on to csoundCompileCSD's landing. */
TEST_F (OptionParserTests, testRefusedIndirectionFileFailsTheCsdThatNamesIt)
{
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path () /
        ("csound-options-" + std::to_string (std::random_device {} ()));
    std::ofstream (file) << "-%\n";
    const std::string csd =
        "<CsoundSynthesizer>\n<CsOptions>\n-@ \"" + file.generic_string () +
        "\"\n</CsOptions>\n<CsInstruments>\ninstr 1\nendin\n"
        "</CsInstruments>\n</CsoundSynthesizer>\n";
    EXPECT_NE (csoundCompileCSD (csound, csd.c_str (), 1, 0), CSOUND_SUCCESS);
    fs::remove (file);
}

TEST_F (OptionParserTests, testMessageColoursTakeEitherSpelling)
{
    EXPECT_EQ (csoundSetOption (csound, "--m-colors=1"), 0);
    EXPECT_EQ (csound->oparms->msglevel & 96, 96);
    EXPECT_EQ (csoundSetOption (csound, "--m-colors=10"), 0);
    EXPECT_EQ (csound->oparms->msglevel & 96, 96);
    EXPECT_EQ (csoundSetOption (csound, "--m-colours=0"), 0);
    EXPECT_EQ (csound->oparms->msglevel & 96, 0);
}

TEST_F (OptionParserTests, testCodeAndEventsTakeTheTextAfterTheirEqualsSign)
{
    /* Without the =, the text was read from one past the option's end: the
       next argument here, and past the arguments for the last one. */
    EXPECT_EQ (csoundSetOption (csound, "--code -d"), 0);
    EXPECT_EQ (csound->orchname, nullptr);
    EXPECT_EQ (csound->oparms->displays, 0);
    EXPECT_EQ (csoundSetOption (csound, "--events"), 0);
    EXPECT_EQ (csoundSetOption (csound, "--code=;"), 0);
    EXPECT_STREQ (csound->orchname, "cmd-string");
}

TEST_F (OptionParserTests, testMessageLevelOptionsRefuseAValueThatIsNotANumber)
{
    for (const char *option : {"--m-amps=x", "--m-range=x", "--m-warnings=x",
                               "--m-raw=x", "--m-dB=x", "--m-colours=x",
                               "--m-benchmarks=x"}) {
        const int32_t level = csound->oparms->msglevel;
        EXPECT_NE (csoundSetOption (csound, option), 0) << option;
        EXPECT_EQ (csound->oparms->msglevel, level) << option;
    }
    EXPECT_EQ (csoundSetOption (csound, "--m-amps=0"), 0);
    EXPECT_EQ (csound->oparms->msglevel & CS_AMPLMSG, 0);
}

TEST_F (OptionParserTests, testNumericFlagsRefuseAValueThatIsNotANumber)
{
    /* -b256 left 3 in the count -bx stepped over: past its x, its end and
       into the next argument, whose d then ran as a flag. */
    const int32_t displays = csound->oparms->displays;
    EXPECT_NE (csoundSetOption (csound, "-b256 -bx xd"), 0);
    EXPECT_EQ (csound->oparms->displays, displays);
    for (const char *option : {"-mx", "-Bx", "-jx", "-tx"})
        EXPECT_NE (csoundSetOption (csound, option), 0) << option;
}

TEST_F (OptionParserTests, testEmptyFlagValueStaysInItsArgument)
{
    /* As with -d-, a read past the empty last argument leaves argdecode()'s
       copy, which AddressSanitizer reports. */
    for (const char *option : {"-r \"\"", "-k \"\"", "-x \"\""})
        EXPECT_EQ (csoundSetOption (csound, option), 0) << option;
}

TEST_F (OptionParserTests, testIndirectionFileLeavesTheCsdLineNumbers)
{
    /* The orchestra starts on the same line, whatever line 3 reads. */
    namespace fs = std::filesystem;
    const fs::path file = fs::temp_directory_path () /
        ("csound-options-" + std::to_string (std::random_device {} ()));
    std::ofstream (file) << "-d\n; one\n; two\n; three\n; four\n";
    int32_t offsets[2];
    const std::string lines[2] = {"-d", "-@ \"" + file.string () + "\""};
    for (int i = 0; i < 2; i++) {
        CSOUND *own = csoundCreate (NULL, NULL);
        csoundCreateMessageBuffer (own, 0);
        const std::string csd =
            "<CsoundSynthesizer>\n<CsOptions>\n" + lines[i] +
            "\n</CsOptions>\n<CsInstruments>\ninstr 1\nendin\n"
            "</CsInstruments>\n</CsoundSynthesizer>\n";
        EXPECT_EQ (csoundCompileCSD (own, csd.c_str (), 1, 0), 0) << lines[i];
        offsets[i] = own->orcLineOffset;
        csoundDestroy (own);
    }
    fs::remove (file);
    EXPECT_EQ (offsets[1], offsets[0]);
}

TEST_F (OptionParserTests, testHardwareBufferOptionSetsWhatItsShortFormSets)
{
    const int32_t software = csound->oparms->outbufsamps;
    EXPECT_EQ (csoundSetOption (csound, "--hardwarebufsamps=4096"), 0);
    EXPECT_EQ (csound->oparms->oMaxLag, 4096);
    EXPECT_EQ (csound->oparms->outbufsamps, software);
    EXPECT_EQ (csound->oparms->inbufsamps, software);
}

/* A device list that counts devicesCounted devices, then writes the first
   one if devicesWritten, and answers devicesFilled. */
static int32_t devicesCounted, devicesFilled, devicesFills;
static bool devicesWritten;

static int32_t listMidiDevices (CSOUND *, CS_MIDIDEVICE *list, int32_t)
{
    if (list == NULL)
        return devicesCounted;
    devicesFills++;
    if (devicesWritten) {
        strcpy (list[0].device_name, "first");
        strcpy (list[0].device_id, "hw:0");
    }
    return devicesFilled;
}

static int32_t listAudioDevices (CSOUND *, CS_AUDIODEVICE *list, int32_t)
{
    if (list == NULL)
        return devicesCounted;
    devicesFills++;
    if (devicesWritten) {
        strcpy (list[0].device_name, "first");
        strcpy (list[0].device_id, "hw:0");
        list[0].max_nchnls = 2;
    }
    return devicesFilled;
}

/* What --devices and --midi-devices printed, without other messages. */
static std::string deviceListing (CSOUND *csound)
{
    std::string listing;
    for (; csoundGetMessageCnt (csound) > 0; csoundPopFirstMessage (csound)) {
        const std::string line = csoundGetFirstMessage (csound);
        if (line.find (" input devices\n") != std::string::npos ||
            line.find (" output devices\n") != std::string::npos ||
            line.compare (0, 4, " 0: ") == 0 || line.compare (0, 4, " 1: ") == 0)
            listing += line;
    }
    return listing;
}

TEST_F (OptionParserTests, testDeviceListsPrintOnlyTheDevicesFilled)
{
    ASSERT_EQ (csoundSetOption (csound, "-+rtaudio=null -+rtmidi=null"), 0);
    csoundSetAudioDeviceListCallback (csound, listAudioDevices);
    csoundSetMIDIDeviceListCallback (csound, listMidiDevices);
    deviceListing (csound);
    devicesCounted = 2;
    devicesFilled = 1;
    devicesWritten = true;
    ASSERT_EQ (csoundSetOption (csound, "--midi-devices=in --devices=out"), 0);
    EXPECT_EQ (deviceListing (csound),
               "1 MIDI input devices\n 0: hw:0 (first)\n"
               "1 audio output devices\n 0: hw:0 (first) [ch:2]\n");
    /* One the fill claims and does not write is empty, not what the memory
       held before: one entry, listed and freed just before. */
    devicesCounted = 1;
    for (const bool written : {true, false}) {
        devicesWritten = written;
        ASSERT_EQ (csoundSetOption (csound, "--midi-devices=in --devices=out"), 0);
        EXPECT_EQ (deviceListing (csound), written ?
                   "1 MIDI input devices\n 0: hw:0 (first)\n"
                   "1 audio output devices\n 0: hw:0 (first) [ch:2]\n" :
                   "1 MIDI input devices\n 0:  ()\n"
                   "1 audio output devices\n 0:  ()\n");
    }
    devicesCounted = 2;
    devicesFilled = CSOUND_ERROR;
    ASSERT_EQ (csoundSetOption (csound, "--midi-devices=out --devices=in"), 0);
    EXPECT_EQ (deviceListing (csound),
               "0 MIDI output devices\n0 audio input devices\n");
    /* With no device counted, or an error, there is nothing to fill. */
    devicesFills = 0;
    for (const int32_t counted : {0, (int32_t) CSOUND_ERROR}) {
        devicesCounted = counted;
        ASSERT_EQ (csoundSetOption (csound, "--midi-devices=in --devices=in"), 0);
        EXPECT_EQ (deviceListing (csound),
                   "0 MIDI input devices\n0 audio input devices\n") << counted;
    }
    EXPECT_EQ (devicesFills, 0);
}
