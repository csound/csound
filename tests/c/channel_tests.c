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

void test_control_channel(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    int argc = 2;
    csoundCompileOrc(csound, orc1);
    csoundStart(csound);
    csoundSetControlChannel(csound, "testing", 5.0);
    CU_ASSERT_EQUAL(5.0, csoundGetControlChannel(csound, "testing"));

    csoundDestroy(csound);
    csoundDestroyMessageBuffer(csound);
}

const char orc2[] = "chn_k \"testing\", 3, 1, 0, 10\n  chn_a \"testing2\", 3\n  instr 1\n  endin\n";

void test_channel_list(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    int argc = 2;
    csoundCompileOrc(csound, orc2);
    csoundStart(csound);
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

