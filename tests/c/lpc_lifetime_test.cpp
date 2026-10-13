#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "lpred.h"
#include "gtest/gtest.h"
#include <unordered_set>

namespace {
class LpcLifetimeTests : public ::testing::Test {
protected:
    CSOUND *csound;
    void *(*allocate)(CSOUND *, size_t);
    void (*release)(CSOUND *, void *);
    std::unordered_set<void *> live;
    bool inSetup = false;
    int setups = 0;

    static LpcLifetimeTests &owner(CSOUND *cs) {
        return *static_cast<LpcLifetimeTests *>(csoundGetHostData(cs));
    }
    static void *trackAllocation(CSOUND *cs, size_t size) {
        auto &test = owner(cs);
        void *ptr = test.allocate(cs, size);
        if (test.inSetup) test.live.insert(ptr);
        return ptr;
    }
    static void trackFree(CSOUND *cs, void *ptr) {
        auto &test = owner(cs);
        test.live.erase(ptr);
        test.release(cs, ptr);
    }
    static void *trackSetup(CSOUND *cs, int32_t size, int32_t order) {
        auto &test = owner(cs);
        ++test.setups;
        test.inSetup = true;
        void *ptr = csoundLPsetup(cs, size, order);
        test.inSetup = false;
        return ptr;
    }
    void SetUp() override {
        csound = csoundCreate(this, nullptr);
        csoundCreateMessageBuffer(csound, 0);
        csoundSetOption(csound, "-n -d -m0");
        allocate = csound->Calloc;
        release = csound->Free;
        csound->Calloc = trackAllocation;
        csound->Free = trackFree;
        csound->LPsetup = trackSetup;
    }
    void TearDown() override {
        csoundDestroy(csound);
    }
};

TEST_F(LpcLifetimeTests, FreeReleasesEveryWorkspaceAllocation)
{
    for (int size : {0, 64}) {
        void *setup = csound->LPsetup(csound, size, 4);
        ASSERT_FALSE(live.empty());
        csound->LPfree(csound, setup);
        EXPECT_TRUE(live.empty()) << "frame size " << size;
    }
}

TEST_F(LpcLifetimeTests, OpcodesReleaseWorkspacesOnReinitAndNoteEnd)
{
    const char *csd = R"(<CsoundSynthesizer>
<CsInstruments>
sr=1024
ksmps=8
nchnls=1
0dbfs=1
giTable ftgen 1, 0, 64, 10, 1
instr 1
 kCycle timeinstk
 if kCycle == 3 then
  reinit AGAIN
 endif
 AGAIN:
 aIn init 0
 aTable lpcfilter aIn, 0, 0, giTable, 64, 4
 aStream lpcfilter aIn, aIn, 0, 16, 64, 4
 kTable[], kR1, kE1, kF1 lpcanal 0, 0, giTable, 64, 4
 kStream[], kR2, kE2, kF2 lpcanal aIn, 0, 16, 64, 4
 iTable[], iR, iE, iF lpcanal 0, 1, giTable, 64, 4
 chnset kF1+kF2, "silent_pitch"
 fLpc pvslpc aIn, 64, 16, 4
 kAmp, kFreq pvsbin fLpc, 1
 chnset kFreq, "silent_bin"
 kPvs[], kR3, kE3 pvscfs fLpc, 4, 0
 kCoefs[] fillarray 0, .25
 kParams[] apoleparams kCoefs
 rireturn
endin
</CsInstruments>
<CsScore>
i 1 0 .125
i 1 .25 .125
f 0 .4
</CsScore>
</CsoundSynthesizer>)";
    ASSERT_EQ(csoundCompileCSD(csound, csd, 1, 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(setups, 8);
    EXPECT_EQ(csoundGetControlChannel(csound, "silent_pitch", nullptr), 0);
    EXPECT_EQ(csoundGetControlChannel(csound, "silent_bin", nullptr), 16);
    const size_t active = live.size();
    ASSERT_GT(active, 0u);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    ASSERT_EQ(csoundPerformKsmps(csound), 0);
    EXPECT_EQ(setups, 16);
    EXPECT_EQ(live.size(), active);
    int status = 0;
    for (int n = 0; n < 64 && status == 0; ++n)
        status = csoundPerformKsmps(csound);
    EXPECT_GT(status, 0);
    EXPECT_EQ(setups, 32);
    EXPECT_EQ(csound->inerrcnt, 0);
    EXPECT_EQ(csound->perferrcnt, 0);
    EXPECT_TRUE(live.empty());
}
}
