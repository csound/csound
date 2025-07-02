#include <stdio.h>
#include "gtest/gtest.h"
#define __BUILDING_LIBCSOUND
#include "csoundCore.h"


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
