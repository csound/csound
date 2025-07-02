#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "csound.h"
extern "C" {
  #include "cscore.h"
}

void cscore(CSOUND *cs)
{
    EVENT  *e, *f;
    EVLIST *a, *b;
    int32_t n;

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
    cscoreListGetUntil(cs, 100);
}

// TODO: fix on apple
// TEST (CScoreTests, testCscore)
// {
//     csoundSetGlobalEnv("OPCODE6DIR64", "../../");
//     CSOUND *csound = csoundCreate(NULL,NULL);

//     FILE *in_file = fopen("cscore_score.sco", "r");
//     FILE *out_file = fopen("cscore_out.sco", "w");
//     csoundInitializeCscore(csound, in_file, out_file);
//     cscore(csound);
//     csoundCleanup(csound);
//     csoundDestroy(csound);

//     ASSERT_TRUE (true);
// }
