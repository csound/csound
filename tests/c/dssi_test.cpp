#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <dlfcn.h>

#ifndef CSOUND_TEST_DSSI_HOST
#error DSSI host path is required
#endif

class DssiTests : public ::testing::Test {
protected:
    CSOUND *csound = nullptr;
    void *fixture = nullptr;
    int (*stat)(int) = nullptr;
    std::vector<cs_float> audio;
    std::string messages() {
        std::string result;
        while (csoundGetMessageCnt(csound)) {
            result += csoundGetFirstMessage(csound);
            csoundPopFirstMessage(csound);
        }
        return result;
    }
    std::string load(int index = 0, const char *id = "gih", const char *path = CSOUND_TEST_DSSI_PLUGIN) {
        return std::string(id) + " dssiinit \"" + path + "\", " + std::to_string(index) + ", 1\n";
    }
    int run(const std::string &orc, const std::string &score = "i1 0 .01", int ksmps = 16, int dbfs = 1) {
        std::string csd = "<CsoundSynthesizer>\n<CsOptions>\n-n -d -m0 --sample-accurate\n</CsOptions>\n"
            "<CsInstruments>\nsr=8192\nksmps=" + std::to_string(ksmps) +
            "\nnchnls=2\n0dbfs=" + std::to_string(dbfs) + "\n" + orc +
            "\n</CsInstruments>\n<CsScore>\n" + score +
            "\ne\n</CsScore>\n</CsoundSynthesizer>\n";
        int result = csoundCompileCSD(csound, csd.c_str(), 1, 0);
        if (result == 0) result = csoundStart(csound);
        if (result) return result;
        for (int blocks = 0; blocks < 1000; ++blocks) {
            int done = csoundPerformKsmps(csound);
            const cs_float *out = csoundGetSpout(csound);
            audio.insert(audio.end(), out, out + ksmps * 2);
            if (done) return csound->inerrcnt + csound->perferrcnt;
        }
        ADD_FAILURE() << "Performance did not terminate";
        return -1;
    }
    void samples(int begin, int end, cs_double expected) {
        ASSERT_GE(audio.size(), size_t(end * 2));
        for (int i = begin; i < end; ++i) {
            EXPECT_NEAR(audio[2*i], expected, 1e-6) << "sample " << i;
            EXPECT_NEAR(audio[2*i+1], expected, 1e-6) << "sample " << i;
        }
    }
    void SetUp() override {
        fixture = dlopen(CSOUND_TEST_DSSI_PLUGIN, RTLD_NOW | RTLD_LOCAL);
        ASSERT_NE(fixture, nullptr) << dlerror();
        stat = reinterpret_cast<int (*)(int)>(dlsym(fixture, "dssi_test_stat"));
        ASSERT_NE(stat, nullptr);
        reinterpret_cast<void (*)()>(dlsym(fixture, "dssi_test_reset"))();
        csound = csoundCreate(nullptr, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n");
        csoundSetOption(csound, "-d");
        csoundSetOption(csound, "--sample-accurate");
        csoundSetOption(csound, "--opcode-lib=" CSOUND_TEST_DSSI_HOST);
    }
    void TearDown() override {
        csoundDestroy(csound);
        EXPECT_EQ(stat(0), 0) << "plugin instances leaked";
        EXPECT_EQ(stat(2), 0) << "plugin callback contract failed";
        dlclose(fixture);
    }
};

TEST_F(DssiTests, SynthAndNoteOpcodesAreAvailable) {
    const char *orc = "sr=48000\nksmps=16\nnchnls=2\n0dbfs=1\n"
        "instr 1\n"
        "dssinote 1, 0, 60, 100, 1\n"
        "dssievent 1, 0, 60, 100\n"
        "a1, a2 dssisynth 0\n"
        "endin\n";
    EXPECT_EQ(csoundCompileOrc(csound, orc, 0), 0);
}

TEST_F(DssiTests, DurationSchedulesNoteOffAcrossBlocks) {
    ASSERT_EQ(run(load() + R"(
instr 1
 dssiactivate gih, 1
 konce init 1
 dssinote konce, gih, 60, 127, 20/sr
 konce = 0
 a1,a2 dssisynth gih
 outs a1,a2
endin
)"), 0) << messages();
    samples(0,20,1);
    samples(20,64,0);
    csoundReset(csound);
    EXPECT_EQ(stat(1),1);
}

TEST_F(DssiTests, MidiOffsetsAreSortedAndZeroVelocityBecomesNoteOff) {
    ASSERT_EQ(run(load() + R"(
instr 1
 dssiactivate gih,1
 konce init 1
 dssievent konce,gih,144,0,60,0,11
 dssievent konce,gih,144,0,60,127,3
 konce=0
 a1,a2 dssisynth gih
 outs a1,a2
endin
)"), 0) << messages();
    samples(0,3,0); samples(3,11,1); samples(11,64,0);
}

TEST_F(DssiTests, ProgramAndMappedControllerChangesSplitAtTheirSample) {
    ASSERT_EQ(run(load() + R"(
instr 1
 dssiactivate gih,1
 konce init 1
 dssievent konce,gih,144,0,60,127
 dssievent konce,gih,176,0,0,1,4
 dssievent konce,gih,176,0,32,2,4
 dssievent konce,gih,192,0,5,0,4
 dssievent konce,gih,176,0,7,0,10
 konce=0
 a1,a2 dssisynth gih
 kprogram dssiget gih,6
 chnset kprogram,"program"
 outs a1,a2
endin
)"), 0) << messages();
    samples(0,4,1); samples(4,10,.5); samples(10,64,0);
    EXPECT_EQ(csoundGetControlChannel(csound,"program",nullptr),130*128+5);
}

TEST_F(DssiTests, InterleavedControlPortsAndConfiguration) {
    ASSERT_EQ(run(load() + R"(
dssiconfigure gih,"bias",".125"
Sname,ibank,iprog dssiprograminfo gih,1
chnset ibank,"bank"
chnset iprog,"preset"
instr 1
 dssiactivate gih,1
 dssictls gih,4,.25,1
 dssievent 1,gih,60,127
 a1,a2 dssisynth gih
 kvalue dssiget gih,4
 chnset kvalue,"bias"
 outs a1,a2
endin
)"), 0) << messages();
    samples(0,64,1.25);
    EXPECT_EQ(csoundGetControlChannel(csound,"bias",nullptr),.25);
    EXPECT_EQ(csoundGetControlChannel(csound,"bank",nullptr),130);
    EXPECT_EQ(csoundGetControlChannel(csound,"preset",nullptr),5);
}

TEST_F(DssiTests, OptionalCallbacksAndPartialBlocks) {
    ASSERT_EQ(run(load(2) + R"(
instr 1
 dssiactivate gih,1
 konce init 1
 dssievent konce,gih,60,127
 konce=0
 a1,a2 dssisynth gih
 outs a1,a2
endin
)","i1 [3/8192] [8/8192]"), 0) << messages();
    samples(0,3,0); samples(3,11,1); samples(11,16,0);
}

TEST_F(DssiTests, LocalKsmpsAndDbfsConversion) {
    ASSERT_EQ(run(load() + R"(
opcode render,aa,i
 setksmps 4
 ih xin
 dssiactivate ih,1
 konce init 1
 dssinote konce,ih,60,127,7/sr
 konce=0
 a1,a2 dssisynth ih
 xout a1,a2
endop
instr 1
 a1,a2 render gih
 outs a1,a2
endin
)","i1 0 .01",16,32768),0) << messages();
    samples(0,7,32768); samples(7,64,0);
}

TEST_F(DssiTests, MultipleSynthCallbackReceivesEveryActiveInstance) {
    ASSERT_EQ(run(load(1,"gih") + load(1,"gih2") + R"(
instr 1
 dssiactivate gih,1
 dssiactivate gih2,1
 konce init 1
 dssievent konce,gih,60,127
 dssievent konce,gih2,64,127
 konce=0
 a1,a2 dssisynth gih
 a3,a4 dssisynth gih2
 outs a1+a3,a2+a4
endin
)","i1 0 [64/8192]"),0) << messages();
    samples(0,64,2);
    EXPECT_EQ(stat(4),2);
    EXPECT_EQ(stat(3),4);
}

TEST_F(DssiTests, LadspaEffectUsesInputAndOutputScaling) {
    ASSERT_EQ(run(load(0,"gih",CSOUND_TEST_LADSPA_PLUGIN) + R"(
instr 1
 dssiactivate gih,1
 dssictls gih,1,.5,1
 ain init 16384
 a1,a2 dssiaudio gih,ain
 outs a1,a2
endin
)","i1 0 .01",16,32768),0) << messages();
    samples(0,64,8192);
}

TEST_F(DssiTests, RejectsBadDescriptorWithoutPublishingAnInstance) {
    EXPECT_NE(run(load(99)),0);
    EXPECT_EQ(stat(0),0);
}

TEST_F(DssiTests, RejectsBankBeyondUnsigned32BitRange) {
    // 2^32 is exact in float, but UINT32_MAX is not.
    EXPECT_NE(run(load() + R"(
instr 1
 dssiprogram gih,4294967296,0,1
endin
)"), 0);
    const auto error = messages();
    EXPECT_NE(error.find("invalid bank or program"), std::string::npos) << error;
}

TEST_F(DssiTests, RejectsProgramBeyondUnsigned32BitRange) {
    EXPECT_NE(run(load() + R"(
instr 1
 dssiprogram gih,0,4294967296,1
endin
)"), 0);
    const auto error = messages();
    EXPECT_NE(error.find("invalid bank or program"), std::string::npos) << error;
}

TEST_F(DssiTests, RejectsMalformedDescriptor) {
    EXPECT_NE(run(load(3)),0);
    EXPECT_EQ(stat(0),0);
}

TEST_F(DssiTests, RejectsAudioPortUsedAsControl) {
    EXPECT_NE(run(load()+"instr 1\ndssictls gih,3,1,1\nendin\n"),0);
    EXPECT_NE(messages().find("input control port"),std::string::npos);
}

TEST_F(DssiTests, RejectsInvalidActivationHandle) {
    EXPECT_NE(run("instr 1\ndssiactivate -1,1\nendin\n"),0);
    EXPECT_NE(messages().find("invalid handle"),std::string::npos);
}

TEST_F(DssiTests, RejectsTwoRenderersForOneInstance) {
    EXPECT_NE(run(load()+R"(
instr 1
 a1,a2 dssisynth gih
 a3,a4 dssisynth gih
endin
)"),0);
    EXPECT_NE(messages().find("exactly one audio renderer"),std::string::npos);
}

TEST_F(DssiTests, QueueOverflowReportsAnError) {
    EXPECT_NE(run(load()+R"(
instr 1
 dssiactivate gih,1
 kindex=0
 while kindex < 1025 do
  dssievent 1,gih,60,127
  kindex+=1
 od
 a1,a2 dssisynth gih
endin
)"),0);
    EXPECT_NE(messages().find("event queue is full"),std::string::npos);
}

TEST_F(DssiTests, DiscoveryIncludesDssiOnlyLibrary) {
    std::string path = CSOUND_TEST_DSSI_PLUGIN;
    csoundSetOption(csound,("--env:DSSI_PATH="+path.substr(0,path.find_last_of('/'))).c_str());
    ASSERT_EQ(run("dssilist\ninstr 1\nendin\n"),0) << messages();
    EXPECT_NE(messages().find("fixture (fixture)"),std::string::npos);
}

TEST_F(DssiTests, ConfigurationErrorsReachCsound) {
    EXPECT_NE(run(load()+"dssiconfigure gih,\"bad\",\"value\"\n"),0);
    EXPECT_NE(messages().find("unknown fixture key"),std::string::npos);
}

TEST_F(DssiTests, NrpnMappingChangesControlsWithoutDuplicateEvents) {
    ASSERT_EQ(run(load()+R"(
instr 1
 dssiactivate gih,1
 konce init 1
 dssievent konce,gih,60,127
 dssinrpn konce,gih,0,42,0,9
 konce=0
 a1,a2 dssisynth gih
 kcount dssiget gih,2
 chnset kcount,"events"
 outs a1,a2
endin
)"),0) << messages();
    samples(0,9,1); samples(9,64,0);
    EXPECT_EQ(csoundGetControlChannel(csound,"events",nullptr),1);
}

TEST_F(DssiTests, ResumeStopsOldNotesWhenPluginOmitsActivationCallbacks) {
    ASSERT_EQ(run(load(2)+R"(
instr 1
 kcycle timeinstk
 kactive = (kcycle == 2 ? 0 : 1)
 dssiactivate gih,kactive
 dssievent (kcycle == 1 ? 1 : 0),gih,60,127
 a1,a2 dssisynth gih
 outs a1,a2
endin
)"),0) << messages();
    samples(0,16,1); samples(16,64,0);
}

TEST_F(DssiTests, GlobalConfigurationAppliesToEachMatchingInstance) {
    ASSERT_EQ(run(load()+load(0,"gih2")+R"(
dssiconfigure gih,"GLOBAL:bias",".125"
instr 1
 dssiactivate gih,1
 dssiactivate gih2,1
 a1,a2 dssisynth gih
 a3,a4 dssisynth gih2
 outs a1+a3,a2+a4
endin
)"),0) << messages();
    samples(0,64,.25);
}

TEST_F(DssiTests, RejectsEventsSentAfterTheRenderer) {
    EXPECT_NE(run(load()+R"(
instr 1
 dssiactivate gih,1
 a1,a2 dssisynth gih
 dssievent 1,gih,60,127
endin
)"),0);
    EXPECT_NE(messages().find("before rendering"),std::string::npos);
}

TEST_F(DssiTests, ProgramNamesAreCopiedBeforeNextEnumeration) {
    ASSERT_EQ(run(load()+R"(
Sfirst,ibank,iprog dssiprograminfo gih,0
Ssecond,ibank2,iprog2 dssiprograminfo gih,1
idiff strcmp Sfirst,"first"
chnset idiff,"diff"
Snone,ibank3,iprog3 dssiprograminfo gih,2
chnset ibank3,"end"
instr 1
endin
)"),0) << messages();
    EXPECT_EQ(csoundGetControlChannel(csound,"diff",nullptr),0);
    EXPECT_EQ(csoundGetControlChannel(csound,"end",nullptr),-1);
}

TEST_F(DssiTests, SustainedAndRepeatedNotesDoNotSurviveResume) {
    ASSERT_EQ(run(load(2)+R"(
instr 1
 kcycle timeinstk
 kactive = (kcycle == 2 ? 0 : 1)
 dssiactivate gih,kactive
 konce = (kcycle == 1 ? 1 : 0)
 dssievent konce,gih,176,0,64,127
 dssievent konce,gih,144,0,60,127
 dssievent konce,gih,144,0,60,127
 dssievent konce,gih,128,0,60,0,8
 dssievent konce,gih,128,0,60,0,8
 a1,a2 dssisynth gih
 outs a1,a2
endin
)"),0) << messages();
    samples(0,16,1); samples(16,64,0);
}

TEST_F(DssiTests, LateActivationCannotRenderAGroupTwice) {
    EXPECT_NE(run(load(1)+load(1,"gih2")+R"(
instr 1
 dssiactivate gih,1
 dssievent 1,gih,60,127
 a1,a2 dssisynth gih
endin
instr 2
 dssiactivate gih2,1
 a1,a2 dssisynth gih2
endin
)","i1 0 .01\ni2 0 .01"),0);
    EXPECT_NE(messages().find("activate all grouped instances"),std::string::npos);
    EXPECT_EQ(stat(4),1);
}

TEST_F(DssiTests, FailedFirstLoadThenInvalidHandleReportsInitErrors) {
    EXPECT_NE(run(R"(
instr 1
 ih dssiinit "/does/not/exist.so",0,0
endin
instr 2
 dssiactivate 4096,1
endin
)","i1 0 .002\ni2 .004 .002"),0);
    EXPECT_NE(messages().find("invalid handle"),std::string::npos);
}
