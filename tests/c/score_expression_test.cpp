#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
extern "C" {
#include "corfile.h"
#include "extract.h"
}
#include "gtest/gtest.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {
void checkExpression(const std::string& expression, cs_float expected,
                     const char* error = nullptr, int mode = 1)
{
    SCOPED_TRACE(expression);
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    std::string csd =
      "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=44100\nksmps=32\nnchnls=1\n"
      "instr 1\nchnset p4,\"result\"\nendin\n"
      "</CsInstruments>\n<CsScore>\ni 1 0 .01 [" + expression +
      "]\n</CsScore>\n</CsoundSynthesizer>\n";
    std::string path;
    if (mode == 0) {
#ifdef _WIN32
        int processId = _getpid();
#else
        int processId = getpid();
#endif
        path = ::testing::TempDir() + "csound-score-expression-" +
               std::to_string(processId) + ".csd";
        std::ofstream file(path, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        file << csd;
        file.close();
        ASSERT_TRUE(file.good());
    }
    int result = csoundCompileCSD(csound,
                                 mode == 0 ? path.c_str() : csd.c_str(), mode, 0);
    if (!path.empty()) {
        EXPECT_EQ(0, std::remove(path.c_str()));
    }
    if (error) {
        EXPECT_NE(CSOUND_SUCCESS, result);
        std::string messages;
        while (csoundGetMessageCnt(csound)) {
            messages += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        EXPECT_NE(std::string::npos, messages.find(error)) << messages;
    }
    else {
        ASSERT_EQ(CSOUND_SUCCESS, result);
        ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
        ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
        int channelError = 0;
        EXPECT_EQ(expected, csoundGetControlChannel(csound, "result", &channelError));
        EXPECT_EQ(0, channelError);
    }
}

TEST(ScoreExpressionTests, PreservesArithmeticAndGrouping)
{
    for (const auto& item : {
      std::pair<const char*, cs_float>{"110+220", 330},
      {"330-55", 275}, {"44*10", 440}, {"1100/2", 550},
      {"5^4", 625}, {"5660%1000", 660}, {"110&220", 76},
      {"110|220", 254}, {"110#220", 178}, {"8/2*3", 12},
      {"4+3-2+1", 6}, {"4+3*2+1", 11}, {"(4+3)*(2+1)", 21},
      {"2*2&3", 4}, {"3&2*2", 0}, {"1&1+2", 3}, {"4|3*3", 13},
      {"2^3^2", 512}, {"-2+3", 1}, {"1e+2/4", 25},
      {"@11", 16}, {"@@11", 17}, {"[2+3]*[4-1]", 15},
      {"~*0+1", 1}}) {
        checkExpression(item.first, item.second);
    }
}

TEST(ScoreExpressionTests, HandlesFileInputAndScoreErrors)
{
    checkExpression("2+3", 5, nullptr, 0);
    checkExpression("2^", 0, "missing operand before closing bracket", 0);
}

TEST(ScoreExpressionTests, AcceptsNumberBufferBoundary)
{
    checkExpression(std::string(98, '0') + "1", 1);
    checkExpression("-" + std::string(97, '0') + "1", -1);
    checkExpression("1e+" + std::string(95, '0') + "1", 10);
}

TEST(ScoreExpressionTests, RejectsNumbersBeyondBufferCapacity)
{
    for (const auto& number : {
      std::string(99, '0') + "1",
      "0." + std::string(97, '0') + "1",
      "1e" + std::string(97, '0') + "1",
      "1e+" + std::string(96, '0') + "1"}) {
        checkExpression(number, 0, "number too long in score expression");
    }
}

TEST(ScoreExpressionTests, ChecksOperatorStackCapacity)
{
    checkExpression(std::string(29, '(') + "1" + std::string(29, ')'), 1);
    checkExpression(std::string(30, '(') + "1" + std::string(30, ')'), 0,
                    "score expression operator stack full");
    std::string powers = "1";
    for (int i = 0; i < 29; ++i) powers += "^1";
    checkExpression(powers, 1);
    checkExpression(powers + "^1", 0, "score expression operator stack full");
}

TEST(ScoreExpressionTests, BoundsNestedSquareBrackets)
{
    checkExpression(std::string(29, '[') + "1" + std::string(29, ']'), 1);
    checkExpression(std::string(30, '[') + "1" + std::string(30, ']'), 0,
                    "score expression nested too deeply");
}

TEST(ScoreExpressionTests, RejectsMissingPowerOperands)
{
    for (const char* expression : {"^2", "1^^2", "(^2)", "1+(^2)"})
        checkExpression(expression, 0, "missing operand before '^'");
    checkExpression("2^", 0, "missing operand before closing bracket");
}

TEST(ScoreExpressionTests, BoundsPowerOfTwoIntegerAccumulation)
{
    checkExpression("@2147483647", 67108864);
    checkExpression("@@2147483647", FL(67108865.0));
    checkExpression("@2147483648", 0, "integer overflow in @ expression");
    checkExpression("@@2147483648", 0, "integer overflow in @ expression");
}

TEST(ScorePipelineTests, BareCInScorePreservesMAndHex)
{
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
    csoundSetOption(csound, "-n -d -m0");
    const char* csd =
      "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=44100\nksmps=32\nnchnls=1\n"
      "gkCount init 0\n"
      "instr 1\n"
      "gkCount = i(gkCount) + 1\n"
      "chnset gkCount, \"count\"\n"
      "endin\n"
      "instr 2\n"
      "Sval strget p4\n"
      "chnset Sval, \"hexstr\"\n"
      "endin\n"
      "</CsInstruments>\n<CsScore>\n"
      "c column-1 comment should be ignored\n"
      "  c indented c comment should be ignored\n"
      "m chorus\n"
      "i 1 0 0.01\n"
      "s\n"
      "n chorus\n"
      "i 2 0 0.01 \"0x1c\"\n"
      "e\n"
      "</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileCSD(csound, csd, 1, 0));
    ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
    while (csoundPerformKsmps(csound) == 0) {
    }
    int err = 0;
    EXPECT_EQ(FL(2.0), csoundGetControlChannel(csound, "count", &err));
    EXPECT_EQ(0, err);
    char strbuf[32] = {};
    csoundGetStringChannel(csound, "hexstr", strbuf);
    EXPECT_STREQ("0x1c", strbuf);
}

TEST(ScorePipelineTests, RampAndExprampMismatchedPFieldsDoNotCrash)
{
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    const char* csd =
      "<CsoundSynthesizer>\n<CsInstruments>\n"
      "sr=44100\nksmps=32\nnchnls=1\n"
      "instr 1\n"
      "chnset p5, \"r5\"\n"
      "endin\n"
      "</CsInstruments>\n<CsScore>\n"
      "i 1 0 0.01 1\n"
      "i 1 0.02 0.01 1 >\n"
      "i 1 0.04 0.01 2\n"
      "i 1 0.06 0.01 2 (\n"
      "i 1 0.08 0.01 3\n"
      "i 1 0.10 0.01 3 ~\n"
      "i 1 0.12 0.01 4\n"
      "e\n"
      "</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileCSD(csound, csd, 1, 0));
    ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
    while (csoundPerformKsmps(csound) == 0) {
    }
    int err = 0;
    EXPECT_EQ(FL(0.0), csoundGetControlChannel(csound, "r5", &err));
    EXPECT_EQ(0, err);
}

TEST(ScorePipelineTests, ExtractPreservesOnbeatOffbeatAndRejectsOutOfRangeInsno)
{
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
#ifdef _WIN32
    int processId = _getpid();
#else
    int processId = getpid();
#endif
    std::string xfile = ::testing::TempDir() + "csound-extract-" +
                        std::to_string(processId) + ".xtr";
    {
        std::ofstream f(xfile, std::ios::binary);
        ASSERT_TRUE(f.is_open());
        f << "f 2:2.0\nt 2:5.0\ni 1\n";
    }
    EXTRACT_STATICS ex{};
    FILE* xfp = std::fopen(xfile.c_str(), "r");
    ASSERT_NE(nullptr, xfp);
    readxfil(csound, &ex, xfp);
    std::fclose(xfp);
    EXPECT_EQ(0, std::remove(xfile.c_str()));

    EXPECT_EQ(FL(2.0), ex.a0.p3val);
    EXPECT_EQ(FL(5.0), ex.f0.p2val);

    SRTBLK s1End{};
    s1End.text[0] = 's';
    s1End.text[1] = '\n';
    csound->frstbp = &s1End;
    extract(csound, &ex);
    EXPECT_EQ(FL(2.0), ex.a0.p3val);
    EXPECT_EQ(FL(5.0), ex.f0.p2val);

    SRTBLK blkNeg{};
    blkNeg.insno = -1;
    blkNeg.p2val = blkNeg.newp2 = FL(2.5);
    blkNeg.p3val = blkNeg.newp3 = FL(1.0);
    blkNeg.text[0] = 'i';
    SRTBLK blkBig{};
    blkBig.insno = INSMAX + 5;
    blkBig.p2val = blkBig.newp2 = FL(2.5);
    blkBig.p3val = blkBig.newp3 = FL(1.0);
    blkBig.text[0] = 'i';
    SRTBLK blkValid{};
    blkValid.insno = 1;
    blkValid.p2val = blkValid.newp2 = FL(2.5);
    blkValid.p3val = blkValid.newp3 = FL(1.0);
    blkValid.text[0] = 'i';
    SRTBLK blkEnd{};
    blkEnd.text[0] = 'e';

    blkNeg.nxtblk = &blkBig;
    blkBig.prvblk = &blkNeg;
    blkBig.nxtblk = &blkValid;
    blkValid.prvblk = &blkBig;
    blkValid.nxtblk = &blkEnd;
    blkEnd.prvblk = &blkValid;
    csound->frstbp = &blkNeg;

    extract(csound, &ex);
    bool sawNeg = false, sawBig = false, sawValid = false;
    for (SRTBLK* cur = csound->frstbp; cur != nullptr; cur = cur->nxtblk) {
        if (cur == &blkNeg) sawNeg = true;
        if (cur == &blkBig) sawBig = true;
        if (cur == &blkValid) sawValid = true;
    }
    EXPECT_FALSE(sawNeg);
    EXPECT_FALSE(sawBig);
    EXPECT_TRUE(sawValid);
    EXPECT_EQ(FL(2.0), ex.a0.p3val);
    EXPECT_EQ(FL(5.0), ex.f0.p2val);
    csound->frstbp = nullptr;
}

TEST(ScorePipelineTests, RdscorUnterminatedStringDoesNotHang)
{
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
    csoundCreateMessageBuffer(csound, 0);
    csound->scstr = corfile_create_r(csound, "i 1 0 1 \"unterminated");
    EVTBLK evt{};
    int32_t rc = rdscor(csound, &evt);
    EXPECT_EQ(1, rc);
    EXPECT_EQ('i', evt.opcod);
    EXPECT_GE(evt.pcnt, 3);
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(std::string::npos, messages.find("unterminated string in scoreline"))
      << messages;
    csound->Free(csound, evt.p);
    csound->Free(csound, evt.strarg);

    EVTBLK evt2{};
    EXPECT_EQ(0, rdscor(csound, &evt2));
    csound->Free(csound, evt2.p);
}

TEST(ScorePipelineTests, CorfileGetFltAtEofAndWithoutTrailingSpace)
{
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
    CORFIL* cf = corfile_create_r(csound, "42.5");
    ASSERT_NE(nullptr, cf);
    EXPECT_DOUBLE_EQ(42.5, (double)corfile_get_flt(cf));
    EXPECT_EQ(FL(0.0), corfile_get_flt(cf));
    corfile_rm(csound, &cf);
}

TEST(ScorePipelineTests, SrtScoreFileLoading)
{
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
    csoundSetOption(csound, "-n -d -m0");
#ifdef _WIN32
    int processId = _getpid();
#else
    int processId = getpid();
#endif
    std::string orcPath = ::testing::TempDir() + "csound-srt-test-" +
                          std::to_string(processId) + ".orc";
    std::string srtPath = ::testing::TempDir() + "csound-srt-test-" +
                          std::to_string(processId) + ".srt";
    {
        std::ofstream orc(orcPath, std::ios::binary);
        ASSERT_TRUE(orc.is_open());
        orc << "sr=44100\nksmps=32\nnchnls=1\n"
               "instr 1\nchnset p4, \"srtval\"\nendin\n";
    }
    {
        std::ofstream srt(srtPath, std::ios::binary);
        ASSERT_TRUE(srt.is_open());
        srt << "w 0 60\ni 1 0 0 0.05 0.05 77\ne\n";
    }
    const char* argv[] = {"csound", orcPath.c_str(), srtPath.c_str()};
    int rc = csoundCompile(csound, 3, argv);
    EXPECT_EQ(0, std::remove(orcPath.c_str()));
    EXPECT_EQ(0, std::remove(srtPath.c_str()));
    ASSERT_EQ(CSOUND_SUCCESS, rc);
    ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
    ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
    int err = 0;
    EXPECT_EQ(FL(77.0), csoundGetControlChannel(csound, "srtval", &err));
    EXPECT_EQ(0, err);
}

TEST(ScorePipelineTests, CheckVersionAcceptsCurrentVersion)
{
    std::unique_ptr<CSOUND, decltype(&csoundDestroy)> engine(
      csoundCreate(nullptr, nullptr), csoundDestroy);
    CSOUND* csound = engine.get();
    csoundSetOption(csound, "-n -d -m0");
    const char* csd =
      "<CsoundSynthesizer>\n"
      "<CsVersion>\n7.00\nLater 7.00\nAfter 1.00\nBefore 99.00\n</CsVersion>\n"
      "<CsInstruments>\nsr=44100\nksmps=32\nnchnls=1\n</CsInstruments>\n"
      "<CsScore>\ne\n</CsScore>\n</CsoundSynthesizer>\n";
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileCSD(csound, csd, 1, 0));
}
} // namespace
