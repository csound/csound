#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "../../H/cs_internal.h"
#include "../../H/midiops.h"
#include "gtest/gtest.h"

#include <initializer_list>

extern "C" int32_t midiarp_set(CSOUND *, MIDIARP *);
extern "C" int32_t midiarp(CSOUND *, MIDIARP *);

class MidiarpTests : public ::testing::Test {
 protected:
  CSOUND *csound = nullptr;
  INSDS instrument{};
  MIDIARP arp{};
  cs_float note = 0, trigger = 0, rate = 1000, mode = 1;

  void SetUp() override
  {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
    ASSERT_EQ(csoundSetOption(csound, "-n"), 0);
    ASSERT_EQ(csoundCompileOrc(csound, "sr=1000\nksmps=1\nnchnls=1\n", 0), 0);
    ASSERT_EQ(csoundStart(csound), 0);
    instrument.csound = csound;
    instrument.onedkr = FL(.001);
    arp.h.insdshead = &instrument;
    arp.noteOut = &note;
    arp.counter = &trigger;
    arp.arpRate = &rate;
    arp.arpMode = &mode;
    ASSERT_EQ(midiarp_set(csound, &arp), 0);
  }

  void TearDown() override { csoundDestroy(csound); }

  void message(int status, int key, int velocity, int channel = 1)
  {
    auto *midi = csound->midiGlobals;
    auto *data = midi->MIDIINbuffer2[midi->MIDIINbufIndex++].bData;
    midi->MIDIINbufIndex &= MIDIINBUFMSK;
    data[0] = status;
    data[1] = channel;
    data[2] = key;
    data[3] = velocity;
  }

  void tick(int expected)
  {
    ASSERT_EQ(midiarp(csound, &arp), 0);
    EXPECT_EQ(trigger, expected < 0 ? 0 : 1);
    if (expected >= 0)
      EXPECT_EQ(note, expected);
  }

  void chord()
  {
    message(0x90, 67, 100);
    message(0x90, 60, 100);
    message(0x90, 64, 100);
  }
};

TEST_F(MidiarpTests, ReleaseRetriggerAndChannelState)
{
  message(0x80, 60, 0); // Unmatched release.
  tick(-1);
  message(0x90, 0, 100); // Note zero is a valid key.
  message(0x90, 0, 100); // Repeated note-on does not add a held key.
  tick(0);
  message(0x90, 0, 0);
  tick(-1);
  tick(-1); // The trigger must clear even after a trigger on the prior cycle.
  message(0x90, 64, 100, 1);
  message(0x90, 64, 100, 2);
  tick(64);
  message(0x80, 64, 0, 1);
  tick(64);
  message(0x80, 64, 0, 2);
  tick(-1);
}

TEST_F(MidiarpTests, CapacityAndRemovalKeepOnlyHeldNotes)
{
  for (int key = 60; key < 71; key++)
    message(0x90, key, 100);
  for (int key = 60; key < 70; key++)
    tick(key);
  message(0x80, 70, 0); // The eleventh key was not stored.
  message(0x80, 64, 0);
  message(0x90, 72, 100); // Fill a removed slot without losing another key.
  for (int key : {60, 61, 62, 63, 65, 66, 67, 68, 69, 72})
    tick(key);
  for (int key = 60; key < 73; key++)
    message(0x80, key, 0);
  tick(-1);
  message(0x90, 50, 100);
  message(0x90, 50, 0); // A queued press and release must not trigger a note.
  tick(-1);
}

TEST_F(MidiarpTests, UpDownAndBouncePatterns)
{
  chord();
  for (int key : {60, 64, 67, 60, 64, 67})
    tick(key);
  ASSERT_EQ(midiarp_set(csound, &arp), 0);
  mode = 2;
  chord();
  for (int key : {67, 64, 60, 67, 64, 60})
    tick(key);
  ASSERT_EQ(midiarp_set(csound, &arp), 0);
  mode = 0;
  chord();
  for (int key : {60, 64, 67, 64, 60, 64, 67})
    tick(key);
}

TEST_F(MidiarpTests, RandomChoosesOnlyHeldNotes)
{
  mode = 3;
  chord();
  for (int i = 0; i < 100; i++) {
    ASSERT_EQ(midiarp(csound, &arp), 0);
    EXPECT_EQ(trigger, 1);
    EXPECT_TRUE(note == 60 || note == 64 || note == 67);
  }
}

TEST_F(MidiarpTests, ClockStopsAndResetsOnReinitialization)
{
  rate = 250;
  message(0x90, 60, 100);
  tick(60);
  tick(-1);
  tick(-1);
  tick(-1);
  tick(60);
  rate = 0;
  tick(-1);
  rate = -10;
  tick(-1);
  ASSERT_EQ(midiarp_set(csound, &arp), 0);
  EXPECT_EQ(trigger, 0);
  EXPECT_EQ(note, 0);
  rate = 250;
  message(0x90, 67, 100);
  tick(67);
  tick(-1);
  rate = 2000;
  tick(67);
  tick(67);
}
