#include "csound.h"
#include "gtest/gtest.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

class JsonUdtTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        restart();
    }

    void restart()
    {
        if (csound)
            csoundDestroy(csound);
        csound = csoundCreate(nullptr, nullptr);
        ASSERT_NE(csound, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
    }

    void TearDown() override
    {
        if (csound)
            csoundDestroy(csound);
        if (!directory.empty()) {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }
    }

    std::string messages()
    {
        std::string text;
        while (csoundGetMessageCnt(csound)) {
            text += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return text;
    }

    void expectChannel(const char *name, MYFLT expected)
    {
        int32_t error = 0;
        MYFLT actual = csoundGetControlChannel(csound, name, &error);
        ASSERT_EQ(error, 0) << name;
        EXPECT_EQ(actual, expected) << name;
    }

    void expectStringChannel(const char *name, const char *expected)
    {
        int32_t size = csoundGetChannelDatasize(csound, name);
        ASSERT_GT(size, 0) << name;
        std::vector<char> value(static_cast<size_t>(size));
        csoundGetStringChannel(csound, name, value.data());
        EXPECT_STREQ(value.data(), expected) << name;
    }

    void run(const std::string &declarations, const std::string &body,
             const char *source = nullptr,
             const std::vector<std::pair<std::string, MYFLT>> &controls = {})
    {
        const std::string orchestra =
            "sr=44100\nksmps=32\nnchnls=1\n" + declarations +
            "\ninstr 1\n" + body + "\nendin\n";
        ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), 0)
            << messages();
        if (source)
            csoundSetStringChannel(csound, "source", source);
        for (const auto &control : controls)
            csoundSetControlChannel(csound, control.first.c_str(), control.second);
        csoundEventString(csound, "i 1 0 0.1\ne\n", 0);
        ASSERT_EQ(csoundStart(csound), 0) << messages();
        int32_t result = 0;
        for (int cycle = 0; cycle < 400 && result == 0; ++cycle)
            result = csoundPerformKsmps(csound);
        ASSERT_GT(result, 0) << messages();
    }

    void expectError(const char *opcode, const char *detail)
    {
        const std::string text = messages();
        EXPECT_GT(csoundErrCnt(csound), 0) << text;
        EXPECT_NE(text.find(opcode), std::string::npos) << text;
        EXPECT_NE(text.find(detail), std::string::npos) << text;
    }

    void writeFile(const std::string &name, const std::string &contents)
    {
        if (directory.empty()) {
            const auto stamp = std::chrono::steady_clock::now()
                                   .time_since_epoch().count();
            const auto root = std::filesystem::temp_directory_path();
            for (int attempt = 0; attempt < 100; ++attempt) {
                auto candidate = root / ("csound-json-udt-" +
                    std::to_string(stamp) + "-" + std::to_string(attempt));
                std::error_code error;
                if (std::filesystem::create_directory(candidate, error)) {
                    directory = candidate;
                    break;
                }
                ASSERT_TRUE(!error || error == std::errc::file_exists)
                    << error.message();
            }
            ASSERT_FALSE(directory.empty());
        }
        std::ofstream file(directory / name, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        file.write(contents.data(),
                   static_cast<std::streamsize>(contents.size()));
        file.close();
        ASSERT_TRUE(file.good());
    }

    CSOUND *csound = nullptr;
    std::filesystem::path directory;
};

TEST_F(JsonUdtTests, RoundTripsDeclaredNumericMembers)
{
    const char *orchestra = R"orc(
sr = 44100
ksmps = 32
nchnls = 1
struct Note pitch:i, onset:i, duration:i, velocity:i
instr 1
    chnset 1, "ran"
    note:Note jsonunmarshal {{ {"pitch":60,"onset":0.25,"duration":1.5,"velocity":96} }}
    chnset note.pitch, "pitch"
    chnset note.onset, "onset"
    chnset note.duration, "duration"
    chnset note.velocity, "velocity"
    encoded:S jsonmarshal note
    restored:Note jsonunmarshal encoded
    chnset restored.pitch, "restored-pitch"
    chnset restored.onset, "restored-onset"
    chnset restored.duration, "restored-duration"
    chnset restored.velocity, "restored-velocity"
endin
)orc";

    ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), 0) << messages();
    csoundEventString(csound, "i 1 0 0.1\ne\n", 0);
    ASSERT_EQ(csoundStart(csound), 0) << messages();
    int32_t result = 0;
    for (int cycle = 0; cycle < 400 && result == 0; ++cycle)
        result = csoundPerformKsmps(csound);
    SCOPED_TRACE(messages());
    ASSERT_GT(result, 0);
    ASSERT_EQ(csoundErrCnt(csound), 0);

    expectChannel("ran", 1);
    expectChannel("pitch", 60);
    expectChannel("onset", 0.25);
    expectChannel("duration", 1.5);
    expectChannel("velocity", 96);
    expectChannel("restored-pitch", 60);
    expectChannel("restored-onset", 0.25);
    expectChannel("restored-duration", 1.5);
    expectChannel("restored-velocity", 96);
}

TEST_F(JsonUdtTests, RoundTripsNestedStructsWithStringsAndTypedArrays)
{
    const char *orchestra = R"orc(
sr = 44100
ksmps = 32
nchnls = 1
struct Note pitch:i, label:S
struct Phrase title:S, notes:Note[]
struct Score phrase:Phrase
instr 1
    score:Score jsonunmarshal {{ {
        "phrase": {
            "title": "Prelude",
            "notes": [
                {"pitch": 60, "label": "start"},
                {"pitch": 67, "label": "end"}
            ]
        }
    } }}
    chnset score.phrase.title, "title"
    chnset lenarray(score.phrase.notes), "count"
    chnset score.phrase.notes[0].pitch, "first-pitch"
    chnset score.phrase.notes[0].label, "first-label"
    chnset score.phrase.notes[1].pitch, "second-pitch"
    chnset score.phrase.notes[1].label, "second-label"
    encoded:S jsonmarshal score
    restored:Score jsonunmarshal encoded
    chnset restored.phrase.title, "restored-title"
    chnset lenarray(restored.phrase.notes), "restored-count"
    chnset restored.phrase.notes[0].pitch, "restored-first-pitch"
    chnset restored.phrase.notes[0].label, "restored-first-label"
    chnset restored.phrase.notes[1].pitch, "restored-second-pitch"
    chnset restored.phrase.notes[1].label, "restored-second-label"
endin
)orc";

    ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), 0) << messages();
    csoundEventString(csound, "i 1 0 0.1\ne\n", 0);
    ASSERT_EQ(csoundStart(csound), 0) << messages();
    int32_t result = 0;
    for (int cycle = 0; cycle < 400 && result == 0; ++cycle)
        result = csoundPerformKsmps(csound);
    SCOPED_TRACE(messages());
    ASSERT_GT(result, 0);
    ASSERT_EQ(csoundErrCnt(csound), 0);

    expectStringChannel("title", "Prelude");
    expectChannel("count", 2);
    expectChannel("first-pitch", 60);
    expectStringChannel("first-label", "start");
    expectChannel("second-pitch", 67);
    expectStringChannel("second-label", "end");
    expectStringChannel("restored-title", "Prelude");
    expectChannel("restored-count", 2);
    expectChannel("restored-first-pitch", 60);
    expectStringChannel("restored-first-label", "start");
    expectChannel("restored-second-pitch", 67);
    expectStringChannel("restored-second-label", "end");
}

TEST_F(JsonUdtTests, JsoncExtensionsRequireTheirOwnFlags)
{
    struct Case {
        const char *source;
        const char *options;
        bool accepted;
    };
    const Case cases[] = {
        {"{\"pitch\":60}", "", true},
        {"{/* note */\"pitch\":60}", "", false},
        {"{/* note */\"pitch\":60}", ", 1", true},
        {"{/* note */\"pitch\":60}", ", 2", false},
        {"{\"pitch\":60,}", "", false},
        {"{\"pitch\":60,}", ", 1", false},
        {"{\"pitch\":60,}", ", 2", true},
        {"// note\n{\"pitch\":60,}", ", 1", false},
        {"// note\n{\"pitch\":60,}", ", 2", false},
        {"// note\n{\"pitch\":60,}", ", 3, 0", true},
    };
    for (const Case &item : cases) {
        SCOPED_TRACE(std::string(item.source) + item.options);
        ASSERT_NO_FATAL_FAILURE(restart());
        ASSERT_NO_FATAL_FAILURE(run("struct Note pitch:i",
            std::string("source:S chnget \"source\"\n") +
            "note:Note jsonunmarshal source" + item.options +
            "\nchnset note.pitch, \"pitch\"", item.source));
        const std::string text = messages();
        if (item.accepted) {
            ASSERT_EQ(csoundErrCnt(csound), 0) << text;
            expectChannel("pitch", 60);
        } else {
            EXPECT_GT(csoundErrCnt(csound), 0) << text;
            EXPECT_NE(text.find("jsonunmarshal:"), std::string::npos) << text;
        }
    }
}

TEST_F(JsonUdtTests, ReadsJsoncFileAndProducesPrettyJson)
{
    ASSERT_NO_FATAL_FAILURE(writeFile("note.jsonc",
        "// first note\n{\"pitch\":67,}\n"));
    const std::string path = (directory / "note.jsonc").generic_string();
    ASSERT_NO_FATAL_FAILURE(run("struct Note pitch:i", R"orc(
source:S chnget "source"
note:Note jsonunmarshalfile source, 3, 0
chnset note.pitch, "pitch"
encoded:S jsonmarshal note, 1, 0
chnset encoded, "encoded"
restored:Note jsonunmarshal encoded
chnset restored.pitch, "restored-pitch"
)orc", path.c_str()));
    const std::string text = messages();
    ASSERT_EQ(csoundErrCnt(csound), 0) << text;
    expectChannel("pitch", 67);
    expectChannel("restored-pitch", 67);
    const int32_t size = csoundGetChannelDatasize(csound, "encoded");
    ASSERT_GT(size, 0);
    std::vector<char> encoded(static_cast<size_t>(size));
    csoundGetStringChannel(csound, "encoded", encoded.data());
    EXPECT_NE(std::string(encoded.data()).find('\n'), std::string::npos);
}

TEST_F(JsonUdtTests, RejectsMalformedObjectsWithFieldPaths)
{
    struct Case {
        const char *note;
        const char *path;
        const char *reason;
    };
    const Case cases[] = {
        {R"json({"label":"end"})json",
         "$.phrase.notes[1].pitch", "missing"},
        {R"json({"pitch":67,"label":"end","extra":1})json",
         "$.phrase.notes[1].extra", "unknown"},
        {R"json({"pitch":67,"pitch":68})json",
         "$.phrase.notes[1].pitch", "duplicate"},
        {R"json({"pitch":67,"\u0070itch":68})json",
         "$.phrase.notes[1].pitch", "duplicate"},
        {R"json({"pitch\u0000hidden":67,"label":"end"})json",
         "$.phrase.notes[1]", "NUL"},
        {R"json({"pitch":null,"label":"end"})json",
         "$.phrase.notes[1].pitch", "number"},
        {R"json({"pitch":"67","label":"end"})json",
         "$.phrase.notes[1].pitch", "number"},
        {R"json({"pitch":true,"label":"end"})json",
         "$.phrase.notes[1].pitch", "number"},
        {R"json({"pitch":67,"label":null})json",
         "$.phrase.notes[1].label", "string"},
        {R"json({"pitch":NaN,"label":"end"})json", nullptr, nullptr},
        {R"json({"pitch":Infinity,"label":"end"})json", nullptr, nullptr},
        {R"json({"pitch":1e999,"label":"end"})json", nullptr, nullptr},
    };
    for (const Case &item : cases) {
        SCOPED_TRACE(item.note);
        ASSERT_NO_FATAL_FAILURE(restart());
        const std::string source =
            std::string("{\"phrase\":{\"notes\":[") +
            "{\"pitch\":60,\"label\":\"start\"}," + item.note + "]}}";
        ASSERT_NO_FATAL_FAILURE(run(
            "struct Note pitch:i, label:S\n"
            "struct Phrase notes:Note[]\n"
            "struct Score phrase:Phrase", R"orc(
source:S chnget "source"
score:Score jsonunmarshal source
chnset 1, "completed"
)orc", source.c_str()));
        const std::string text = messages();
        EXPECT_GT(csoundErrCnt(csound), 0) << text;
        EXPECT_NE(text.find("jsonunmarshal:"), std::string::npos) << text;
        if (item.path)
            EXPECT_NE(text.find(item.path), std::string::npos) << text;
        if (item.reason)
            EXPECT_NE(text.find(item.reason), std::string::npos) << text;
        expectChannel("completed", 0);
    }
}

std::string nestedNodes(size_t count)
{
    std::string source = R"json({"label":"leaf","children":[]})json";
    for (size_t node = 1; node < count; ++node)
        source = "{\"label\":\"node\",\"children\":[" + source + "]}";
    return source;
}

TEST_F(JsonUdtTests, RoundTripsAtTheMaximumContainerDepth)
{
    const std::string source = nestedNodes(128);
    ASSERT_NO_FATAL_FAILURE(run("struct Node label:S, children:Node[]", R"orc(
source:S chnget "source"
tree:Node jsonunmarshal source
encoded:S jsonmarshal tree
chnset encoded, "encoded"
restored:Node jsonunmarshal encoded
encodedAgain:S jsonmarshal restored
chnset encodedAgain, "encoded-again"
)orc", source.c_str()));
    ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
    expectStringChannel("encoded", source.c_str());
    expectStringChannel("encoded-again", source.c_str());

    ASSERT_NO_FATAL_FAILURE(restart());
    const std::string tooDeep = nestedNodes(129);
    ASSERT_NO_FATAL_FAILURE(run("struct Node label:S, children:Node[]", R"orc(
source:S chnget "source"
tree:Node jsonunmarshal source
)orc", tooDeep.c_str()));
    const std::string text = messages();
    EXPECT_GT(csoundErrCnt(csound), 0) << text;
    EXPECT_NE(text.find("jsonunmarshal:"), std::string::npos) << text;
    EXPECT_NE(text.find("depth"), std::string::npos) << text;
}

TEST_F(JsonUdtTests, AppliesContainerDepthLimitsToReadingAndWriting)
{
    struct Case {
        size_t nodes;
        int readLimit;
        int writeLimit;
        const char *failure;
    };
    const Case cases[] = {
        {1, 2, 2, nullptr},
        {1, 1, 0, "jsonunmarshal:"},
        {1, 2, 1, "jsonmarshal:"},
        {2, 4, 4, nullptr},
        {2, 3, 0, "jsonunmarshal:"},
        {2, 4, 3, "jsonmarshal:"},
    };
    for (const Case &item : cases) {
        SCOPED_TRACE(std::to_string(item.nodes) + " nodes, read limit " +
                     std::to_string(item.readLimit) + ", write limit " +
                     std::to_string(item.writeLimit));
        ASSERT_NO_FATAL_FAILURE(restart());
        const std::string source = nestedNodes(item.nodes);
        ASSERT_NO_FATAL_FAILURE(run("struct Node label:S, children:Node[]",
            "source:S chnget \"source\"\n"
            "tree:Node jsonunmarshal source, 0, " +
            std::to_string(item.readLimit) +
            "\nencoded:S jsonmarshal tree, 0, " +
            std::to_string(item.writeLimit) +
            "\nchnset encoded, \"encoded\"", source.c_str()));
        const std::string text = messages();
        if (item.failure) {
            EXPECT_GT(csoundErrCnt(csound), 0) << text;
            EXPECT_NE(text.find(item.failure), std::string::npos) << text;
            EXPECT_NE(text.find("depth"), std::string::npos) << text;
        } else {
            ASSERT_EQ(csoundErrCnt(csound), 0) << text;
            expectStringChannel("encoded", source.c_str());
        }
    }

    ASSERT_NO_FATAL_FAILURE(restart());
    ASSERT_NO_FATAL_FAILURE(run("struct Flat label:S", R"orc(
flat:Flat jsonunmarshal {{ {"label":"leaf"} }}, 0, 1
encoded:S jsonmarshal flat, 0, 1
chnset encoded, "encoded"
)orc"));
    ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
    expectStringChannel("encoded", R"json({"label":"leaf"})json");
}

TEST_F(JsonUdtTests, ChecksDepthOfUnknownFieldsBeforeBindingTheSchema)
{
    const std::string source = "{\"label\":\"ok\",\"unexpected\":" +
        std::string(10000, '[') + "0" + std::string(10000, ']') + "}";
    ASSERT_NO_FATAL_FAILURE(run("struct Flat label:S", R"orc(
source:S chnget "source"
flat:Flat jsonunmarshal source
)orc", source.c_str()));
    const std::string text = messages();
    EXPECT_GT(csoundErrCnt(csound), 0) << text;
    EXPECT_NE(text.find("jsonunmarshal:"), std::string::npos) << text;
    EXPECT_NE(text.find("depth"), std::string::npos) << text;
    EXPECT_EQ(text.find("unknown field"), std::string::npos) << text;
}

TEST_F(JsonUdtTests, RejectsTruncatedDeepJsonWithoutCrashing)
{
    const std::string source = "{\"label\":" + std::string(10000, '[') + "0";
    ASSERT_NO_FATAL_FAILURE(run("struct Flat label:S", R"orc(
source:S chnget "source"
flat:Flat jsonunmarshal source
)orc", source.c_str()));
    const std::string text = messages();
    EXPECT_GT(csoundErrCnt(csound), 0) << text;
    EXPECT_NE(text.find("jsonunmarshal:"), std::string::npos) << text;
}

TEST_F(JsonUdtTests, RoundTripsRootTypedStructArrays)
{
    ASSERT_NO_FATAL_FAILURE(run("struct Note pitch:i, label:S", R"orc(
notes:Note[] jsonunmarshal {{ [
    {"pitch":60,"label":"start"}, {"pitch":67,"label":"end"}
] }}
chnset lenarray(notes), "count"
chnset notes[0].pitch, "pitch"
chnset notes[1].label, "label"
encoded:S jsonmarshal notes
restored:Note[] jsonunmarshal encoded
chnset lenarray(restored), "restored-count"
chnset restored[0].pitch, "restored-pitch"
chnset restored[1].label, "restored-label"
)orc"));
    ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
    expectChannel("count", 2);
    expectChannel("pitch", 60);
    expectStringChannel("label", "end");
    expectChannel("restored-count", 2);
    expectChannel("restored-pitch", 60);
    expectStringChannel("restored-label", "end");
}

TEST_F(JsonUdtTests, RoundTripsInitAndControlRateBooleans)
{
    ASSERT_NO_FATAL_FAILURE(run("struct Flags enabled:b, sounding:B", R"orc(
flags:Flags jsonunmarshal {{ {"enabled":true,"sounding":false} }}
enabled:b = flags.enabled
sounding:B = flags.sounding
enabledNumber:i = enabled
soundingNumber:k = sounding
chnset enabledNumber, "enabled"
chnset soundingNumber, "sounding"
encoded:S jsonmarshal flags
chnset encoded, "encoded"
restored:Flags jsonunmarshal encoded
restoredEnabled:b = restored.enabled
restoredSounding:B = restored.sounding
restoredEnabledNumber:i = restoredEnabled
restoredSoundingNumber:k = restoredSounding
chnset restoredEnabledNumber, "restored-enabled"
chnset restoredSoundingNumber, "restored-sounding"
)orc"));
    ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
    expectChannel("enabled", 1);
    expectChannel("sounding", 0);
    expectStringChannel("encoded", R"json({"enabled":true,"sounding":false})json");
    expectChannel("restored-enabled", 1);
    expectChannel("restored-sounding", 0);
}

TEST_F(JsonUdtTests, ReplacingValuesPreservesSharedNestedArrayCopies)
{
    ASSERT_NO_FATAL_FAILURE(run(
        "struct Child text:S, values:i[]\n"
        "struct Inner children:Child[]\n"
        "struct Outer title:S, inner:Inner", R"orc(
target:Outer jsonunmarshal {{ {"title":"first","inner":{"children":[{"text":"kept","values":[1,2,3]}]} } }}
copy:Outer = target
shared:Child[] = target.inner.children
target jsonunmarshal {{ {"title":"second","inner":{"children":[{"text":"new","values":[4]},{"text":"another","values":[]}]} } }}
encoded:S jsonmarshal target
old:S jsonmarshal copy
oldcopy:Outer jsonunmarshal old
chnset oldcopy.title, "old-title"
chnset oldcopy.inner.children[0].text, "old-text"
chnset oldcopy.inner.children[0].values[2], "old-number"
chnset shared[0].text, "shared-text"
chnset shared[0].values[2], "shared-number"
chnset target.inner.children[1].text, "new-text"
target jsonunmarshal {{ {"title":"empty","inner":{"children":[]} } }}
encoded jsonmarshal target
chnset encoded, "empty"
chnset copy.inner.children[0].values[2], "still-old-number"
)orc"));
    ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
    expectStringChannel("old-title", "first");
    expectStringChannel("old-text", "kept");
    expectStringChannel("shared-text", "kept");
    expectStringChannel("new-text", "another");
    expectChannel("old-number", 3);
    expectChannel("shared-number", 3);
    expectChannel("still-old-number", 3);
    expectStringChannel("empty", "{\"title\":\"empty\",\"inner\":{\"children\":[]}}");
}

TEST_F(JsonUdtTests, FailedDecodeLeavesExistingHeapFieldsUnchanged)
{
    const char *orchestra = R"orc(
sr=44100
ksmps=32
nchnls=1
struct Child text:S, values:i[]
struct Outer title:S, children:Child[], last:i
target@global:Outer jsonunmarshal {{ {"title":"original","children":[{"text":"keep","values":[1,2,3]}],"last":7} }}
instr 1
target jsonunmarshal {{ {"title":"changed","children":[{"text":"discard","values":[9,8]}],"last":null} }}
endin
instr 2
chnset target.title, "title"
chnset target.children[0].text, "text"
chnset target.children[0].values[2], "number"
chnset target.last, "last"
chnset 1, "checked"
endin
)orc";
    ASSERT_EQ(csoundCompileOrc(csound, orchestra, 0), 0) << messages();
    csoundEventString(csound, "i 1 0 0.02\ni 2 0.025 0.02\ne\n", 0);
    ASSERT_EQ(csoundStart(csound), 0) << messages();
    int32_t result = 0;
    for (int cycle = 0; cycle < 400 && result == 0; ++cycle)
        result = csoundPerformKsmps(csound);
    const std::string text = messages();
    EXPECT_GT(result, 0) << text;
    EXPECT_GT(csoundErrCnt(csound), 0) << text;
    EXPECT_NE(text.find("jsonunmarshal:"), std::string::npos) << text;
    EXPECT_NE(text.find("$.last"), std::string::npos) << text;
    expectChannel("checked", 1);
    expectStringChannel("title", "original");
    expectStringChannel("text", "keep");
    expectChannel("number", 3);
    expectChannel("last", 7);
}

TEST_F(JsonUdtTests, PreservesEveryBitOfFiniteMyfltValues)
{
    const MYFLT values[] = {
        MYFLT(0), -MYFLT(0), MYFLT(1) / MYFLT(3),
        std::nextafter(MYFLT(1), MYFLT(2)),
        std::nextafter(MYFLT(1), MYFLT(0)),
        std::numeric_limits<MYFLT>::epsilon(),
        std::numeric_limits<MYFLT>::min(),
        std::numeric_limits<MYFLT>::max(),
        std::numeric_limits<MYFLT>::lowest(),
        std::numeric_limits<MYFLT>::denorm_min(),
    };
    for (MYFLT expected : values) {
        std::ostringstream json;
        json.imbue(std::locale::classic());
        json << "{\"value\":" << std::scientific
             << std::setprecision(std::numeric_limits<MYFLT>::max_digits10)
             << expected << "}";
        const std::string source = json.str();
        SCOPED_TRACE(source);
        ASSERT_NO_FATAL_FAILURE(restart());
        ASSERT_NO_FATAL_FAILURE(run("struct Number value:i", R"orc(
source:S chnget "source"
number:Number jsonunmarshal source
chnset number.value, "value"
encoded:S jsonmarshal number
restored:Number jsonunmarshal encoded
chnset restored.value, "restored-value"
)orc", source.c_str()));
        ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
        for (const char *name : {"value", "restored-value"}) {
            int32_t error = 0;
            const MYFLT actual = csoundGetControlChannel(csound, name, &error);
            ASSERT_EQ(error, 0) << name;
            EXPECT_EQ(std::memcmp(&actual, &expected, sizeof(MYFLT)), 0)
                << name << ": expected " << expected << ", got " << actual;
        }
    }
}

TEST_F(JsonUdtTests, PreservesUnicodeEscapedControlsAndLongStrings)
{
    const std::string escaped =
        R"json({"text":"\uD834\uDD1E caf\u00E9 \"quote\" \\ slash\/\b\f\n\r\t"})json";
    const std::string decoded =
        u8"\U0001D11E caf\u00E9 \"quote\" \\ slash/\b\f\n\r\t";
    const std::string longValue(50000, 'x');
    const std::pair<std::string, std::string> cases[] = {
        {escaped, decoded},
        {"{\"text\":\"\"}", ""},
        {"{\"text\":\"" + longValue + "\"}", longValue},
        {R"json({"text":"/* keep */ // keep [ ] { }"})json",
         "/* keep */ // keep [ ] { }"},
    };
    for (const auto &item : cases) {
        ASSERT_NO_FATAL_FAILURE(restart());
        ASSERT_NO_FATAL_FAILURE(run("struct Text text:S", R"orc(
source:S chnget "source"
text:Text jsonunmarshal source
chnset text.text, "text"
encoded:S jsonmarshal text
restored:Text jsonunmarshal encoded
chnset restored.text, "restored-text"
)orc", item.first.c_str()));
        ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
        expectStringChannel("text", item.second.c_str());
        expectStringChannel("restored-text", item.second.c_str());
    }
}

TEST_F(JsonUdtTests, RejectsInvalidUnicodeAndEmbeddedNulStrings)
{
    const std::string cases[] = {
        R"json({"text":"\uD834"})json",
        R"json({"text":"\uDD1E"})json",
        R"json({"text":"a\u0000b"})json",
        std::string("{\"text\":\"") + char(0xc0) + char(0xaf) + "\"}",
        std::string("{\"text\":\"") + char(0xed) + char(0xa0) + char(0x80) + "\"}",
        std::string("{\"text\":\"") + char(0x80) + "\"}",
    };
    for (const std::string &source : cases) {
        ASSERT_NO_FATAL_FAILURE(restart());
        ASSERT_NO_FATAL_FAILURE(run("struct Text text:S", R"orc(
source:S chnget "source"
text:Text jsonunmarshal source
)orc", source.c_str()));
        const std::string text = messages();
        EXPECT_GT(csoundErrCnt(csound), 0) << text;
        EXPECT_NE(text.find("jsonunmarshal:"), std::string::npos) << text;
    }
}

TEST_F(JsonUdtTests, RejectsUnsupportedTypesEvenInEmptyArrays)
{
    for (const char *type : {"a", "f", "Opcode"}) {
        for (bool array : {false, true}) {
            const std::string fieldType = std::string(type) + (array ? "[]" : "");
            const std::string declaration = "struct Holder value:" + fieldType;
            const std::string source = array ? "{\"value\":[]}" : "{\"value\":0}";
            SCOPED_TRACE(fieldType);
            ASSERT_NO_FATAL_FAILURE(restart());
            ASSERT_NO_FATAL_FAILURE(run(declaration, R"orc(
source:S chnget "source"
holder:Holder jsonunmarshal source
)orc", source.c_str()));
            const std::string decodeText = messages();
            EXPECT_GT(csoundErrCnt(csound), 0) << decodeText;
            EXPECT_NE(decodeText.find("jsonunmarshal:"), std::string::npos)
                << decodeText;
            EXPECT_NE(decodeText.find("unsupported"), std::string::npos)
                << decodeText;
            EXPECT_NE(decodeText.find("$.value"), std::string::npos) << decodeText;

            ASSERT_NO_FATAL_FAILURE(restart());
            const std::string initialization = array
                ? "items:" + fieldType + " init 0\nholder:Holder init items\n"
                : "holder:Holder = init()\n";
            ASSERT_NO_FATAL_FAILURE(run(declaration,
                initialization + "encoded:S jsonmarshal holder"));
            const std::string encodeText = messages();
            EXPECT_GT(csoundErrCnt(csound), 0) << encodeText;
            EXPECT_NE(encodeText.find("jsonmarshal:"), std::string::npos)
                << encodeText;
            EXPECT_NE(encodeText.find("unsupported"), std::string::npos)
                << encodeText;
            EXPECT_NE(encodeText.find("$.value"), std::string::npos) << encodeText;
        }
    }

    ASSERT_NO_FATAL_FAILURE(restart());
    ASSERT_NO_FATAL_FAILURE(run(
        "struct Hidden signal:a\nstruct Holder values:Hidden[]", R"orc(
holder:Holder jsonunmarshal {{ {"values":[]} }}
)orc"));
    const std::string decodeText = messages();
    EXPECT_GT(csoundErrCnt(csound), 0) << decodeText;
    EXPECT_NE(decodeText.find("unsupported"), std::string::npos) << decodeText;
    EXPECT_NE(decodeText.find("$.values"), std::string::npos) << decodeText;

    ASSERT_NO_FATAL_FAILURE(restart());
    ASSERT_NO_FATAL_FAILURE(run(
        "struct Hidden signal:a\nstruct Holder values:Hidden[]", R"orc(
values:Hidden[] init 0
holder:Holder init values
encoded:S jsonmarshal holder
)orc"));
    const std::string encodeText = messages();
    EXPECT_GT(csoundErrCnt(csound), 0) << encodeText;
    EXPECT_NE(encodeText.find("unsupported"), std::string::npos) << encodeText;
    EXPECT_NE(encodeText.find("$.values"), std::string::npos) << encodeText;
}

TEST_F(JsonUdtTests, RejectsInvalidOptionsBeforeReadingOrWriting)
{
    const char *readOptions[] = {
        "4", "-1", "0.5", "0, -1", "0, 1.5", "0, 257",
    };
    for (const char *options : readOptions) {
        for (const char *opcode : {"jsonunmarshal", "jsonunmarshalfile"}) {
            SCOPED_TRACE(std::string(opcode) + " " + options);
            ASSERT_NO_FATAL_FAILURE(restart());
            ASSERT_NO_FATAL_FAILURE(run("struct Number value:i",
                "source:S chnget \"source\"\nnumber:Number " +
                std::string(opcode) + " source, " + options,
                "{\"value\":1}"));
            expectError((std::string(opcode) + ":").c_str(), "invalid");
        }
    }
    for (const char *options : {"2", "-1", "0.5", "0, -1", "0, 1.5", "0, 257"}) {
        SCOPED_TRACE(options);
        ASSERT_NO_FATAL_FAILURE(restart());
        ASSERT_NO_FATAL_FAILURE(run("struct Number value:i",
            std::string("number:Number init 1\nencoded:S jsonmarshal number, ") +
            options));
        expectError("jsonmarshal:", "invalid");
    }
}

TEST_F(JsonUdtTests, ReportsMissingEmptyAndMalformedFiles)
{
    ASSERT_NO_FATAL_FAILURE(writeFile("valid.json", "{\"value\":7}"));
    ASSERT_NO_FATAL_FAILURE(writeFile("empty.json", ""));
    ASSERT_NO_FATAL_FAILURE(writeFile("broken.json", "{\"value\":"));
    ASSERT_NO_FATAL_FAILURE(writeFile("trailing.json", "{\"value\":7} junk"));
    ASSERT_NO_FATAL_FAILURE(writeFile("nul.json",
        std::string("{\"value\":7}") + '\0' + "junk"));
    for (const char *name : {"missing.json", "empty.json", "broken.json",
                             "trailing.json", "nul.json"}) {
        SCOPED_TRACE(name);
        ASSERT_NO_FATAL_FAILURE(restart());
        const std::string path = (directory / name).generic_string();
        ASSERT_NO_FATAL_FAILURE(run("struct Number value:i", R"orc(
source:S chnget "source"
number:Number jsonunmarshalfile source
)orc", path.c_str()));
        expectError("jsonunmarshalfile:", std::string(name) == "missing.json"
                    ? "cannot open file" : "byte");
    }

    const std::string path = (directory / "valid.json").generic_string();
    ASSERT_NO_FATAL_FAILURE(restart());
    ASSERT_NO_FATAL_FAILURE(run("struct Number value:i", R"orc(
source:S chnget "source"
number:Number jsonunmarshal source
)orc", path.c_str()));
    expectError("jsonunmarshal:", "byte");
}

TEST_F(JsonUdtTests, RejectsScalarRootsAndMultidimensionalOrRaggedArrays)
{
    struct Case {
        const char *body;
        const char *source;
        const char *opcode;
        const char *reason;
    };
    const Case cases[] = {
        {"value:i jsonunmarshal source", "7", "jsonunmarshal:", "declared UDT"},
        {"encoded:S jsonmarshal 7", "", "jsonmarshal:", "declared UDT"},
        {"values:i[][] init 2, 2\nvalues jsonunmarshal source", "[[1,2],[3,4]]",
         "jsonunmarshal:", "one-dimensional"},
        {"values:i[][] init 2, 2\nencoded:S jsonmarshal values", "",
         "jsonmarshal:", "one-dimensional"},
        {"values:i[] jsonunmarshal source", "[[1,2],[3]]",
         "jsonunmarshal:", "$[0]"},
    };
    for (const Case &item : cases) {
        SCOPED_TRACE(item.body);
        ASSERT_NO_FATAL_FAILURE(restart());
        ASSERT_NO_FATAL_FAILURE(run("",
            std::string("source:S chnget \"source\"\n") + item.body, item.source));
        expectError(item.opcode, item.reason);
    }
}

TEST_F(JsonUdtTests, RejectsNonfiniteValuesDuringMarshal)
{
    const MYFLT values[] = {
        std::numeric_limits<MYFLT>::quiet_NaN(),
        std::numeric_limits<MYFLT>::infinity(),
        -std::numeric_limits<MYFLT>::infinity(),
    };
    for (MYFLT value : values) {
        ASSERT_NO_FATAL_FAILURE(restart());
        ASSERT_NO_FATAL_FAILURE(run("struct Number value:i", R"orc(
input:i chnget "input"
number:Number init input
encoded:S jsonmarshal number
)orc", nullptr, {{"input", value}}));
        const std::string text = messages();
        EXPECT_GT(csoundErrCnt(csound), 0) << text;
        EXPECT_NE(text.find("jsonmarshal:"), std::string::npos) << text;
        EXPECT_NE(text.find("finite number"), std::string::npos) << text;
        EXPECT_NE(text.find("$.value"), std::string::npos) << text;
    }
}

TEST_F(JsonUdtTests, ConvertsOnlyDuringInitialization)
{
    ASSERT_NO_FATAL_FAILURE(run(R"orc(
struct Number value:k
struct Snapshot value:i
instr 2
    encoded:S chnget "encoded"
    snapshot:Snapshot jsonunmarshal encoded
    chnset snapshot.value, "snapshot"
endin
)orc", R"orc(
source:S init {{ {"value":1} }}
number:Number jsonunmarshal source
encoded:S jsonmarshal number
schedule 2, 0.05, 0.01
source strcpyk "invalid JSON after initialization"
number.value = 2
chnset number.value, "value"
chnset encoded, "encoded"
)orc"));
    ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
    expectChannel("value", 2);
    expectChannel("snapshot", 1);
}

TEST_F(JsonUdtTests, ReinitializesTheSameHeapBackedOpcodeInstances)
{
    ASSERT_NO_FATAL_FAILURE(run("struct Value text:S, values:i[]", R"orc(
round:i init 0
cycle:k init 0
cycle += 1
if cycle < 50 then
    reinit READ
endif
READ:
source:S sprintf {{ {"text":"%d","values":[%d]} }}, round, round
value:Value jsonunmarshal source
encoded:S jsonmarshal value
restored:Value jsonunmarshal encoded
chnset restored.text, "text"
chnset restored.values[0], "number"
round += 1
rireturn
)orc"));
    ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
    expectStringChannel("text", "49");
    expectChannel("number", 49);
}

TEST_F(JsonUdtTests, RoundTripsAChainOfDistinctDeclaredTypes)
{
    constexpr int count = 128;
    std::string declarations = "struct Layer0 text:S\n";
    std::string source = "{\"text\":\"leaf\"}";
    for (int index = 1; index < count; ++index) {
        declarations += "struct Layer" + std::to_string(index) +
            " child:Layer" + std::to_string(index - 1) + "\n";
        source = "{\"child\":" + source + "}";
    }
    ASSERT_NO_FATAL_FAILURE(run(declarations, R"orc(
source:S chnget "source"
root:Layer127 jsonunmarshal source
encoded:S jsonmarshal root
restored:Layer127 jsonunmarshal encoded
encodedAgain:S jsonmarshal restored
chnset encodedAgain, "encoded"
)orc", source.c_str()));
    ASSERT_EQ(csoundErrCnt(csound), 0) << messages();
    expectStringChannel("encoded", source.c_str());
}

} // namespace
