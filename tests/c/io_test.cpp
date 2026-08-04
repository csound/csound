#include <stdio.h>
#include <algorithm>
#include <chrono>
#include <future>
#include <thread>
#include "gtest/gtest.h"
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"
#include "filesys.h"

extern "C" int32_t csoundKillInstance(CSOUND *, MYFLT, char *, int32_t,
                                       int32_t, int32_t);

typedef struct {
    OPDS h;
} SLOW_INIT;

static int32_t slowInit(CSOUND *csound, SLOW_INIT *p)
{
    (void) p;
    csoundSleep(25);
    return CSOUND_SUCCESS;
}

static int32_t failSndfileClose(CSOUND *, void *)
{
    return -1;
}

static int32_t removeAfterDeferredClose(const std::string &path)
{
    int32_t result = -1;

    /* Retired files are detached from the registry before the worker closes
       them. POSIX permits removing an open file, but Windows requires waiting
       for that final close to finish. */
    for (int32_t i = 0; i < 1000 && result != 0; ++i) {
      result = remove(path.c_str());
      if (result != 0)
        csoundSleep(1);
    }
    return result;
}


class IOTests : public ::testing::Test {
public:
    IOTests ()
    {
    }

    virtual ~IOTests ()
    {
    }

    virtual void SetUp ()
    {
      csound = csoundCreate (NULL,NULL);
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

TEST_F (IOTests, testDeviceList)
{
    char *name, *type;
    int32_t n = 0;

    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "midi") == 0) {
            csoundSetMIDIModule(csound,name);
            int32_t i,ndevs = csoundGetMIDIDevList(csound,NULL,1);
            CS_MIDIDEVICE *devs = (CS_MIDIDEVICE *) malloc(ndevs*sizeof(CS_MIDIDEVICE));
            csoundGetMIDIDevList(csound,devs,1);
            for(i=0; i < ndevs; i++) {
                ASSERT_TRUE (devs[i].device_id != NULL);
                ASSERT_TRUE (devs[i].device_name != NULL);
                ASSERT_STREQ (devs[i].midi_module, name);
            }
            free(devs);
        } else if (strcmp(type, "audio") == 0) {
            csoundSetRTAudioModule(csound,name);
            int32_t i,ndevs = csoundGetAudioDevList(csound,NULL,1);
            CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *) malloc(ndevs*sizeof(CS_AUDIODEVICE));
            csoundGetAudioDevList(csound,devs,1);
            for(i=0; i < ndevs; i++) {
                ASSERT_TRUE (devs[i].device_id != NULL);
                ASSERT_TRUE (devs[i].device_name != NULL);
            }
            free(devs);
        }
    }

    printf("\nInput ------------------------------\n");
    n = 0;
    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "midi") == 0) {
            csoundSetMIDIModule(csound,name);
            int32_t i,ndevs = csoundGetMIDIDevList(csound,NULL,0);
            CS_MIDIDEVICE *devs = (CS_MIDIDEVICE *) malloc(ndevs*sizeof(CS_MIDIDEVICE));
            csoundGetMIDIDevList(csound,devs,0);
            for(i=0; i < ndevs; i++) {
                ASSERT_TRUE (devs[i].device_id != NULL);
                ASSERT_TRUE (devs[i].device_name != NULL);
                ASSERT_STREQ (devs[i].midi_module, name) ;
            }
            free(devs);
        } else if (strcmp(type, "audio") == 0) {
            csoundSetRTAudioModule(csound,name);
            int32_t i,ndevs = csoundGetAudioDevList(csound,NULL,0);
            CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *) malloc(ndevs*sizeof(CS_AUDIODEVICE));
            csoundGetAudioDevList(csound,devs,0);
            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            for(i=0; i < ndevs; i++) {
                ASSERT_TRUE (devs[i].device_id != NULL);
                ASSERT_TRUE (devs[i].device_name != NULL);
                printf(" %d: %s (%s)\n", i, devs[i].device_id, devs[i].device_name);
            }
            free(devs);
        }
    }
}

int32_t key_callback_evt(void *userData, void *p, uint32_t type)
{
    int32_t *prev = (int32_t *) userData;
    *((int32_t *) p) = *prev;
    *prev += 1;
    return CSOUND_SUCCESS;
}

int32_t key_callback_txt(void *userData, void *p, uint32_t type)
{
    int32_t *prev = (int32_t *) userData;
    *((int32_t *) p) =  *prev;
    *prev += 1;
    return CSOUND_SUCCESS;
}

struct ReadlineInput {
    const char *text;
    size_t position;
};

int32_t readline_callback(void *userData, void *p, uint32_t type)
{
    ReadlineInput *input = (ReadlineInput *) userData;
    unsigned char character = (unsigned char) input->text[input->position];

    *((int32_t *) p) = (int32_t) character;
    if (character != '\0')
        input->position++;
    return CSOUND_SUCCESS;
}

int32_t readline_error_callback(void *, void *, uint32_t)
{
    return CSOUND_ERROR;
}

TEST_F (IOTests, testKeyboardIO)
{
    int32_t ret, prev = 100;

    ret = csoundRegisterKeyboardCallback(csound, key_callback_evt, &prev, CSOUND_CALLBACK_KBD_EVENT);
    ASSERT_TRUE (ret == CSOUND_SUCCESS);

    ret = csoundRegisterKeyboardCallback(csound, key_callback_txt, &prev, CSOUND_CALLBACK_KBD_TEXT);
    ASSERT_TRUE (ret == CSOUND_SUCCESS);

    const char  *instrument =
            "chn_k \"key\", 2\n"
            "instr 1 \n"
            "kkey sensekey\n"
            "chnset kkey, \"key\"\n"
            "kkey2, kdown2 sensekey\n"
            "chnset kkey2, \"key2\"\n"
            "chnset kdown2, \"down2\"\n"
            "endin \n";

    csoundCompileOrc(csound, instrument, 0);
    csoundEventString(csound, "i 1 0 1", 0);

    ret = csoundStart(csound);
    ASSERT_TRUE (ret == CSOUND_SUCCESS);

    ret = csoundPerformKsmps(csound);
    ASSERT_TRUE (ret == CSOUND_SUCCESS);

    // TODO these assertions are failing
    // MYFLT val = csoundGetControlChannel(csound, "key", &err);
    // ASSERT_DOUBLE_EQ (val, 100.0);

    // val = csoundGetControlChannel(csound, "key2", &err);
    // ASSERT_DOUBLE_EQ (val, 101.0);

    // val = csoundGetControlChannel(csound, "down2", &err);
    // ASSERT_DOUBLE_EQ (val, 1.0);
}

TEST_F (IOTests, testSynchronousCloseWhileAsyncWorkerRuns)
{
    std::string prefix = ::testing::TempDir() + "csound_" +
                         std::to_string(reinterpret_cast<uintptr_t>(csound));
    std::string asyncPath = prefix + "_async.tmp";
    std::string syncPath = prefix + "_sync.tmp";
    FILE *asyncFile = nullptr;
    FILE *syncFile = nullptr;

    remove(asyncPath.c_str());
    remove(syncPath.c_str());
    void *asyncHandle = csoundFileOpenAsync(
        csound, &asyncFile, CSFILE_STD, asyncPath.c_str(), (void *) "w+",
        nullptr, CSFTYPE_OTHER_TEXT, 64, 0);
    ASSERT_NE(asyncHandle, nullptr);
    void *syncHandle = csoundFileOpen(
        csound, &syncFile, CSFILE_STD, syncPath.c_str(), (void *) "w+", nullptr,
        CSFTYPE_OTHER_TEXT, 0);
    ASSERT_NE(syncHandle, nullptr);
    auto retiredFilesEmpty = [&]() {
      csoundSpinLock(&csound->open_files_lock);
      const bool empty = csound->retired_files == nullptr;
      csoundSpinUnLock(&csound->open_files_lock);
      return empty;
    };

    ASSERT_EQ(csoundFileClose(csound, syncHandle, CSFILE_CLOSE_SYNC), 0);
    EXPECT_TRUE(retiredFilesEmpty());
    EXPECT_EQ(remove(syncPath.c_str()), 0);

    ASSERT_EQ(csoundFileClose(csound, asyncHandle, CSFILE_CLOSE_SYNC), 0);
    EXPECT_TRUE(retiredFilesEmpty());
    EXPECT_EQ(remove(asyncPath.c_str()), 0);
}

TEST_F (IOTests, testSynchronousCloseReportsBorrowedFileError)
{
    SNDFILE *fakeSndfile = reinterpret_cast<SNDFILE *>(csound);
    void *handle = csoundCreateFileHandle(
        csound, &fakeSndfile, CSFILE_SND_R, "invalid-borrowed-file");
    ASSERT_NE(handle, nullptr);
    auto *file = static_cast<CSFILE *>(handle);
    csound->SndfileClose = failSndfileClose;

    csoundSpinLock(&csound->open_files_lock);
    file->io_readers++;
    csoundSpinUnLock(&csound->open_files_lock);
    std::thread reader([&]() {
      csoundSleep(10);
      csoundSpinLock(&csound->open_files_lock);
      file->io_readers--;
      csoundSpinUnLock(&csound->open_files_lock);
    });

    EXPECT_EQ(csoundFileClose(csound, handle, CSFILE_CLOSE_SYNC), -1);
    reader.join();
    EXPECT_EQ(csound->retired_files, nullptr);
}

TEST_F (IOTests, testDeferredCloseDoesNotWaitForBorrowedFile)
{
    std::string path = ::testing::TempDir() + "csound_deferred_" +
                       std::to_string(reinterpret_cast<uintptr_t>(csound)) +
                       ".tmp";
    FILE *stream = nullptr;

    remove(path.c_str());
    void *handle = csoundFileOpenAsync(
        csound, &stream, CSFILE_STD, path.c_str(), (void *) "w+", nullptr,
        CSFTYPE_OTHER_TEXT, 64, 0);
    ASSERT_NE(handle, nullptr);
    auto *file = static_cast<CSFILE *>(handle);

    csoundSpinLock(&csound->open_files_lock);
    file->io_readers++;
    csoundSpinUnLock(&csound->open_files_lock);

    EXPECT_EQ(csoundFileClose(csound, handle, CSFILE_CLOSE_DEFER),
              CSOUND_SUCCESS);
    csoundSpinLock(&csound->open_files_lock);
    EXPECT_TRUE(file->retired);
    file->io_readers--;
    csoundSpinUnLock(&csound->open_files_lock);

    bool retiredFilesEmpty = false;
    for (int32_t i = 0; i < 100 && !retiredFilesEmpty; ++i) {
      csoundSpinLock(&csound->open_files_lock);
      retiredFilesEmpty = csound->retired_files == nullptr;
      csoundSpinUnLock(&csound->open_files_lock);
      if (!retiredFilesEmpty)
        csoundSleep(1);
    }
    EXPECT_TRUE(retiredFilesEmpty);
    EXPECT_EQ(removeAfterDeferredClose(path), 0);
}

TEST_F (IOTests, testFileCloseRejectsUnknownFlags)
{
    std::string path = ::testing::TempDir() + "csound_close_flags_" +
                       std::to_string(reinterpret_cast<uintptr_t>(csound)) +
                       ".tmp";
    FILE *stream = nullptr;

    remove(path.c_str());
    void *handle = csoundFileOpen(
        csound, &stream, CSFILE_STD, path.c_str(), (void *) "w+", nullptr,
        CSFTYPE_OTHER_TEXT, 0);
    ASSERT_NE(handle, nullptr);

    EXPECT_EQ(csoundFileClose(csound, handle, 1U << 8), CSOUND_ERROR);
    EXPECT_EQ(csoundFileClose(csound, handle, CSFILE_CLOSE_SYNC), 0);
    EXPECT_EQ(remove(path.c_str()), 0);
}

TEST_F (IOTests, testRealtimeFoutTurnoffDoesNotWaitForBorrowedFile)
{
    std::string path = ::testing::TempDir() + "csound_fout_" +
                       std::to_string(reinterpret_cast<uintptr_t>(csound)) +
                       ".wav";
    std::replace(path.begin(), path.end(), '\\', '/');
    std::string orchestra = R"(
      sr = 48000
      ksmps = 64
      nchnls = 1
      0dbfs = 1
      instr 1
        asig init 0
        test_sleep_init
        fout ")" + path + R"(", 14, asig
      endin
    )";

    remove(path.c_str());
    ASSERT_EQ(csoundAppendOpcode(csound, "test_sleep_init", sizeof(SLOW_INIT),
                                 0, "", "", (SUBR) slowInit, nullptr, nullptr),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "--realtime"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, orchestra.c_str(), 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 10", 0);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    /* Realtime init runs on the event thread. Advance performance until
       fout's async handle is ready; the short score also verifies that a
       queued init prevents premature end-of-performance. */
    CSFILE *file = nullptr;
    bool borrowed = false;
    auto initDeadline = std::chrono::steady_clock::now() +
                        std::chrono::seconds(5);
    while (!borrowed && std::chrono::steady_clock::now() < initDeadline) {
      ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
      csoundSpinLock(&csound->open_files_lock);
      for (file = static_cast<CSFILE *>(csound->open_files);
           file != nullptr && file->type != CSFILE_SND_W; file = file->nxt) {
      }
      if (file != nullptr && file->async_flag != 0) {
        file->io_readers++;
        borrowed = true;
      }
      csoundSpinUnLock(&csound->open_files_lock);
      if (!borrowed)
        csoundSleep(1);
    }
    ASSERT_NE(file, nullptr);
    ASSERT_TRUE(borrowed);

    ASSERT_EQ(csoundKillInstance(csound, FL(1.0), nullptr, 0, 0, 1),
              CSOUND_SUCCESS);
    auto turnoff = std::async(std::launch::async, [this]() {
      return csoundPerformKsmps(csound);
    });
    auto status = turnoff.wait_for(std::chrono::seconds(5));

    csoundSpinLock(&csound->open_files_lock);
    file->io_readers--;
    csoundSpinUnLock(&csound->open_files_lock);

    EXPECT_EQ(status, std::future_status::ready);
    EXPECT_EQ(turnoff.get(), CSOUND_SUCCESS);

    bool retiredFilesEmpty = false;
    for (int32_t i = 0; i < 100 && !retiredFilesEmpty; ++i) {
      csoundSpinLock(&csound->open_files_lock);
      retiredFilesEmpty = csound->retired_files == nullptr;
      csoundSpinUnLock(&csound->open_files_lock);
      if (!retiredFilesEmpty)
        csoundSleep(1);
    }
    EXPECT_TRUE(retiredFilesEmpty);
    EXPECT_EQ(removeAfterDeferredClose(path), 0);
}

TEST_F (IOTests, testReadline)
{
    ReadlineInput input = {
        "acd\033[D\033[Db\033[CX\177\033[C\n", 0
    };
    int32_t ret = csoundRegisterKeyboardCallback(
        csound, readline_callback, &input, CSOUND_CALLBACK_KBD_TEXT);
    ASSERT_EQ(ret, CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);

    const char *instrument =
        "ksmps = 1\n"
        "chnk \"readline_status\", 2\n"
        "chnS \"readline_line\", 2\n"
        "instr 1\n"
        "  Sline, kstatus readline \"\"\n"
        "  if (kstatus == 1) then\n"
        "    chnset Sline, \"readline_line\"\n"
        "    chnset kstatus, \"readline_status\"\n"
        "  endif\n"
        "endin\n";

    ASSERT_EQ(csoundCompileOrc(csound, instrument, 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    MYFLT status = 0;
    int32_t error = 0;
    for (int32_t cycle = 0; cycle < 32 && status == 0; cycle++) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        status = csoundGetControlChannel(csound, "readline_status", &error);
        ASSERT_EQ(error, CSOUND_SUCCESS);
    }

    char line[32];
    ASSERT_EQ(status, FL(1.0));
    csoundGetStringChannel(csound, "readline_line", line);
    ASSERT_STREQ(line, "abcd");
}

TEST_F (IOTests, testReadlineHostInput)
{
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);

    const char *instrument =
        "ksmps = 1\n"
        "chnk \"readline_status\", 2\n"
        "chnS \"readline_line\", 2\n"
        "instr 1\n"
        "  Sline, kstatus readline \"\"\n"
        "  if (kstatus == 1) then\n"
        "    chnset Sline, \"readline_line\"\n"
        "    chnset kstatus, \"readline_status\"\n"
        "  endif\n"
        "endin\n";

    ASSERT_EQ(csoundCompileOrc(csound, instrument, 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundReadlinePushText(csound, "from host\n"),
              CSOUND_SUCCESS);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);

    MYFLT status = 0;
    int32_t error = 0;
    for (int32_t cycle = 0; cycle < 32 && status == 0; cycle++) {
        ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
        status = csoundGetControlChannel(csound, "readline_status", &error);
        ASSERT_EQ(error, CSOUND_SUCCESS);
    }

    char line[32];
    ASSERT_EQ(status, FL(1.0));
    csoundGetStringChannel(csound, "readline_line", line);
    ASSERT_STREQ(line, "from host");
}

TEST_F (IOTests, testReadlineRejectsSecondPrompt)
{
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);

    const char *instrument =
        "ksmps = 1\n"
        "instr 1\n"
        "  Sfirst, kfirst readline \"first> \"\n"
        "  Ssecond, ksecond readline \"second> \"\n"
        "endin\n";

    ASSERT_EQ(csoundCompileOrc(csound, instrument, 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    ASSERT_NE(csoundPerformKsmps(csound), CSOUND_SUCCESS);
}

TEST_F (IOTests, testReadlineReportsEof)
{
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);

    const char *instrument =
        "ksmps = 1\n"
        "chnk \"readline_status\", 2\n"
        "chnk \"second_readline_status\", 2\n"
        "chnS \"second_readline_line\", 2\n"
        "instr 1\n"
        "  Sline, kstatus readline \"\"\n"
        "  chnset kstatus, \"readline_status\"\n"
        "endin\n"
        "instr 2\n"
        "  Sline, kstatus readline \"\"\n"
        "  if (kstatus == 1) then\n"
        "    chnset Sline, \"second_readline_line\"\n"
        "    chnset kstatus, \"second_readline_status\"\n"
        "  endif\n"
        "endin\n";

    ASSERT_EQ(csoundCompileOrc(csound, instrument, 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    csoundKeyPress(csound, '\004');
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);

    int32_t error = 0;
    MYFLT status = csoundGetControlChannel(
        csound, "readline_status", &error);
    ASSERT_EQ(error, CSOUND_SUCCESS);
    ASSERT_EQ(status, FL(-1.0));

    csoundEventString(csound, "i 2 0 1", 0);
    csoundKeyPress(csound, 'x');
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);
    csoundKeyPress(csound, '\n');
    ASSERT_EQ(csoundPerformKsmps(csound), CSOUND_SUCCESS);

    status = csoundGetControlChannel(
        csound, "second_readline_status", &error);
    ASSERT_EQ(error, CSOUND_SUCCESS);
    ASSERT_EQ(status, FL(1.0));

    char line[8];
    csoundGetStringChannel(csound, "second_readline_line", line);
    ASSERT_STREQ(line, "x");
}

TEST_F (IOTests, testReadlineReleasesInputAfterError)
{
    const char *instrument =
        "ksmps = 1\n"
        "instr 1\n"
        "  Sline, kstatus readline \"\"\n"
        "endin\n";
    ASSERT_EQ(csoundRegisterKeyboardCallback(
        csound, readline_error_callback, nullptr,
        CSOUND_CALLBACK_KBD_TEXT), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(csound, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(csound, instrument, 0), CSOUND_SUCCESS);
    csoundEventString(csound, "i 1 0 1", 0);
    ASSERT_EQ(csoundStart(csound), CSOUND_SUCCESS);
    // A performance error need not change this control period's return value.
    (void) csoundPerformKsmps(csound);

    CSOUND *other = csoundCreate(NULL, NULL);
    ASSERT_NE(other, nullptr);
    csoundCreateMessageBuffer(other, 0);
    ASSERT_EQ(csoundSetOption(other, "--logfile=NULL"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundSetOption(other, "-n"), CSOUND_SUCCESS);
    ASSERT_EQ(csoundCompileOrc(other, instrument, 0), CSOUND_SUCCESS);
    csoundEventString(other, "i 1 0 1", 0);
    ASSERT_EQ(csoundStart(other), CSOUND_SUCCESS);
    EXPECT_EQ(csoundPerformKsmps(other), CSOUND_SUCCESS);
    csoundDestroy(other);
}


TEST_F (IOTests, testAudioModules)
{
    char *name, *type;
    int32_t n = 0;

    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "audio") == 0) {
            const char  *instrument =
                    "ksmps = 256\n"
                    "instr 1 \n"
                    "asig oscil 0.1, 440\n"
                    "out asig\n"
                    "endin \n";
            csoundSetOption(csound, "-B4096");
            csoundSetOption(csound, "-odac");
            csoundCompileOrc(csound, instrument, 0);
            csoundEventString(csound, "i 1 0 0.1\n e 0.2", 0);
            csoundSetRTAudioModule(csound, name);     
            int32_t ret = csoundStart(csound);
            if (strcmp(name, "jack") != 0) { // Jack module would fail this test if jack is not running
              ASSERT_TRUE (ret == 0);
            }
            if (ret == 0) {
              while(ret == 0) ret = csoundPerformKsmps(csound);
              ASSERT_TRUE (ret > 0);
            }
            csoundReset(csound);
        }
    }

}

TEST_F (IOTests, testAudioHostBased)
{
    const char  *instrument =
            "ksmps = 256\n"
            "instr 1 \n"
            "asig oscil 0.1, 440\n"
            "out asig\n"
            "endin \n";
    csoundSetOption(csound, "-B4096");
    csoundSetOption(csound, "-odac");
    csoundCompileOrc(csound, instrument, 0);
    csoundEventString(csound, "i 1 0 0.1\n e 0.2", 0);
    csoundSetHostAudioIO(csound);

    int32_t ret = csoundStart(csound);
    ASSERT_TRUE (ret == 0);
    while(ret == 0) ret = csoundPerformKsmps(csound);
    ASSERT_TRUE (ret > 0);
    csoundReset(csound);
}

TEST_F (IOTests, testMidiModules)
{
    char *name, *type;
    int32_t n = 0;

    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "midi") == 0) {
            const char  *instrument =
                    "ksmps = 256\n"
                    "instr 1 \n"
                    "asig oscil 0.1, 440\n"
                    "out asig\n"
                    "endin \n";
            csoundSetOption(csound, "-B4096");
            csoundSetOption(csound, "-odac");
            csoundCompileOrc(csound, instrument, 0);
            csoundEventString(csound, "i 1 0 0.1\n e 0.2", 0);
            csoundSetMIDIModule(csound, name);
            int32_t ret = csoundStart(csound);
            ASSERT_TRUE (ret == 0);
            while(ret == 0) ret = csoundPerformKsmps(csound);
            ASSERT_TRUE (ret > 0);
            csoundReset(csound);
        }
    }
}

TEST_F (IOTests, testMidiHostBased)
{
    const char  *instrument =
            "ksmps = 256\n"
            "instr 1 \n"
            "asig oscil 0.1, 440\n"
            "out asig\n"
            "endin \n";
    csoundSetOption(csound, "-B4096");
    csoundSetOption(csound, "-odac");  
    csoundCompileOrc(csound, instrument, 0);
    csoundEventString(csound, "i 1 0 0.1\n e 0.2", 0);
    csoundSetHostMIDIIO(csound);
    int32_t ret = csoundStart(csound);
    ASSERT_TRUE (ret == 0);
    while(ret == 0) ret = csoundPerformKsmps(csound);
    ASSERT_TRUE (ret > 0);
    csoundReset(csound);
}
