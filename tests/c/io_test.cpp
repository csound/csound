#include <stdio.h>
#include "gtest/gtest.h"

#include "csound.h"

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
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (0);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL -odac");
    }

    virtual void TearDown ()
    {
        csoundCleanup (csound);
        csoundDestroyMessageBuffer (csound);
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};

TEST_F (IOTests, testDeviceList)
{
    char *name, *type;
    int n = 0;

    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "midi") == 0) {
            csoundSetMIDIModule(csound,name);
            int i,ndevs = csoundGetMIDIDevList(csound,NULL,1);
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
            int i,ndevs = csoundGetAudioDevList(csound,NULL,1);
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
            int i,ndevs = csoundGetMIDIDevList(csound,NULL,0);
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
            int i,ndevs = csoundGetAudioDevList(csound,NULL,0);
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

int key_callback_evt(void *userData, void *p, unsigned int type)
{
    int *prev = (int *) userData;
    *((int *) p) = *prev;
    *prev += 1;
    return CSOUND_SUCCESS;
}

int key_callback_txt(void *userData, void *p, unsigned int type)
{
    int *prev = (int *) userData;
    *((int *) p) =  *prev;
    *prev += 1;
    return CSOUND_SUCCESS;
}

TEST_F (IOTests, testKeyboardIO)
{
    int ret, err, prev = 100;
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
    csoundCompileOrc(csound, instrument);
    csoundReadScore(csound, "i 1 0 1");
    ret = csoundStart(csound);
    ASSERT_TRUE (ret == CSOUND_SUCCESS);
    ret = csoundPerformKsmps(csound);
    ASSERT_TRUE (ret == CSOUND_SUCCESS);
    MYFLT val = csoundGetControlChannel(csound, "key", &err);
    ASSERT_DOUBLE_EQ (val, 100.0);
    val = csoundGetControlChannel(csound, "key2", &err);
    ASSERT_DOUBLE_EQ (val, 101.0);
    val = csoundGetControlChannel(csound, "down2", &err);
    ASSERT_DOUBLE_EQ (val, 1.0);
}


TEST_F (IOTests, testAudioModules)
{
    char *name, *type;
    int n = 0;

    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "audio") == 0) {
            const char  *instrument =
                    "ksmps = 256\n"
                    "instr 1 \n"
                    "asig oscil 0.1, 440\n"
                    "out asig\n"
                    "endin \n";
            csoundSetOption(csound, "-B4096");
            csoundCompileOrc(csound, instrument);
            csoundReadScore(csound, "i 1 0 0.1\n e 0.2");
            csoundSetRTAudioModule(csound, name);
            csoundSetOutput(csound, "dac", NULL, NULL);
            int ret = csoundStart(csound);
            if (strcmp(name, "jack") != 0) { // Jack module would fail this test if jack is not running
              ASSERT_TRUE (ret == 0);
            }
            if (ret == 0) {
              ret = csoundPerform(csound);
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
    csoundCompileOrc(csound, instrument);
    csoundReadScore(csound, "i 1 0 0.1\n e 0.2");
    csoundSetHostImplementedAudioIO(csound, 1, 256);
    csoundSetOutput(csound, "dac", NULL, NULL);
    int ret = csoundStart(csound);
    ASSERT_TRUE (ret == 0);
    ret = csoundPerform(csound);
    ASSERT_TRUE (ret > 0);
    csoundReset(csound);
}

TEST_F (IOTests, testMidiModules)
{
    char *name, *type;
    int n = 0;

    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "midi") == 0) {
            const char  *instrument =
                    "ksmps = 256\n"
                    "instr 1 \n"
                    "asig oscil 0.1, 440\n"
                    "out asig\n"
                    "endin \n";
            csoundSetOption(csound, "-B4096");
            csoundCompileOrc(csound, instrument);
            csoundReadScore(csound, "i 1 0 0.1\n e 0.2");
            csoundSetMIDIModule(csound, name);
            csoundSetOutput(csound, "dac", NULL, NULL);
            int ret = csoundStart(csound);
            ASSERT_TRUE (ret == 0);
            ret = csoundPerform(csound);
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
    csoundCompileOrc(csound, instrument);
    csoundReadScore(csound, "i 1 0 0.1\n e 0.2");
    csoundSetHostImplementedMIDIIO(csound, 1);
    csoundSetOutput(csound, "dac", NULL, NULL);
    int ret = csoundStart(csound);
    ASSERT_TRUE (ret == 0);
    ret = csoundPerform(csound);
    ASSERT_TRUE (ret > 0);
    csoundReset(csound);
}
