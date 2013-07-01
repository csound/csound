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
    csoundInitialize(0,0,0);
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
//                printf(" %d: %s (%s)\n", i, devs[i].device_id, devs[i].device_name);
            }
            free(devs);
        }
    }
    csoundDestroy(csound);
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

