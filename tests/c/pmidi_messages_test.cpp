/*
  pmidi_messages_test.cpp:

  The PortMIDI module (InOut/pmidi.c) reports the devices it lists, opens
  and maps as information (CSOUNDMSG_INFO), and a failure still as an
  error. It printed all of them with ErrorMsg, so a host showing message
  types saw errors on every start with a MIDI device. With no device at
  all, 'a' and 'm' open none, input or output, with a warning, as the
  module's comment says they may: they were refused, and Csound stopped at
  its start. An output device's number out of range is an error, as an
  input's is: it opened nothing, and said nothing. And no device name opens
  the default device: the module read the name it had just found missing.

  The module runs on stand-in PortMidi functions
  (fixtures/pmidi_test_module.c), so no MIDI hardware is required.

  The test is only compiled when the pmidi plugin target exists.
*/

#include "gtest/gtest.h"

#if defined(CSOUND_TEST_PMIDI)

#include "csound.h"

#include <map>
#include <string>

extern "C" {
int32_t csound_test_pmidi_open_input(CSOUND *, void **, const char *);
int32_t csound_test_pmidi_close_input(CSOUND *, void *);
int32_t csound_test_pmidi_open_output(CSOUND *, void **, const char *);
int32_t csound_test_pmidi_close_output(CSOUND *, void *);
extern int csound_test_pmidi_no_devices;
}

namespace {

class PortMidiMessageTests : public ::testing::Test {
 protected:
  CSOUND *csound {nullptr};

  void SetUp() override {
    csound = csoundCreate(nullptr, nullptr);
    ASSERT_NE(csound, nullptr);
    csoundCreateMessageBuffer(csound, 0);
  }

  void TearDown() override {
    csound_test_pmidi_no_devices = 0;
    if (csound != nullptr) {
      csoundDestroyMessageBuffer(csound);
      csoundDestroy(csound);
    }
  }

  /* The buffered messages' text, by type, emptying the buffer. */
  std::map<int, std::string> take() {
    std::map<int, std::string> text;
    while (csoundGetMessageCnt(csound) > 0) {
      text[csoundGetFirstMessageAttr(csound) & CSOUNDMSG_TYPE_MASK] +=
          csoundGetFirstMessage(csound);
      csoundPopFirstMessage(csound);
    }
    return text;
  }
};

TEST_F(PortMidiMessageTests, OpeningMappedInputsIsInformation) {
  void *data = nullptr;
  ASSERT_EQ(csound_test_pmidi_open_input(csound, &data, "m"), 0);
  auto text = take();
  const std::string info = text[CSOUNDMSG_INFO];
  text.erase(CSOUNDMSG_INFO);
  EXPECT_EQ(text, (std::map<int, std::string>{}));
  for (const char *line : {"0: Keys", "1: Pads",
                           "Activated input device 0: 'Keys' (Stand-in)\n",
                           "Activated input device 1: 'Pads'\n",
                           "Device mapped to channels 1 to 16",
                           "Device mapped to channels 17 to 32"})
    EXPECT_NE(info.find(line), std::string::npos) << line;
  EXPECT_EQ(csound_test_pmidi_close_input(csound, data), 0);
}

TEST_F(PortMidiMessageTests, OpeningOutputsIsInformation) {
  void *data = nullptr;
  ASSERT_EQ(csound_test_pmidi_open_output(csound, &data, "a"), 0);
  auto text = take();
  const std::string info = text[CSOUNDMSG_INFO];
  text.erase(CSOUNDMSG_INFO);
  EXPECT_EQ(text, (std::map<int, std::string>{}));
  for (const char *line : {"0: Synth", "1: Box",
                           "selected output device 0: 'Synth' (Stand-in)\n",
                           "selected output device 1: 'Box'\n"})
    EXPECT_NE(info.find(line), std::string::npos) << line;
  EXPECT_EQ(csound_test_pmidi_close_output(csound, data), 0);
}

TEST_F(PortMidiMessageTests, AFailureIsStillAnError) {
  void *data = nullptr;
  EXPECT_NE(csound_test_pmidi_open_input(csound, &data, "5"), 0);
  auto text = take();
  EXPECT_NE(text[CSOUNDMSG_ERROR].find("device number is out of range"),
            std::string::npos);
  EXPECT_EQ(text[CSOUNDMSG_INFO].find("out of range"), std::string::npos);
}

TEST_F(PortMidiMessageTests, AnOutputOutOfRangeIsAnError) {
  void *data = nullptr;
  ASSERT_EQ(csound_test_pmidi_open_output(csound, &data, "1"), 0);
  EXPECT_NE(take()[CSOUNDMSG_INFO].find("selected output device 1: 'Box'"),
            std::string::npos);
  EXPECT_EQ(csound_test_pmidi_close_output(csound, data), 0);
  data = nullptr;
  EXPECT_NE(csound_test_pmidi_open_output(csound, &data, "2"), 0);
  EXPECT_EQ(data, nullptr);
  EXPECT_NE(take()[CSOUNDMSG_ERROR].find("device number is out of range"),
            std::string::npos);
}

TEST_F(PortMidiMessageTests, NoNameOpensTheDefaultDevices) {
  void *data = nullptr;
  ASSERT_EQ(csound_test_pmidi_open_input(csound, &data, nullptr), 0);
  auto text = take();
  EXPECT_NE(text[CSOUNDMSG_INFO].find("Activated input device 0: 'Keys'"),
            std::string::npos);
  EXPECT_EQ(text[CSOUNDMSG_INFO].find("Device mapped"), std::string::npos);
  EXPECT_EQ(csound_test_pmidi_close_input(csound, data), 0);
  data = nullptr;
  ASSERT_EQ(csound_test_pmidi_open_output(csound, &data, nullptr), 0);
  EXPECT_NE(take()[CSOUNDMSG_INFO].find("selected output device 0: 'Synth'"),
            std::string::npos);
  EXPECT_EQ(csound_test_pmidi_close_output(csound, data), 0);
}

TEST_F(PortMidiMessageTests, AllOrMappedDevicesOpenNoneWithNoDevice) {
  csound_test_pmidi_no_devices = 1;
  for (const char *dev : {"a", "m"}) {
    void *data = nullptr;
    EXPECT_EQ(csound_test_pmidi_open_input(csound, &data, dev), 0) << dev;
    auto text = take();
    EXPECT_EQ(text[CSOUNDMSG_ERROR], "") << dev;
    EXPECT_NE(text[CSOUNDMSG_WARNING].find("no input devices are available"),
              std::string::npos) << dev;
    EXPECT_EQ(csound_test_pmidi_close_input(csound, data), 0) << dev;
    data = nullptr;
    EXPECT_EQ(csound_test_pmidi_open_output(csound, &data, dev), 0) << dev;
    text = take();
    EXPECT_EQ(text[CSOUNDMSG_ERROR], "") << dev;
    EXPECT_NE(text[CSOUNDMSG_WARNING].find("no output devices are available"),
              std::string::npos) << dev;
    EXPECT_EQ(csound_test_pmidi_close_output(csound, data), 0) << dev;
  }
}

TEST_F(PortMidiMessageTests, TheDefaultDeviceWithNoDeviceIsStillAnError) {
  csound_test_pmidi_no_devices = 1;
  const char *names[] = {"", nullptr};
  for (const char *dev : names) {
    void *data = nullptr;
    EXPECT_NE(csound_test_pmidi_open_input(csound, &data, dev), 0);
    EXPECT_NE(take()[CSOUNDMSG_ERROR].find("no input devices are available"),
              std::string::npos);
    EXPECT_NE(csound_test_pmidi_open_output(csound, &data, dev), 0);
    EXPECT_NE(take()[CSOUNDMSG_ERROR].find("no output devices are available"),
              std::string::npos);
  }
}

}  // namespace

#endif  // CSOUND_TEST_PMIDI
