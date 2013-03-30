#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
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
    csoundSetControlChannelHints(csound, "testing", hints);

    controlChannelHints_t hints2;
    csoundGetControlChannelHints(csound, "testing", &hints2);
    CU_ASSERT(hints2.behav == CSOUND_CONTROL_CHANNEL_INT);
    CU_ASSERT(hints2.dflt == 5);
    CU_ASSERT(hints2.min == 1);
    CU_ASSERT(hints2.max == 10);

    csoundDestroy(csound);
    csoundDestroyMessageBuffer(csound);
}

void test_control_channel(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundSetOption(csound, "--logfile=NULL");
    int argc = 2;
    csoundCompileOrc(csound, orc1);
    int err = csoundStart(csound);
    CU_ASSERT(err == 0);
    csoundSetControlChannel(csound, "testing", 5.0);
    CU_ASSERT_EQUAL(5.0, csoundGetControlChannel(csound, "testing"));

    csoundDestroy(csound);
    csoundDestroyMessageBuffer(csound);
}

const char orc2[] = "chn_k \"testing\", 3, 1, 1, 0, 10\n  chn_a \"testing2\", 3\n  instr 1\n  endin\n";

void test_channel_list(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    csoundSetOption(csound, "--logfile=NULL");
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

    csoundDestroy(csound);
    csoundDestroyMessageBuffer(csound);
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
    int argc = 2;
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

