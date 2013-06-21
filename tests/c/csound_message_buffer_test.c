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

void test_create_buffer(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);
    int argc = 2;
    char *argv[] = {"csound", "-v"};
    csoundEnableMessageBuffer(csound, 1);
    int result = csoundCompile(csound, argc, argv);

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
   if ((NULL == CU_add_test(pSuite, "Create Message Buffer", test_create_buffer))
//           || (NULL == CU_add_test(pSuite, "test of fread()", testFREAD))
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

