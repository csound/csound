/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#define __BUILDING_LIBCSOUND


#include <stdio.h>
#include <stdlib.h>
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <cstring>
#include <cmath>
#include <limits>
#include <tuple>
#include <vector>
extern "C" {
#include "str_ops.h"
}

#define csoundCompileOrc(a,b) csoundCompileOrc(a,b,0)
#define csoundReadScore(a,b) csoundEventString(a,b,0)

class OrcCompileTests : public ::testing::Test {
public:
    OrcCompileTests ()
    {
    }

    virtual ~OrcCompileTests ()
    {
    }

    virtual void SetUp ()
    {
        csound = csoundCreate (NULL,NULL);
        //csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "-odac --logfile=null");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

extern "C" {
    extern int32_t args_required (const char* arrayName);
    extern char** split_args (CSOUND* csound, const char* argString);
    extern OENTRY* find_opcode_new (CSOUND* csound, const char* opname,
                                    const char* outArgsFound,
                                    const char* inArgsFound);
}

TEST_F (OrcCompileTests, testSpaceTrajectoryGuardPoint)
{
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    const char *orc = R"(
sr = 48000
ksmps = 1
nchnls = 4
0dbfs = 1
giPos ftgen 1, 0, 4, -2, -1, -1, 1, 1
instr 1
  asig = 1
  a1, a2, a3, a4 space asig, 1, .01, 0, 0, 0
  kdist spdist 1, .01, 0, 0
  chnset kdist, "distance"
  out a1, a2, a3, a4
endin
)";
    ASSERT_EQ(0, csoundCompileOrc(csound, orc));
    csoundReadScore(csound, "i 1 0 .01\n");
    ASSERT_EQ(0, csoundStart(csound));
    MYFLT *table = nullptr;
    ASSERT_EQ(4, csoundGetTable(csound, &table, 1));
    // Even a zero interpolation weight must not read beyond the last pair.
    table[4] = std::numeric_limits<MYFLT>::quiet_NaN();
    csoundPerformKsmps(csound);
    ASSERT_EQ(0, csoundErrCnt(csound));
    const MYFLT *output = csoundGetSpout(csound);
    EXPECT_NEAR(0, output[0], 1e-6);
    EXPECT_NEAR(1, output[1], 1e-6);
    EXPECT_NEAR(0, output[2], 1e-6);
    EXPECT_NEAR(0, output[3], 1e-6);
    int32_t error = 0;
    EXPECT_NEAR(std::sqrt(2.0),
                csoundGetControlChannel(csound, "distance", &error), 1e-6);
    EXPECT_EQ(0, error);
    csoundDestroyMessageBuffer(csound);
}

class SpaceTrajectoryErrorTests : public OrcCompileTests,
    public ::testing::WithParamInterface<std::tuple<const char *, int>> {};

TEST_P(SpaceTrajectoryErrorTests, ReportsInvalidInput)
{
    const char *opcode = std::get<0>(GetParam());
    int invalid = std::get<1>(GetParam());
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "-n -d -m0");
    std::string orc = "sr = 48000\nksmps = 1\nnchnls = 1\n0dbfs = 1\n"
                      "chn_k \"time\", 1\n"
                      "giPos ftgen 1, 0, ";
    orc += invalid == 0 ? "1, -2, 1\n" : "4, -2, -1, -1, 1, 1\n";
    orc += "instr 1\nktime chnget \"time\"\n";
    if (std::strcmp(opcode, "space") == 0)
        orc += "asig = 1\na1, a2, a3, a4 space asig, 1, ktime, 0, 0, 0\n";
    else
        orc += "kdist spdist 1, ktime, 0, 0\n";
    orc += "endin\n";
    ASSERT_EQ(0, csoundCompileOrc(csound, orc.c_str()));
    MYFLT time = invalid == 1 ? std::numeric_limits<MYFLT>::quiet_NaN() :
                 invalid == 2 ? std::numeric_limits<MYFLT>::infinity() : 0;
    csoundSetControlChannel(csound, "time", time);
    csoundReadScore(csound, "i 1 0 .01\n");
    ASSERT_EQ(0, csoundStart(csound));
    csoundPerformKsmps(csound);
    EXPECT_EQ(1, csoundErrCnt(csound));
    std::string messages;
    while (csoundGetMessageCnt(csound)) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(std::string::npos, messages.find(invalid == 0 ?
              "trajectory table must contain an xy pair" :
              "trajectory time must be finite"));
    csoundDestroyMessageBuffer(csound);
}

INSTANTIATE_TEST_SUITE_P(
    SpaceAndSpdist, SpaceTrajectoryErrorTests,
    ::testing::Combine(::testing::Values("space", "spdist"),
                       ::testing::Values(0, 1, 2)));

class OLABufferTests : public OrcCompileTests,
                      public ::testing::WithParamInterface<MYFLT> {};

TEST_P(OLABufferTests, ValidatesOverlapBeforeIntegerArithmetic)
{
    const MYFLT overlap = GetParam();
    const bool valid = overlap == 1 || overlap == 2 || overlap == 4;
    const char *instrument = R"(
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
instr 1
  kFrame[] fillarray 1, 1, 1, 1, 1, 1, 1, 1
  iOverlap chnget "overlap"
  aOut olabuffer kFrame, iOverlap
  out aOut
endin
instr 2
  kCount init 0
  kCount += 1
  chnset kCount, "alive"
endin
schedule(1, 0, 0.001)
schedule(2, 0, 0.001)
)";

    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(csoundCompileOrc(csound, instrument), CSOUND_SUCCESS);
    csoundSetControlChannel(csound, "overlap", overlap);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    if (valid) {
        // Once all frames contain ones, overlap-add must sum to the factor.
        for (int sample = 0; sample < 24; ++sample) {
            ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
            if (sample >= 8)
                EXPECT_EQ(csoundGetSpout(csound)[0], overlap);
        }
    }
    else {
        // Reject this instrument while the other one keeps performing.
        for (int sample = 0; sample < 24; ++sample)
            ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        EXPECT_EQ(csoundErrCnt(csound), 1);
        int error = 0;
        EXPECT_EQ(csoundGetControlChannel(csound, "alive", &error), FL(24.0));
        EXPECT_EQ(error, 0);
        bool reportedOverlapError = false;
        while (csoundGetMessageCnt(csound)) {
            const char *message = csoundGetFirstMessage(csound);
            if (message && strstr(message, "olabuffer: Error,"))
                reportedOverlapError = true;
            csoundPopFirstMessage(csound);
        }
        EXPECT_TRUE(reportedOverlapError);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapFactors, OLABufferTests,
    ::testing::Values(FL(0.0), FL(-1.0), FL(0.5), FL(3.0), FL(8.0),
                      FL(2147483648.0), std::numeric_limits<MYFLT>::infinity(),
                      -std::numeric_limits<MYFLT>::infinity(),
                      std::numeric_limits<MYFLT>::quiet_NaN(),
                      FL(1.0), FL(2.0), FL(4.0)));

struct ScanhammerCase {
    const char *name;
    int source;
    int destination;
    MYFLT position;
    MYFLT gain;
    bool valid;
    std::vector<MYFLT> expected;
};

class ScanhammerTests : public OrcCompileTests,
                       public ::testing::WithParamInterface<ScanhammerCase> {};

TEST_P(ScanhammerTests, CopiesWithinDestinationBounds)
{
    const auto &test = GetParam();
    const char *instrument = R"(
sr = 48000
ksmps = 1
nchnls = 1
giSource ftgen 1, 0, 3, -2, 10, 20, 123
giDestination ftgen 2, 0, 5, -2, 1, 2, 3, 4, 99
instr 1
  iSource chnget "source"
  iDestination chnget "destination"
  iPosition chnget "position"
  iGain chnget "gain"
  scanhammer iSource, iDestination, iPosition, iGain
endin
schedule(1, 0, 0.001)
)";

    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(csoundCompileOrc(csound, instrument), CSOUND_SUCCESS);
    csoundSetControlChannel(csound, "source", test.source);
    csoundSetControlChannel(csound, "destination", test.destination);
    csoundSetControlChannel(csound, "position", test.position);
    csoundSetControlChannel(csound, "gain", test.gain);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    EXPECT_EQ(csoundErrCnt(csound), test.valid ? 0 : 1);

    // Check every point, including untouched values and the separate guard.
    // Invalid input must leave the existing destination unchanged.
    MYFLT *table = nullptr;
    ASSERT_EQ(csoundGetTable(csound, &table, 2),
              static_cast<int>(test.expected.size()) - 1);
    ASSERT_NE(table, nullptr);
    for (size_t index = 0; index < test.expected.size(); ++index)
        EXPECT_EQ(table[index], test.expected[index]) << "index " << index;
}

INSTANTIATE_TEST_SUITE_P(
    TableCopies, ScanhammerTests,
    ::testing::Values(
        ScanhammerCase{"Wrap", 1, 2, 3, 1, true, {20, 2, 3, 10, 99}},
        ScanhammerCase{"RoundedEnd", 1, 2, FL(3.75), 1, true,
                       {20, 2, 3, 10, 99}},
        ScanhammerCase{"InPlaceWrap", 2, 2, 1, 1, true, {4, 1, 2, 3, 99}},
        ScanhammerCase{"MissingSource", 99, 2, 0, 1, false, {1, 2, 3, 4, 99}},
        ScanhammerCase{"MissingDestination", 1, 99, 0, 1, false, {1, 2, 3, 4, 99}},
        ScanhammerCase{"NegativePosition", 1, 2, -1, 1, false, {1, 2, 3, 4, 99}},
        ScanhammerCase{"PositionAtLength", 1, 2, 4, 1, false, {1, 2, 3, 4, 99}},
        ScanhammerCase{"NaNPosition", 1, 2, std::numeric_limits<MYFLT>::quiet_NaN(),
                       1, false, {1, 2, 3, 4, 99}}),
    [](const ::testing::TestParamInfo<ScanhammerCase> &info) {
        return info.param.name;
    });

TEST_F (OrcCompileTests, testArgsRequired)
{
    ASSERT_EQ (1, args_required("a"));
    ASSERT_EQ (2, args_required("ka"));
    ASSERT_EQ (3, args_required("kak"));
    ASSERT_EQ (2, args_required("ak"));
    ASSERT_EQ (3, args_required("a[]ka"));
    ASSERT_EQ (4, args_required("a[]k[]ka"));
    ASSERT_EQ (4, args_required("a[][]k[][]ka"));
    ASSERT_EQ (0, args_required(NULL));
}

TEST_F (OrcCompileTests, testSplitArgs)
{
    char** results = split_args(csound, "kak");

    ASSERT_STREQ ("k", results[0]);
    ASSERT_STREQ ("a", results[1]);
    ASSERT_STREQ ("k", results[2]);
    csound->Free(csound, results);

    results = split_args(csound, "a[]k[]ka");

    ASSERT_STREQ ("[a]", results[0]);
    ASSERT_STREQ ("[k]", results[1]);
    ASSERT_STREQ ("k", results[2]);
    ASSERT_STREQ ("a", results[3]);
    csound->Free(csound, results);

    results = split_args(csound, "a[][]k[][]ka");

    ASSERT_STREQ ("[[a]", results[0]);
    ASSERT_STREQ ("[[k]", results[1]);
    ASSERT_STREQ ("k", results[2]);
    ASSERT_STREQ ("a", results[3]);
    csound->Free(csound, results);
}

TEST_F (OrcCompileTests, testMutuallyRecursiveUdoRateInference)
{
    const char* orchestra = R"(
struct RateValue value:i, values:i[]
rateValues@global:RateValue[] init 1

declare RateInitEven(depth:i):(i)
declare RateInitOdd(depth:i):(i)
declare RatePerfEven(depth:i):(i)
declare RatePerfOdd(depth:i):(i)

opcode RateInitEven(depth:i):i
  result:i init 1
  switch depth
    case 0
    default
      result = RateInitOdd(depth - 1)
  endsw
  xout result
endop

opcode RateInitOdd(depth:i):i
  result:i init 1
  if (depth > 0) ithen
    result = RateInitEven(depth - 1)
  endif
  xout result
endop

opcode RateInitRead(index:i):i
  value:RateValue init rateValues[index]
  result:i init value.values[0]
  xout result
endop

opcode RateInitWhile(index:i):i
  result:i init 0
  ; Keep the member read nested so loop expansion must preserve init context.
  while ((rateValues[index].value + 0) < 1) do
    result += 1
    break
  od
  xout result
endop

opcode RatePerfWhile(index:k):i
  result:i init 0
  while ((rateValues[index].value + 0) < 1) do
    result += 1
    break
  od
  xout result
endop

opcode RatePerfEven(depth:i):i
  result:i init 1
  if (depth > 0) ithen
    result = RatePerfOdd(depth - 1)
  endif
  xout result
endop

opcode RatePerfOdd(depth:i):i
  result:i init 1
  printks "", 1
  if (depth > 0) ithen
    result = RatePerfEven(depth - 1)
  endif
  xout result
endop
)";

    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra));

    OENTRY* initEven = find_opcode_new(csound, "RateInitEven", "i", "i");
    OENTRY* initOdd = find_opcode_new(csound, "RateInitOdd", "i", "i");
    OENTRY* initRead = find_opcode_new(csound, "RateInitRead", "i", "i");
    OENTRY* initWhile = find_opcode_new(csound, "RateInitWhile", "i", "i");
    OENTRY* perfWhile = find_opcode_new(csound, "RatePerfWhile", "i", "k");
    OENTRY* perfEven = find_opcode_new(csound, "RatePerfEven", "i", "i");
    OENTRY* perfOdd = find_opcode_new(csound, "RatePerfOdd", "i", "i");

    ASSERT_NE(nullptr, initEven);
    ASSERT_NE(nullptr, initOdd);
    ASSERT_NE(nullptr, initRead);
    ASSERT_NE(nullptr, initWhile);
    ASSERT_NE(nullptr, perfWhile);
    ASSERT_NE(nullptr, perfEven);
    ASSERT_NE(nullptr, perfOdd);
    EXPECT_EQ(nullptr, initEven->perf);
    EXPECT_EQ(nullptr, initOdd->perf);
    EXPECT_EQ(nullptr, initRead->perf);
    EXPECT_EQ(nullptr, initWhile->perf);
    EXPECT_NE(nullptr, perfWhile->perf);
    EXPECT_NE(nullptr, perfEven->perf);
    EXPECT_NE(nullptr, perfOdd->perf);
}


TEST_F (OrcCompileTests, testInitOnlyGeneratedGetterConsumers)
{
    const char* orchestra = R"(
struct GetterValue text:S, children:GetterValue[]
getterValues@global:GetterValue[] init 1
getterStrings@global:S[] fillarray "hello"
getterNumbers@global:k[] init 1

opcode GetterLength(input:S):i
  xout strlen(input)
endop
opcode GetterDiscard(input:S):void
  length:i = strlen(input)
endop
opcode GetterMember(index:i):i
  result:i = GetterLength(getterValues[index].text)
  xout result
endop
opcode GetterString(index:i):void
  GetterDiscard(getterStrings[index])
endop
opcode GetterChild(index:i):GetterValue
  xout getterValues[index].children[0]
endop
opcode GetterChildExplicit(index:i):GetterValue
  result:GetterValue init getterValues[index].children[0]
  xout result
endop
opcode GetterPerf(input:S):k
  result:k = strlenk(input)
  xout result
endop
opcode GetterPerfMember(index:i):k
  result:k = GetterPerf(getterValues[index].text)
  xout result
endop
opcode GetterPerfString(index:i):k
  result:k = GetterPerf(getterStrings[index])
  xout result
endop
opcode GetterPerfNumber(index:i):k
  xout getterNumbers[index]
endop
opcode GetterOutput(index:i):S
  xout getterValues[index].text
endop
)";

    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra));
    const char* initNames[] = {"GetterMember", "GetterString", "GetterChild",
                              "GetterChildExplicit"};
    const char* initOutputs[] = {"i", "", ":GetterValue;", ":GetterValue;"};
    for (int i = 0; i < 4; ++i) {
        OENTRY* entry = find_opcode_new(csound, initNames[i], initOutputs[i], "i");
        ASSERT_NE(nullptr, entry) << initNames[i];
        EXPECT_EQ(nullptr, entry->perf) << initNames[i];
    }
    const char* perfNames[] = {"GetterPerfMember", "GetterPerfString",
                              "GetterPerfNumber", "GetterOutput"};
    for (int i = 0; i < 4; ++i) {
        OENTRY* entry = find_opcode_new(csound, perfNames[i], i == 3 ? "S" : "k", "i");
        ASSERT_NE(nullptr, entry) << perfNames[i];
        EXPECT_NE(nullptr, entry->perf) << perfNames[i];
    }
}

TEST_F (OrcCompileTests, testInitGetterRecursiveFrameReuse)
{
    const char* orchestra = R"(
struct BranchValue text:S
branchValues@global:BranchValue[] init 1
branch:BranchValue init "hello"
branchValues[0] init branch

opcode BranchLength(input:S):i
  xout strlen(input)
endop
opcode BranchRead():i
  result:i = BranchLength(branchValues[0].text)
  xout result
endop
opcode CountGetterBranches(depth:i):i
  length:i = BranchRead()
  result:i = length
  if (depth > 0) then
    left:i = CountGetterBranches(depth - 1)
    right:i = CountGetterBranches(depth - 1)
    result = left + right
  endif
  xout result
endop
instr 1
  result:i = CountGetterBranches(12)
  chnset result, "branch-result"
endin
)";

    ASSERT_EQ(CSOUND_SUCCESS, csoundSetOption(csound, "-n -d -m0"));
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra));
    csoundReadScore(csound, "i 1 0 1\n");
    ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
    ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
    int32_t error = 0;
    EXPECT_EQ(20480, csoundGetControlChannel(csound, "branch-result", &error));
    EXPECT_EQ(CSOUND_SUCCESS, error);

    OENTRY* entry = find_opcode_new(csound, "CountGetterBranches", "i", "i");
    ASSERT_NE(nullptr, entry);
    OPCODINFO* info = (OPCODINFO*)entry->useropinfo;
    ASSERT_NE(nullptr, info);
    int instances = 0;
    for (INSDS* frame = info->ip->instance; frame != nullptr;
         frame = frame->nxtinstance) {
        ++instances;
    }
    // Completed branches reuse frames; only recursion depth sets the bound.
    EXPECT_LE(instances, 16);
}

TEST_F (OrcCompileTests, testCompile)
{
    int32_t result, compile_again = 0;
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    const char* instrument2 =
        "instr 2 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 vco2  k1, p5   \n"
        "out  a1   \n"
        "endin \n"
        "event_i \"i\",2, 0.5, 2, 10000, 800 \n";

    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE (result == 0);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");
    result = csoundStart(csound);
    ASSERT_TRUE (result == 0);

    while(!result)
    {
        result = csoundPerformKsmps(csound);

        if(!compile_again)
        {
            /* new compilation */
            csoundCompileOrc(csound, instrument2);
            /* schedule an event on instr2 */
            csoundReadScore(csound, "i2 1 1 10000 110 \n i2 + 1 1000 660");
            compile_again = 1;
        }
    }
}

TEST_F (OrcCompileTests, testNestedExpressionFailurePropagates)
{
    const char *instrument =
        "instr 1\n"
        "  iresult = abs(missing[0] + 1)\n"
        "endin\n";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

TEST_F (OrcCompileTests, testRejectsKRateIndexStructAggregateInit)
{
    const char *instrument = R"(
struct Box value:i

instr 1
  first:Box init 10
  second:Box init 20
  boxes:Box[] fillarray first, second
  index:k = 1
  copy:Box init boxes[index]
endin
)";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

TEST_F (OrcCompileTests, testRejectsKRateExpressionStructAggregateInit)
{
    const char *instrument = R"(
struct Box value:i

instr 1
  box:Box init 37
  boxes:Box[] fillarray box
  index:k init 0
  copy:Box init boxes[index + 0]
endin
)";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

TEST_F (OrcCompileTests, testRejectsKInitializedOutputStructAggregateInit)
{
    const char *instrument = R"(
struct Box value:i

opcode InitializedIndex, K, 0
  index:k init 1
  xout index
endop

instr 1
  first:Box init 10
  second:Box init 20
  boxes:Box[] fillarray first, second
  copy:Box init boxes[InitializedIndex()]
endin
)";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

TEST_F (OrcCompileTests, testRejectsKRateStructMemberAggregateInit)
{
    const char *instrument = R"(
struct Box value:i
struct Selection index:k

instr 1
  box:Box init 37
  boxes:Box[] fillarray box
  selection:Selection init 0
  copy:Box init boxes[selection.index]
endin
)";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

TEST_F (OrcCompileTests, testRejectsKArrayElementStructAggregateInit)
{
    const char *instrument = R"(
struct Box value:i

instr 1
  box:Box init 37
  boxes:Box[] fillarray box
  indices:k[] fillarray 0
  index:i init 0
  copy:Box init boxes[indices[index]]
endin
)";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

TEST_F (OrcCompileTests, testRejectsKRateIndexNestedStructAggregateInit)
{
    const char *instrument = R"(
struct Inner value:i
struct Outer inner:Inner

instr 1
  inner:Inner init 37
  outer:Outer init inner
  outers:Outer[] fillarray outer
  index:k init 0
  copy:Inner init outers[index].inner
endin
)";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

TEST_F (OrcCompileTests, testRejectsKRateIndexInitConsumer)
{
    const char *instrument = R"(
struct Box value:i

opcode PrintAt(items:Box[], index:k):void
  printfi "VALUE=%f\n", 1, items[index].value
endop

instr 1
  box:Box init 37
  boxes:Box[] fillarray box
  index:k init 0
  PrintAt(boxes, index)
endin
)";

    ASSERT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, instrument));
}

class StructInitConditionTests
    : public OrcCompileTests,
      public ::testing::WithParamInterface<const char *> {};

TEST_P (StructInitConditionTests, rejectsPerformanceRead)
{
    std::string orchestra = R"(
struct Box value:i
instr 1
  box:Box init 37
  boxes:Box[] fillarray box
  index:k init 0
)";
    orchestra += GetParam();
    orchestra += "\nendin\n";
    csoundCreateMessageBuffer(csound, 0);
    EXPECT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra.c_str()));
    std::string messages;
    while (csoundGetMessageCnt(csound) > 0) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(std::string::npos,
              messages.find("struct array index must be i-rate during init"));
    csoundDestroyMessageBuffer(csound);
}

TEST_P (StructInitConditionTests, acceptsInitRead)
{
    std::string orchestra = R"(
struct Box value:i
instr 1
  box:Box init 37
  boxes:Box[] fillarray box
  index:i init 0
)";
    orchestra += GetParam();
    orchestra += "\nendin\n";
    EXPECT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra.c_str()));
}

INSTANTIATE_TEST_SUITE_P(
    Expressions, StructInitConditionTests,
    ::testing::Values(
        "if ((boxes[index].value + 0) > 0) ithen\n prints \"yes\"\n endif",
        "if (0 < (boxes[index].value + 0)) ithen\n prints \"yes\"\n endif",
        "if (((boxes[index].value + 0) > 0) && (1 < 2)) ithen\n"
        " prints \"yes\"\n endif",
        "if ((1 < 2) && ((boxes[index].value + 0) > 0)) ithen\n"
        " prints \"yes\"\n endif",
        "value:i = (((boxes[index].value + 0) > 0) ? 1 : 2)",
        "if ((boxes[index].value + 0) > 0) igoto done\n done:",
        "if (1 > 2) ithen\n prints \"no\"\n"
        " elseif ((boxes[index].value + 0) > 0) ithen\n prints \"yes\"\n endif"));

TEST_F (OrcCompileTests, testRejectsPerformanceStructReadByInitUdo)
{
    const char *orchestra = R"(
struct Box value:i
declare PrintBox(item:Box):()

instr 1
  box:Box init 37
  boxes:Box[] fillarray box
  index:k init 0
  PrintBox boxes[index]
endin

opcode PrintBox(item:Box):void
  printfi "VALUE=%f\n", 1, item.value
endop
)";

    csoundCreateMessageBuffer(csound, 0);
    EXPECT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra));
    std::string messages;
    while (csoundGetMessageCnt(csound) > 0) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(std::string::npos,
              messages.find("init-only UDO PrintBox cannot take a "
                            "performance-only struct array read"));
    csoundDestroyMessageBuffer(csound);
}

TEST_F (OrcCompileTests, testRejectsNestedPerformanceStructReadByInitUdo)
{
    const char *orchestra = R"(
struct Box value:i
struct Holder item:Box
opcode PrintBox(item:Box):void
  printfi "VALUE=%f\n", 1, item.value
endop

instr 1
  box:Box init 37
  holder:Holder init box
  holders:Holder[] fillarray holder
  index:k init 0
  PrintBox holders[index].item
endin
)";

    EXPECT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra));
}

class StringArrayInitConsumerTests
    : public OrcCompileTests,
      public ::testing::WithParamInterface<const char *> {};

TEST_P (StringArrayInitConsumerTests, rejectsPerformanceRead)
{
    std::string orchestra =
        "declare PrintWord(word:S):()\n"
        "instr 1\n"
        "  Swords[] fillarray \"first\", \"changed\"\n"
        "  kIndex init 1\n"
        "  ";
    orchestra += GetParam();
    orchestra += "\nendin\n"
                 "opcode PrintWord(word:S):void\n"
                 "  prints \"%s\", word\n"
                 "endop\n";
    csoundCreateMessageBuffer(csound, 0);
    EXPECT_NE(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra.c_str()));
    std::string messages;
    while (csoundGetMessageCnt(csound) > 0) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    EXPECT_NE(std::string::npos, messages.find("performance-only string array read"));
    EXPECT_NE(std::string::npos, messages.find("i(kIndex)"));
    EXPECT_NE(std::string::npos, messages.find("performance-time consumer"));
    EXPECT_NE(std::string::npos, messages.find("line 5"));
    csoundDestroyMessageBuffer(csound);
}

TEST_P (StringArrayInitConsumerTests, acceptsExplicitInitRead)
{
    std::string statement = GetParam();
    size_t index = statement.find("[kIndex]");
    ASSERT_NE(std::string::npos, index);
    statement.replace(index, strlen("[kIndex]"), "[i(kIndex)]");
    std::string orchestra =
        "declare PrintWord(word:S):()\n"
        "instr 1\n"
        "  Swords[] fillarray \"first\", \"changed\"\n"
        "  kIndex init 1\n" + statement + "\nendin\n"
        "opcode PrintWord(word:S):void\n"
        "  prints \"%s\", word\n"
        "endop\n";
    EXPECT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra.c_str()));
}

INSTANTIATE_TEST_SUITE_P(
    StringArrays, StringArrayInitConsumerTests,
    ::testing::Values(
        "prints \"INIT=[%s]\\n\", Swords[kIndex]",
        "printfi \"INIT=[%s]\\n\", 1, Swords[kIndex]",
        "Scopy strcpy Swords[kIndex]",
        "Scopy = strcpy(Swords[kIndex])",
        "Scopy init Swords[kIndex]",
        "printf \"LENGTH=%d\\n\", 1, strlen(Swords[kIndex])",
        "PrintWord Swords[kIndex]"));

TEST_F (OrcCompileTests, testStringArrayInitAndPerformanceValues)
{
    const char *orchestra = R"(
sr = 48000
ksmps = 32
nchnls = 1
opcode PrintWord(word:S):void
  printf "UDO=[%s]\n", 1, word
endop
instr 1
  Swords[] fillarray "first", "changed"
  kIndex init 1
  prints "INIT=[%s]\n", Swords[i(kIndex)]
  Scopy strcpy Swords[i(kIndex)]
  prints "COPY=[%s]\n", Scopy
  Svalue = Swords[kIndex]
  printf "PERF=[%s]\n", kIndex + 1, Swords[kIndex]
  printf "ASSIGN=[%s]\n", kIndex + 1, Svalue
  PrintWord Swords[kIndex]
  printtype Swords[kIndex]
  kIndex = 0
endin
)";
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(CSOUND_SUCCESS, csoundSetOption(csound, "-n -d -m0"));
    ASSERT_EQ(CSOUND_SUCCESS, csoundCompileOrc(csound, orchestra));
    csoundReadScore(csound, "i 1 0 0.01\n");
    ASSERT_EQ(CSOUND_SUCCESS, csoundStart(csound));
    ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
    ASSERT_EQ(CSOUND_SUCCESS, csoundPerformKsmps(csound));
    std::string messages;
    while (csoundGetMessageCnt(csound) > 0) {
        messages += csoundGetFirstMessage(csound);
        csoundPopFirstMessage(csound);
    }
    for (const char *expected : {"INIT=[changed]", "COPY=[changed]",
                                "PERF=[changed]", "PERF=[first]",
                                "ASSIGN=[changed]", "ASSIGN=[first]",
                                "UDO=[changed]"}) {
        EXPECT_NE(std::string::npos, messages.find(expected)) << expected;
    }
    csoundDestroyMessageBuffer(csound);
}

TEST_F (OrcCompileTests, testReuse)
{
    int32_t result;
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE(result == 0);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n");
    result = csoundStart(csound);
    ASSERT_TRUE(result == 0);
    while(csoundPerformKsmps(csound) == 0);
    csoundReset(csound);
    result = csoundCompileOrc(csound, instrument);
    csoundRewindScore(csound);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n");

    while(csoundPerformKsmps(csound) == 0);
    csoundReset(csound);
    result = csoundCompileOrc(csound, instrument);
    csoundRewindScore(csound);
    csoundReadScore(csound,  "i 1 0  1 10000 5000\n i 1 3 1 10000 1000\n");

    while(csoundPerformKsmps(csound) == 0);
}

TEST_F (OrcCompileTests, testLineNumber)
{
    const char* instrument =
        "instr 1 \n"
        "k1 expon p4, p3, p4*0.001 \n"
        "a1 randi  k1, p5   \n"
        "out  a1   \n"
        "endin \n";

    TREE *tree = csoundParseOrc(csound, instrument);
    ASSERT_TRUE(tree != NULL);
}
#if 0
// Helper to dump AST tree for debugging column number tests
static void dump_tree(TREE *t, int depth) {
    if (!t) return;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("type=%d line=%d", t->type, t->line);
    if (t->value) {
        printf(" lexeme=\"%s\" cols=%u-%u",
               t->value->lexeme ? t->value->lexeme : "(null)",
               t->value->first_column, t->value->last_column);
    }
    printf("\n");
    if (t->left) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf(" LEFT:\n");
        dump_tree(t->left, depth + 1);
    }
    if (t->right) {
        for (int i = 0; i < depth; i++) printf("  ");
        printf(" RIGHT:\n");
        dump_tree(t->right, depth + 1);
    }
    if (t->next) {
        dump_tree(t->next, depth);
    }
}
#endif

// Test basic column number tracking on tokens
TEST_F (OrcCompileTests, testColumnNumbers)
{
    //          col: 123456789
    const char *orc = "instr 1\n"        // line 1: "1" at col 7
                      "k1 = 440\n"       // line 2: "k1" cols 1-2, "440" cols 6-8
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    // csoundParseOrc returns a root node (type=0);
    // the instr block is linked via ->next
    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);

    // LEFT child: instrument number "1"
    ASSERT_TRUE(instr->left != NULL);
    ASSERT_TRUE(instr->left->value != NULL);
    ASSERT_STREQ("1", instr->left->value->lexeme);
    ASSERT_EQ(7u, instr->left->value->first_column);
    ASSERT_EQ(7u, instr->left->value->last_column);

    // RIGHT child: the body "k1 = 440"
    TREE *body = instr->right;
    ASSERT_TRUE(body != NULL);

    // body LEFT: "k1" identifier
    ASSERT_TRUE(body->left != NULL);
    ASSERT_TRUE(body->left->value != NULL);
    ASSERT_STREQ("k1", body->left->value->lexeme);
    ASSERT_EQ(1u, body->left->value->first_column);
    ASSERT_EQ(2u, body->left->value->last_column);

    // body RIGHT: "440" number
    ASSERT_TRUE(body->right != NULL);
    ASSERT_TRUE(body->right->value != NULL);
    ASSERT_STREQ("440", body->right->value->lexeme);
    ASSERT_EQ(6u, body->right->value->first_column);
    ASSERT_EQ(8u, body->right->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test that column numbers reset correctly across multiple lines
TEST_F (OrcCompileTests, testColumnNumbersMultiLine)
{
    //          col: 123456789012345
    const char *orc = "instr 1\n"
                      "k1 = 100\n"       // line 2: "k1" cols 1-2
                      "k2 = 200\n"       // line 3: "k2" cols 1-2 (reset from prev line)
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // First statement: k1
    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("k1", stmt1->left->value->lexeme);
    ASSERT_EQ(1u, stmt1->left->value->first_column);
    ASSERT_EQ(2u, stmt1->left->value->last_column);

    // Second statement: k2 (via stmt1->next)
    TREE *stmt2 = stmt1->next;
    ASSERT_TRUE(stmt2 != NULL);
    ASSERT_TRUE(stmt2->left != NULL && stmt2->left->value != NULL);
    ASSERT_STREQ("k2", stmt2->left->value->lexeme);
    ASSERT_EQ(1u, stmt2->left->value->first_column);
    ASSERT_EQ(2u, stmt2->left->value->last_column);

    // Verify the values too
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_STREQ("100", stmt1->right->value->lexeme);
    ASSERT_EQ(6u, stmt1->right->value->first_column);
    ASSERT_EQ(8u, stmt1->right->value->last_column);

    ASSERT_TRUE(stmt2->right != NULL && stmt2->right->value != NULL);
    ASSERT_STREQ("200", stmt2->right->value->lexeme);
    ASSERT_EQ(6u, stmt2->right->value->first_column);
    ASSERT_EQ(8u, stmt2->right->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test column tracking through xstr ({{ }}) multi-line strings
// BUG: <xstr> newline rule does not reset yycolumn, so the column
// reported for the STRING_TOKEN (at the closing }}) is wrong.
TEST_F (OrcCompileTests, testColumnNumbersXstr)
{
    const char *orc = "instr 1\n"                 // line 1
                      "Sval = {{\n"                // line 2: Sval col 1-4
                      "hello\n"                    // line 3: inside string
                      "}}\n"                       // line 4: }} at cols 1-2
                      "k1 = 440\n"                 // line 5
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // First statement: Sval = {{ ... }}
    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("Sval", stmt1->left->value->lexeme);
    ASSERT_EQ(1u, stmt1->left->value->first_column);
    ASSERT_EQ(4u, stmt1->left->value->last_column);

    // The STRING_TOKEN (right child) should have columns of the
    // closing }} which is at columns 1-2 on its line.
    // BUG: without yycolumn reset in <xstr> newline, this will be 17-18
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_EQ(1u, stmt1->right->value->first_column);
    ASSERT_EQ(2u, stmt1->right->value->last_column);

    // Second statement: k1 = 440 (should be correct regardless,
    // since the newline after }} hits the INITIAL rule)
    TREE *stmt2 = stmt1->next;
    ASSERT_TRUE(stmt2 != NULL);
    ASSERT_TRUE(stmt2->left != NULL && stmt2->left->value != NULL);
    ASSERT_STREQ("k1", stmt2->left->value->lexeme);
    ASSERT_EQ(1u, stmt2->left->value->first_column);
    ASSERT_EQ(2u, stmt2->left->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test column tracking through rstr (R{ }R) multi-line strings.
// The <rstr> newline rule was already fixed with yycolumn = 1.
TEST_F (OrcCompileTests, testColumnNumbersRstr)
{
    //                                        R{ starts rstr
    const char *orc = "instr 1\n"                 // line 1
                      "Sval = R{\n"                // line 2: Sval col 1-4
                      "hello\n"                    // line 3: inside string
                      "}R\n"                       // line 4: }R at cols 1-2
                      "k1 = 440\n"                 // line 5
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // First statement: Sval = R{ ... }R
    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("Sval", stmt1->left->value->lexeme);

    // The STRING_TOKEN for rstr: }R at cols 1-2 on its line
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_EQ(1u, stmt1->right->value->first_column);
    ASSERT_EQ(2u, stmt1->right->value->last_column);

    // k1 after the rstr
    TREE *stmt2 = stmt1->next;
    ASSERT_TRUE(stmt2 != NULL);
    ASSERT_TRUE(stmt2->left != NULL && stmt2->left->value != NULL);
    ASSERT_STREQ("k1", stmt2->left->value->lexeme);
    ASSERT_EQ(1u, stmt2->left->value->first_column);
    ASSERT_EQ(2u, stmt2->left->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test that error messages contain column information
TEST_F (OrcCompileTests, testColumnNumbersInErrors)
{
    csoundCreateMessageBuffer(csound, 0);

    // Deliberate error: unknown opcode at a known column position
    //          col: 1234567890123456
    const char *orc = "instr 1\n"
                      "k1 badopcode 440\n"  // "badopcode" at cols 4-12
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Scan message buffer for column info
    bool found_column = false;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg && strstr(msg, "columns")) {
            found_column = true;
            printf("Error message with columns: %s\n", msg);
        }
        csoundPopFirstMessage(csound);
    }
    ASSERT_TRUE(found_column);
}

// Test that "unable to find opcode" error includes correct column range
TEST_F (OrcCompileTests, testErrorMsgUnknownOpcodeColumns)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 1234567890123456
    const char *orc = "instr 1\n"
                      "k1 badopcode 440\n"  // "badopcode" at cols 4-12
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Collect all messages into one string for easier matching
    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // verify_opcode emits "columns %d-%d" with the opcode token columns
    // "badopcode" starts at col 4, ends at col 12
    ASSERT_TRUE(allMessages.find("columns 4-12") != std::string::npos)
        << "Expected 'columns 4-12' in error output, got:\n" << allMessages;
}

// Test that syntax errors (parser-level) include column info
TEST_F (OrcCompileTests, testErrorMsgSyntaxErrorColumns)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 123456789
    const char *orc = "instr 1\n"
                      "k1 = +\n"  // syntax error: unexpected +
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Collect all messages
    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // csound_orcerror emits "columns %d,%d" (note: comma separator)
    ASSERT_TRUE(allMessages.find("columns") != std::string::npos)
        << "Expected 'columns' in syntax error output, got:\n" << allMessages;
}

// Test that error for wrong argument types includes column info
TEST_F (OrcCompileTests, testErrorMsgWrongArgsColumns)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 123456789012345678
    const char *orc = "instr 1\n"
                      "k1 oscil \"bad\"\n"  // wrong arg type for oscil
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Collect all messages
    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // Should contain column info from verify_opcode error path
    ASSERT_TRUE(allMessages.find("columns") != std::string::npos)
        << "Expected 'columns' in arg-mismatch error output, got:\n" << allMessages;
    // "oscil" at cols 4-8
    ASSERT_TRUE(allMessages.find("columns 4-8") != std::string::npos)
        << "Expected 'columns 4-8' for 'oscil' in error output, got:\n" << allMessages;
}

// Test that error columns work correctly with leading whitespace
TEST_F (OrcCompileTests, testErrorMsgColumnsWithIndentation)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 1234567890123456789012
    const char *orc = "instr 1\n"
                      "    k1 badopcode 440\n"  // "badopcode" at cols 8-16
                      "endin\n";

    csoundCompileOrc(csound, orc);

    // Collect all messages
    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // "badopcode" at cols 8-16 (4 spaces + "k1 " = 7, then badopcode at 8)
    ASSERT_TRUE(allMessages.find("columns 8-16") != std::string::npos)
        << "Expected 'columns 8-16' for indented 'badopcode', got:\n" << allMessages;
}

// Test that errors at different column offsets report the correct positions
TEST_F (OrcCompileTests, testErrorMsgDifferentColumnOffsets)
{
    csoundCreateMessageBuffer(csound, 0);

    //          col: 123456789012345678
    const char *orc = "instr 1\n"
                      "k1 bad1 440\n"       // "bad1" at cols 4-7
                      "endin\n";

    csoundCompileOrc(csound, orc);

    std::string allMessages;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages += msg;
        csoundPopFirstMessage(csound);
    }

    // "bad1" at cols 4-7
    ASSERT_TRUE(allMessages.find("columns 4-7") != std::string::npos)
        << "Expected 'columns 4-7' for 'bad1', got:\n" << allMessages;

    // Now try a different column offset in a fresh instance
    csoundReset(csound);
    csoundCreateMessageBuffer(csound, 0);

    //          col: 123456789012345678901
    const char *orc2 = "instr 1\n"
                       "k1 = 1\n"
                       "   k2 badop2 440\n"  // "badop2" at cols 7-12
                       "endin\n";

    csoundCompileOrc(csound, orc2);

    std::string allMessages2;
    while (csoundGetMessageCnt(csound)) {
        const char *msg = csoundGetFirstMessage(csound);
        if (msg) allMessages2 += msg;
        csoundPopFirstMessage(csound);
    }

    // "badop2" at cols 7-12
    ASSERT_TRUE(allMessages2.find("columns 7-12") != std::string::npos)
        << "Expected 'columns 7-12' for 'badop2', got:\n" << allMessages2;
}

// Test column tracking across line continuation (\)
// Preprocessor converts \<LF> to inline #sline N directive;
// the <sline> rule must reset yycolumn so subsequent tokens
// get correct source columns.
TEST_F (OrcCompileTests, testColumnNumbersContinuation)
{
    //                   col: 12345678
    const char *orc = "instr 1\n"
                      "k1 = \\\n"           // line 2: k1 cols 1-2, \ joins next line
                      "     440\n"           // line 3: 440 at cols 6-8
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("k1", stmt1->left->value->lexeme);
    ASSERT_EQ(1u, stmt1->left->value->first_column);
    ASSERT_EQ(2u, stmt1->left->value->last_column);

    // After \, the preprocessor emits #sline which resets yycolumn.
    // Five spaces of source indentation then advance to col 6.
    // "440" should be at columns 6-8.
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_STREQ("440", stmt1->right->value->lexeme);
    ASSERT_EQ(6u, stmt1->right->value->first_column);
    ASSERT_EQ(8u, stmt1->right->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test column tracking inside parentheses (parenthesis depth)
// Newlines inside parens should reset yycolumn but not return NEWLINE.
// Note: the parser expands (100 + 200) into an implicit ##add node
// with a temp #i0 variable, so the AST has 3 statements:
//   ##add(100, 200) -> #i0, k1 = #i0, k2 = 300
TEST_F (OrcCompileTests, testColumnNumbersIgnoreNewline)
{
    const char *orc = "instr 1\n"
                      "k1 = (100 +\n"      // line 2: ( increments parenthesis depth
                      " 200)\n"             // line 3: 200 at cols 2-4
                      "k2 = 300\n"          // line 4: k2 at cols 1-2
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // First node is the implicit ##add operation.
    // Its right children contain the operands:
    //   100 at cols 7-9 (line 2), 200 at cols 2-4 (line 3, after newline)
    TREE *add_node = instr->right;
    ASSERT_TRUE(add_node->right != NULL && add_node->right->value != NULL);
    ASSERT_STREQ("100", add_node->right->value->lexeme);
    ASSERT_EQ(7u, add_node->right->value->first_column);
    ASSERT_EQ(9u, add_node->right->value->last_column);

    // "200" is the next operand of ##add
    ASSERT_TRUE(add_node->right->next != NULL && add_node->right->next->value != NULL);
    ASSERT_STREQ("200", add_node->right->next->value->lexeme);
    ASSERT_EQ(2u, add_node->right->next->value->first_column);
    ASSERT_EQ(4u, add_node->right->next->value->last_column);

    // Third statement: k2 = 300
    TREE *k2_stmt = add_node->next->next;
    ASSERT_TRUE(k2_stmt != NULL);
    ASSERT_TRUE(k2_stmt->left != NULL && k2_stmt->left->value != NULL);
    ASSERT_STREQ("k2", k2_stmt->left->value->lexeme);
    ASSERT_EQ(1u, k2_stmt->left->value->first_column);
    ASSERT_EQ(2u, k2_stmt->left->value->last_column);

    csoundDeleteTree(csound, tree);
}

// Test column tracking for regular quoted strings ("...")
// Note: flex input() used for string scanning bypasses YY_USER_ACTION,
// so the token columns reflect the opening quote position only.
// Next-line tokens are still correct because newline resets yycolumn.
TEST_F (OrcCompileTests, testColumnNumbersQuotedString)
{
    const char *orc = "instr 1\n"
                      "S1 = \"hello\"\n"     // line 2: S1 cols 1-2, "hello" starts at col 6
                      "k1 = 440\n"            // line 3: k1 at cols 1-2 (after newline reset)
                      "endin\n";

    TREE *tree = csoundParseOrc(csound, orc);
    ASSERT_TRUE(tree != NULL);

    TREE *instr = tree->next;
    ASSERT_TRUE(instr != NULL);
    ASSERT_TRUE(instr->right != NULL);

    // S1 = "hello"
    TREE *stmt1 = instr->right;
    ASSERT_TRUE(stmt1->left != NULL && stmt1->left->value != NULL);
    ASSERT_STREQ("S1", stmt1->left->value->lexeme);
    ASSERT_EQ(1u, stmt1->left->value->first_column);
    ASSERT_EQ(2u, stmt1->left->value->last_column);

    // The STRING_TOKEN first_column points to the opening quote
    ASSERT_TRUE(stmt1->right != NULL && stmt1->right->value != NULL);
    ASSERT_EQ(6u, stmt1->right->value->first_column);

    // k1 on the next line should have correct columns (newline resets yycolumn)
    TREE *stmt2 = stmt1->next;
    ASSERT_TRUE(stmt2 != NULL);
    ASSERT_TRUE(stmt2->left != NULL && stmt2->left->value != NULL);
    ASSERT_STREQ("k1", stmt2->left->value->lexeme);
    ASSERT_EQ(1u, stmt2->left->value->first_column);
    ASSERT_EQ(2u, stmt2->left->value->last_column);

    csoundDeleteTree(csound, tree);
}

TEST_F (OrcCompileTests, testStringsInEvent)
{
    const char* instrument = R"(
instr One
 S1 = p4
 S2 = p5
 i1 strcmp S1, "Three"
 i2 strcmp S2, "Two"
 if i1 != 0 && i2 != 0 then
  exitnow(-1)
 endif
endin
     )";

const char* event = R"(
    i "One" 0 1 "Three" "Two"
   )";

   int32_t result = csoundCompileOrc(csound, instrument);
   ASSERT_TRUE(result == 0);
   result = csoundStart(csound);
   ASSERT_TRUE(result == 0);
   csoundEventString(csound, event, 0);
   result = csoundPerformKsmps(csound);
   ASSERT_TRUE(result == 0);
}

TEST_F (OrcCompileTests, testMaxTableSize)
{
    const char* instrument = R"(
instr 1
a1 oscil 0,0,1
endin
     )";

const char* event = R"(
 f 1 0 [2^30-1] 2 0 [2^30-1] 0
 i1 0 1
   )";

   csoundEventString(csound, event, 0);
   int32_t result =
     csoundCompileOrc(csound, instrument);
     ASSERT_TRUE(result == 0);
     result = csoundStart(csound);
     ASSERT_TRUE(result == 0);
     result = csoundPerformKsmps(csound);
    if(sizeof(MYFLT) > 4) {
    ASSERT_TRUE(result == 0);
    ASSERT_TRUE(csoundTableLength(csound,1) == pow(2,30)-1);
   }
   else
    ASSERT_FALSE(result == 0);
}

TEST_F (OrcCompileTests, test0dbfs)
{
  const char* instrument = R"(
   0dbfs = 1
     )";

  int result = csoundCompileOrc(csound, instrument);
  ASSERT_TRUE(result == 0);
  MYFLT val = csoundGet0dBFS(csound);
  ASSERT_TRUE(val == 1.0);
}

TEST_F (OrcCompileTests, testReCompileCSD)
{
  const char* instrument = R"(
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 1000
</CsScore>
</CsoundSynthesizer>
     )";

  int32_t result = csoundCompileCSD(csound,instrument,1,0);
  ASSERT_TRUE(result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  result = csoundCompileCSD(csound,instrument,1,0);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  ASSERT_TRUE(result == 0);
 }

TEST_F (OrcCompileTests, testSampleAccurate)
{
  const char* instrument = R"(
instr 1
a1 oscili 1, 440, -1, 0.25
out a1
endin
schedule(1,6/sr,0.5)
     )";


  int32_t result = csoundSetOption(csound, "--sample-accurate");
  ASSERT_TRUE(result == 0);
  result = csoundCompileOrc(csound, instrument);
  ASSERT_TRUE(result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
  const MYFLT *spout = csoundGetSpout(csound);
  ASSERT_TRUE(spout[5] == 0.0);
  ASSERT_TRUE(spout[6] == 1.0);


}

TEST_F (OrcCompileTests, testSampleAccurateLocalKsmpsUdoOutputTail)
{
  const char* instrument = R"(
sr = 64
ksmps = 64
nchnls = 2
0dbfs = 1

opcode passa, a, a
  aIn xin
  setksmps 32
  xout aIn
endop

instr 1
  aConst = 1
  aOut passa aConst
  outch p4, aOut
endin

schedule(1, 0, 24 / sr, 1)
schedule(1, 0, 40 / sr, 2)
)";

  ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundSetOption(csound, "--sample-accurate"), CSOUND_SUCCESS);
  ASSERT_EQ(csoundCompileOrc(csound, instrument), CSOUND_SUCCESS);
  ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
  ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);

  const MYFLT *spout = csoundGetSpout(csound);
  ASSERT_NE(spout, nullptr);
  constexpr int32_t blockSize = 64;
  constexpr int32_t channelCount = 2;
  constexpr int32_t endSamples[] = {24, 40};
  for (int32_t sample = 0; sample < blockSize; ++sample) {
    for (int32_t channel = 0; channel < channelCount; ++channel) {
      const MYFLT expected =
        sample < endSamples[channel] ? FL(1.0) : FL(0.0);
      EXPECT_EQ(expected, spout[sample * channelCount + channel])
        << "sample " << sample << ", channel " << channel + 1;
    }
  }
}

TEST_F (OrcCompileTests, testCompileCSD)
{
  const char* instrument = R"(
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 -1
</CsScore>
</CsoundSynthesizer>
     )";

  int32_t result = csoundCompileCSD(csound,instrument,1,0);
  ASSERT_TRUE(result == 0);
  result = csoundStart(csound);
  ASSERT_TRUE(result == 0);
  result = csoundPerformKsmps(csound);
}

TEST (ScoreCompileTests, testUnmatchedScoreParenthesis)
{
  CSOUND *csound = csoundCreate(NULL, NULL);
  const char* csd = R"(
<CsoundSynthesizer>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
instr 1
endin
</CsInstruments>
<CsScore>
i1 0 1 [1)
</CsScore>
</CsoundSynthesizer>
  )";
  EXPECT_EQ(CSOUND_ERROR, csoundCompileCSD(csound, csd, 1, 0));
  csoundDestroy(csound);
}

TEST (ScoreCompileTests, testUnmatchedScoreBracket)
{
  CSOUND *csound = csoundCreate(NULL, NULL);
  const char* csd = R"(
<CsoundSynthesizer>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
instr 1
endin
</CsInstruments>
<CsScore>
i1 0 1 [(1+2]
</CsScore>
</CsoundSynthesizer>
  )";
  EXPECT_EQ(CSOUND_ERROR, csoundCompileCSD(csound, csd, 1, 0));
  csoundDestroy(csound);
}

TEST_F (OrcCompileTests, testAssert)
{
    int32_t result;
    const char* instrument =
        "instr 1 \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(0) \n"
        "assert(1) \n"
        "endin \n";

    result = csoundSetOption(csound, "--run-unit-tests");
    ASSERT_TRUE(result == 0);
    result = csoundCompileOrc(csound, instrument);
    ASSERT_TRUE(result == 0);
    csoundReadScore(csound,  "i 1 0 0\n");
    result = csoundStart(csound);
    ASSERT_TRUE(result == 0);
    // Perform one k-cycle to execute the instrument
    csoundPerformKsmps(csound);
    ASSERT_EQ (6, csoundErrCnt(csound));
}

TEST_F (OrcCompileTests, StrcatDoesNotCopyUnusedInputCapacity)
{
    char firstData[128]{};
    char outputData[128];
    char suffixData[] = "[]";
    std::strcpy(firstData, "i");
    std::memset(outputData, 'x', sizeof(outputData));
    outputData[0] = '\0';
    STRINGDAT first{};
    STRINGDAT suffix{};
    STRINGDAT output{};
    first.data = firstData;
    first.size = sizeof(firstData);
    suffix.data = suffixData;
    suffix.size = sizeof(suffixData);
    output.data = outputData;
    output.size = 8;
    STRCAT_OP op{};
    INSDS context{};
    op.h.insdshead = &context;
    op.r = &output;
    op.str1 = &first;
    op.str2 = &suffix;

    ASSERT_EQ(OK, strcat_opcode(csound, &op));
    EXPECT_STREQ("i[]", output.data);
    for (size_t i = output.size; i < sizeof(outputData); i++) {
        EXPECT_EQ('x', outputData[i]) << "write beyond output capacity at " << i;
    }
}
