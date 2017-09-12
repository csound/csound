#include <stdio.h>
#include <stdlib.h>
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

#include  "cscore.h"
void cscore(CSOUND *cs)
{
      EVENT  *e, *f;
      EVLIST *a, *b;
      int n;
      a = cscoreListGetSection(cs);            /* read score into event list "a" */
      b = cscoreListSeparateF(cs, a);          /* separate f statements */
      cscoreListPut(cs, b);                    /* write f statements out to score */
      cscoreListFreeEvents(cs, b);             /* and release the spaces used */
      e = cscoreDefineEvent(cs, "t 0 120");    /* define event for tempo statement */
      cscorePutEvent(cs, e);                   /* write tempo statement to score */
      cscoreListPut(cs, a);                    /* write the notes */
      cscorePutString(cs, "s");                /* section end */
      cscorePutEvent(cs, e);                   /* write tempo statement again */
      b = cscoreListCopyEvents(cs, a);         /* make a copy of the notes in "a" */
      for (n = 1; n <= b->nevents; n++)        /* iterate the following lines nevents times: */
      {
          f = b->e[n];
          f->p[5] *= 0.5;                      /* transpose pitch down one octave */
      }
      a = cscoreListAppendList(cs, a, b);      /* now add these notes to original pitches */
      cscoreListPut(cs, a);
      cscorePutString(cs, "e");
      /*EVLIST *ev = */ cscoreListGetUntil(cs, 100);


      return;
}

void test_cscore(void)
{
    csoundSetGlobalEnv("OPCODE6DIR64", "../../");
    CSOUND *csound = csoundCreate(0);

    FILE *in_file = fopen("cscore_score.sco", "r");
    FILE *out_file = fopen("cscore_out.sco", "w");
    csoundInitializeCscore(csound, in_file, out_file);
    cscore(csound);
    csoundCleanup(csound);
    csoundDestroy(csound);

    CU_ASSERT(1);

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
   if ((NULL == CU_add_test(pSuite, "Cscore test", test_cscore))
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

