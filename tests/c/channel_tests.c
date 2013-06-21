#include <stdio.h>
#include <string.h>
#include <CUnit/Basic.h>
#include "csound.h"

int init_suite1(void)
{
    return 0;
}

int clean_suite1(void)
{
    return 0;
}

const char orc1[] = "chn_k \"testing\", 3\n  instr 1\n  endin\n";

void test_control_channel_params(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=NULL");
    int argc = 2;
    csoundCompileOrc(csound, orc1);
    int err = csoundStart(csound);
    CU_ASSERT(err == 0);
    controlChannelHints_t hints;
    hints.behav = CSOUND_CONTROL_CHANNEL_INT;
    hints.dflt = 5;
    hints.min = 1;
    hints.max = 10;
    hints.attributes = NULL;
    csoundSetControlChannelHints(csound, "testing", hints);

    controlChannelHints_t hints2;
    csoundGetControlChannelHints(csound, "testing", &hints2);
    CU_ASSERT(hints2.behav == CSOUND_CONTROL_CHANNEL_INT);
    CU_ASSERT(hints2.dflt == 5);
    CU_ASSERT(hints2.min == 1);
    CU_ASSERT(hints2.max == 10);

    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);
}

void test_control_channel(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=null");
    int argc = 2;
    csoundCompileOrc(csound, orc1);
    int err = csoundStart(csound);
    CU_ASSERT(err == 0);
    csoundSetControlChannel(csound, "testing", 5.0);
    CU_ASSERT_EQUAL(5.0, csoundGetControlChannel(csound, "testing", NULL));

    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);
}

const char orc2[] = "chn_k \"testing\", 3, 1, 1, 0, 10\n  chn_a \"testing2\", 3\n  instr 1\n  endin\n";

void test_channel_list(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=null");
    int argc = 2;
    csoundCompileOrc(csound, orc2);
    int err = csoundStart(csound);
    CU_ASSERT(err == 0);
    controlChannelInfo_t *lst;
    int numchnls = csoundListChannels(csound, &lst);
    CU_ASSERT(numchnls == 2);
    CU_ASSERT_STRING_EQUAL(lst->name, "testing");
    CU_ASSERT_EQUAL(lst->type, CSOUND_CONTROL_CHANNEL | CSOUND_INPUT_CHANNEL
                      | CSOUND_OUTPUT_CHANNEL | CSOUND_CONTROL_CHANNEL_INT)
    CU_ASSERT_STRING_EQUAL(lst[1].name, "testing2");

    csoundDeleteChannelList(csound, lst);
    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);
}

const char orc3[] = "instr 1\n kval invalue \"intest\"\n"
        "outvalue \"intest\",kval\n"
        "Sval invalue \"instrtest\"\n"
        "outvalue \"instrtest\",Sval\n"
        "endin\n"
        "instr 2\n outvalue \"outtest\", 10\n endin\n";

MYFLT val1, val2;
char strval[32];

void inputCallback(CSOUND *csound,
                   const char *channelName,
                   void *channelValuePtr,
                   void *channelType)
{
    if (strcmp(channelName, "intest") == 0 /*&& channelType == &CS_VAR_TYPE_K*/) {
        MYFLT *v = (MYFLT *) channelValuePtr;
        *v = 5.0;
    }
    if (strcmp(channelName, "instrtest") == 0 /*&& channelType == &CS_VAR_TYPE_S*/) {
        char *v = (char *) channelValuePtr;
        strcpy(v, "hello channels");
    }
}

void outputCallback(CSOUND *csound,
                   const char *channelName,
                   void *channelValuePtr,
                   void *channelType)
{
    if (strcmp(channelName, "intest") == 0 /*&& channelType == &CS_VAR_TYPE_K*/) {
        MYFLT *v = (MYFLT *) channelValuePtr;
        val1 = *v;
    }
    if (strcmp(channelName, "instrtest") == 0 /*&& channelType == &CS_VAR_TYPE_S*/) {
        char *v = (char *) channelValuePtr;
        strcpy(strval,v);
    }
    if (strcmp(channelName, "outtest") == 0 /*&& channelType == &CS_VAR_TYPE_K*/) {
        MYFLT *v = (MYFLT *) channelValuePtr;
        val2 = *v;
    }

}

void test_channel_callbacks(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=null");
    val1 = 0;
    csoundCompileOrc(csound, orc3);
    csoundSetInputChannelCallback(csound, (channelCallback_t) inputCallback);
    csoundSetOutputChannelCallback(csound, (channelCallback_t) outputCallback);
    int err = csoundStart(csound);
    CU_ASSERT(err == 0);
    MYFLT pFields[] = {1.0, 0.0, 1.0};
    err = csoundScoreEvent(csound, 'i', pFields, 3);
    MYFLT pFields2[] = {2.0, 0.0, 1.0};
    err = csoundScoreEvent(csound, 'i', pFields2, 3);
    CU_ASSERT(err == 0);
    err = csoundPerformKsmps(csound);
    CU_ASSERT(err == 0);
    CU_ASSERT_DOUBLE_EQUAL(val1, 5.0, 0.0000001);
    CU_ASSERT_DOUBLE_EQUAL(val2, 10.0, 0.0000001);
    CU_ASSERT_STRING_EQUAL(strval, "hello channels");
    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);
}


const char orc4[] = "chn_k \"1\", 3\n"
        "chn_k \"2\", 3\n"
        "chn_k \"3\", 3\n"
        "chn_k \"4\", 3\n"
        "instr 1\n kval invalue \"1\"\n"
        "outvalue \"2\",kval\n"
        "endin\n"

        "instr 2\n"
        "kval chani 2\n"
        "chano kval + 1, 3\n"
        "endin\n"

        "instr 3\n"
        "kval chnget \"3\"\n"
        "chnset kval + 1, \"4\"\n"
        "endin\n";

void inputCallback2(CSOUND *csound,
                   const char *channelName,
                   void *channelValuePtr,
                   void *channelType)
{
    MYFLT val = csoundGetControlChannel(csound, channelName, NULL);
    MYFLT *valPtr = (MYFLT *) channelValuePtr;
    *valPtr = val;
}

void outputCallback2(CSOUND *csound,
                   const char *channelName,
                   void *channelValuePtr,
                   void *channelType)
{
    MYFLT *valPtr = (MYFLT *) channelValuePtr;
    csoundSetControlChannel(csound, channelName, *valPtr);
}

void test_channel_opcodes(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=null");
    csoundCompileOrc(csound, orc4);
    csoundSetInputChannelCallback(csound, (channelCallback_t) inputCallback2);
    csoundSetOutputChannelCallback(csound, (channelCallback_t) outputCallback2);
    int err = csoundStart(csound);
    CU_ASSERT(err == 0);
    csoundSetControlChannel(csound, "1", 5.0);
    MYFLT pFields[] = {1.0, 0.0, 1.0};
    err = csoundScoreEvent(csound, 'i', pFields, 3);
    err = csoundPerformKsmps(csound);
    CU_ASSERT(err == 0);
    CU_ASSERT_EQUAL(5.0, csoundGetControlChannel(csound, "2", NULL));
    MYFLT pFields2[] = {2.0, 0.0, 1.0};
    err = csoundScoreEvent(csound, 'i', pFields2, 3);
    CU_ASSERT(err == 0);
    err = csoundPerformKsmps(csound);
    CU_ASSERT(err == 0);
    CU_ASSERT_EQUAL(6.0, csoundGetControlChannel(csound, "3", NULL));
    MYFLT pFields3[] = {3.0, 0.0, 1.0};
    err = csoundScoreEvent(csound, 'i', pFields3, 3);
    CU_ASSERT(err == 0);
    err = csoundPerformKsmps(csound);
    CU_ASSERT(err == 0);
    CU_ASSERT_EQUAL(7.0, csoundGetControlChannel(csound, "4", NULL));

    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);
}

const char orc5[] = "instr 1\n kval invalue \"1\"\n"
        "outvalue \"2\",kval\n"
        "endin\n"

        "instr 2\n"
        "kval chani 2\n"
        "chano kval + 1, 3\n"
        "endin\n"

        "instr 3\n"
        "kval chnget \"3\"\n"
        "chnset kval + 1, \"4\"\n"
        "endin\n";

void test_pvs_opcodes(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=null");
    csoundCompileOrc(csound, orc5);
    int err = csoundStart(csound);
    CU_ASSERT(err == 0);
    PVSDATEXT pvs_data, pvs_data2;
    pvs_data.N = 16;
    csoundSetPvsChannel(csound, &pvs_data, "1");
    csoundGetPvsChannel(csound, &pvs_data2, "1");
    CU_ASSERT_EQUAL(pvs_data.N, pvs_data2.N);

    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);
}

void test_invalid_channel(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=null");
    csoundCompileOrc(csound, orc5);

    int err;
    CU_ASSERT_EQUAL(0.0, csoundGetControlChannel(csound, "nonexistent_channel", &err));
    CU_ASSERT_NOT_EQUAL(err, CSOUND_SUCCESS);

    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);

}

const char orc6[] = "chn_k \"chan\", 3, 2, 0.5, 0, 1, 10, 10, 50, 100\n"
        "chn_k \"chan2\", 3, 2, 0.5, 0, 1, 10, 10, 50, 100, \"testattr\"\n"
        "chn_k \"chan3\", 3, 2, 0.5, 0, 1\n"
        "instr 1\n kval invalue \"1\"\n"
        "outvalue \"2\",kval\n"
        "endin\n";

void test_chn_hints(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=null");
    csoundCompileOrc(csound, orc6);
    int err = csoundStart(csound);
//    err = csoundPerformKsmps(csound); //Need this to load instr 0
    controlChannelHints_t hints;
    hints.attributes = 0;
    CU_ASSERT_EQUAL(0, csoundGetControlChannelHints(csound, "chan", &hints));
    CU_ASSERT_EQUAL(hints.x, 10);
    CU_ASSERT_EQUAL(hints.y, 10);
    CU_ASSERT_EQUAL(hints.width, 50);
    CU_ASSERT_EQUAL(hints.height, 100);
    CU_ASSERT_EQUAL(hints.attributes, 0);
    CU_ASSERT_EQUAL(0, csoundGetControlChannelHints(csound, "chan2", &hints));
    CU_ASSERT_EQUAL(hints.x, 10);
    CU_ASSERT_EQUAL(hints.y, 10);
    CU_ASSERT_EQUAL(hints.width, 50);
    CU_ASSERT_EQUAL(hints.height, 100);
    CU_ASSERT_STRING_EQUAL(hints.attributes, "testattr");

    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);
}


void test_string_channel(void)
{
    const char orcS[] = "chn_S \"testing\", 3\n  instr 1\n  endin\n";

    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundCreateMessageBuffer(csound, 0);
    csoundSetOption(csound, "--logfile=NULL");
    csoundCompileOrc(csound, orcS);
    int err = csoundStart(csound);
    CU_ASSERT(err == 0);

    
    csoundSetStringChannel(csound, "testing", "ttt");
    csoundCleanup(csound);
    csoundDestroyMessageBuffer(csound);
    csoundDestroy(csound);
}


int main()
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
   if ((NULL == CU_add_test(pSuite, "Channel Lists", test_channel_list))
           || (NULL == CU_add_test(pSuite, "Control channel", test_control_channel))
           || (NULL == CU_add_test(pSuite, "Control channel parameters", test_control_channel_params))
           || (NULL == CU_add_test(pSuite, "Callbacks", test_channel_callbacks))
           || (NULL == CU_add_test(pSuite, "Opcodes", test_channel_opcodes))
           || (NULL == CU_add_test(pSuite, "PVS Opcodes", test_pvs_opcodes))
           || (NULL == CU_add_test(pSuite, "Invalid channels", test_invalid_channel))
           || (NULL == CU_add_test(pSuite, "Channel hints", test_chn_hints))
           || (NULL == CU_add_test(pSuite, "String channel", test_string_channel))
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

