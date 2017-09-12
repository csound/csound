#include "csound.h"
#include <stdio.h>

#include <CUnit/Basic.h>

int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}

void test_dev_list(void)
{
    CSOUND  *csound;
    csound = csoundCreate(NULL);
    char *name, *type;
    int n = 0;

//    printf("Output ------------------------------\n");
    while(!csoundGetModule(csound, n++, &name, &type)) {
        if (strcmp(type, "midi") == 0) {
            csoundSetMIDIModule(csound,name);
            int i,ndevs = csoundGetMIDIDevList(csound,NULL,1);
            CS_MIDIDEVICE *devs = (CS_MIDIDEVICE *) malloc(ndevs*sizeof(CS_MIDIDEVICE));
            csoundGetMIDIDevList(csound,devs,1);
//            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            for(i=0; i < ndevs; i++) {
                CU_ASSERT_PTR_NOT_NULL(devs[i].device_id);
                CU_ASSERT_PTR_NOT_NULL(devs[i].device_name);
                CU_ASSERT_STRING_EQUAL(devs[i].midi_module, name);
//                printf(" %d: %s %s (%s)\n", i,
//                       devs[i].device_id, devs[i].device_name, devs[i].interface_name);
            }
            free(devs);
        } else if (strcmp(type, "audio") == 0) {
            csoundSetRTAudioModule(csound,name);
            int i,ndevs = csoundGetAudioDevList(csound,NULL,1);
            CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *) malloc(ndevs*sizeof(CS_AUDIODEVICE));
            csoundGetAudioDevList(csound,devs,1);
//            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            for(i=0; i < ndevs; i++) {
                CU_ASSERT_PTR_NOT_NULL(devs[i].device_id);
                CU_ASSERT_PTR_NOT_NULL(devs[i].device_name);
//                CU_ASSERT_STRING_EQUAL(devs[i].rt_module, name);
//                printf(" %d: %s (%s)\n", i, devs[i].device_id, devs[i].device_name);
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
	    //printf("%d \n", ndevs);
            CS_MIDIDEVICE *devs = (CS_MIDIDEVICE *) malloc(ndevs*sizeof(CS_MIDIDEVICE));
            csoundGetMIDIDevList(csound,devs,0);
//            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            for(i=0; i < ndevs; i++) {
                CU_ASSERT_PTR_NOT_NULL(devs[i].device_id);
                CU_ASSERT_PTR_NOT_NULL(devs[i].device_name);
                CU_ASSERT_STRING_EQUAL(devs[i].midi_module, name) ;
//                printf(" %d: %s %s (%s)\n", i,
//                       devs[i].device_id, devs[i].device_name, devs[i].interface_name);
            }
            free(devs);
        } else if (strcmp(type, "audio") == 0) {
            csoundSetRTAudioModule(csound,name);
            int i,ndevs = csoundGetAudioDevList(csound,NULL,0);
            CS_AUDIODEVICE *devs = (CS_AUDIODEVICE *) malloc(ndevs*sizeof(CS_AUDIODEVICE));
            csoundGetAudioDevList(csound,devs,0);
            printf("Module %d:  %s (%s): %i devices\n", n, name, type, ndevs);
            for(i=0; i < ndevs; i++) {
                CU_ASSERT_PTR_NOT_NULL(devs[i].device_id);
                CU_ASSERT_PTR_NOT_NULL(devs[i].device_name);
//                CU_ASSERT_STRING_EQUAL(devs[i].rt_module, name);
                printf(" %d: %s (%s)\n", i, devs[i].device_id, devs[i].device_name);
            }
            free(devs);
        }
    }
    csoundDestroy(csound);
}

int key_callback_evt(void *userData, void *p,
                     unsigned int type)
{
    int *prev = (int *) userData;
    *((int *) p) = *prev;
    *prev += 1;
    return CSOUND_SUCCESS;
}

int key_callback_txt(void *userData, void *p,
                     unsigned int type)
{
    int *prev = (int *) userData;
    *((int *) p) =  *prev;
    *prev += 1;
    return CSOUND_SUCCESS;
}

void test_keyboard_io(void)
{
    int ret, err, prev = 100;
    CSOUND  *csound;
    csound = csoundCreate(NULL);
    ret = csoundRegisterKeyboardCallback(csound, key_callback_evt, &prev, CSOUND_CALLBACK_KBD_EVENT);
    CU_ASSERT(ret == CSOUND_SUCCESS);
    ret = csoundRegisterKeyboardCallback(csound, key_callback_txt, &prev, CSOUND_CALLBACK_KBD_TEXT);
    CU_ASSERT(ret == CSOUND_SUCCESS);
    const char  *instrument =
            "chn_k \"key\", 2\n"
            "instr 1 \n"
            "kkey sensekey\n"
            "chnset kkey, \"key\"\n"
            "kkey2, kdown2 sensekey\n"
            "chnset kkey2, \"key2\"\n"
            "chnset kdown2, \"down2\"\n"
//            "printk2 kkey2\n"
//            "printk2 kdown2\n"
            "endin \n";
    csoundCompileOrc(csound, instrument);
    csoundReadScore(csound, "i 1 0 1");
    ret = csoundStart(csound);
    CU_ASSERT(ret == CSOUND_SUCCESS);
    ret = csoundPerformKsmps(csound);
    CU_ASSERT(ret == CSOUND_SUCCESS);
    MYFLT val = csoundGetControlChannel(csound, "key", &err);
//    CU_ASSERT(err == CSOUND_SUCCESS);
    CU_ASSERT_DOUBLE_EQUAL(val, 100.0, 0.001);
    val = csoundGetControlChannel(csound, "key2", &err);
//    CU_ASSERT(err == CSOUND_SUCCESS);
    CU_ASSERT_DOUBLE_EQUAL(val, 101.0, 0.001);
    val = csoundGetControlChannel(csound, "down2", &err);
//    CU_ASSERT(err == CSOUND_SUCCESS);
    CU_ASSERT_DOUBLE_EQUAL(val, 1.0, 0.001);

    csoundDestroy(csound);
}


void test_audio_modules(void)
{
    CSOUND  *csound;
    csound = csoundCreate(NULL);
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
              CU_ASSERT(ret == 0);
            }
            if (ret == 0) {
              ret = csoundPerform(csound);
              CU_ASSERT(ret > 0);
            }
            csoundReset(csound);
        }
    }

}

void test_audio_hostbased(void)
{
    CSOUND  *csound;
    csound = csoundCreate(NULL);
    const char  *instrument =
            "ksmps = 256\n"
            "instr 1 \n"
            "asig oscil 0.1, 440\n"
            "out asig\n"
            "endin \n";
    csoundSetOption(csound, "-B4096");
    csoundCompileOrc(csound, instrument);
    csoundReadScore(csound, "i 1 0 0.1\n e 0.2");
//    csoundSetRTAudioModule(csound, "hostbased");
    csoundSetHostImplementedAudioIO(csound, 1, 256);
    csoundSetOutput(csound, "dac", NULL, NULL);
    int ret = csoundStart(csound);
    CU_ASSERT(ret == 0);
    ret = csoundPerform(csound);
    CU_ASSERT(ret > 0);
    csoundReset(csound);
}


void test_midi_modules(void)
{
    CSOUND  *csound;
    csound = csoundCreate(NULL);
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
            CU_ASSERT(ret == 0);
            ret = csoundPerform(csound);
            CU_ASSERT(ret > 0);
            csoundReset(csound);
        }
    }
}

void test_midi_hostbased(void)
{
    CSOUND  *csound;
    csound = csoundCreate(NULL);
    const char  *instrument =
            "ksmps = 256\n"
            "instr 1 \n"
            "asig oscil 0.1, 440\n"
            "out asig\n"
            "endin \n";
    csoundSetOption(csound, "-B4096");
    csoundCompileOrc(csound, instrument);
    csoundReadScore(csound, "i 1 0 0.1\n e 0.2");
//    csoundSetMIDIModule(csound, "hostbased");
    csoundSetHostImplementedMIDIIO(csound, 1);
    csoundSetOutput(csound, "dac", NULL, NULL);
    int ret = csoundStart(csound);
    CU_ASSERT(ret == 0);
    ret = csoundPerform(csound);
    CU_ASSERT(ret > 0);
    csoundReset(csound);
}


int main(int argc, char **argv)
{
    CU_pSuite pSuite = NULL;

    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
       return CU_get_error();

    /* add a suite to the registry */
    pSuite = CU_add_suite("Message Buffer Tests", init_suite1, clean_suite1);
    if (NULL == pSuite) {
       CU_cleanup_registry();
       return CU_get_error();
    }

    /* add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "Device Listing", test_dev_list))
            || (NULL == CU_add_test(pSuite, "Keyboard IO", test_keyboard_io))
            || (NULL == CU_add_test(pSuite, "Audio Modules", test_audio_modules))
            || (NULL == CU_add_test(pSuite, "Audio Hostbased", test_audio_hostbased))
            || (NULL == CU_add_test(pSuite, "MIDI Modules", test_midi_modules))
            || (NULL == CU_add_test(pSuite, "MIDI Hostbased", test_midi_hostbased))
        )
    {
       CU_cleanup_registry();
       return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();

}

