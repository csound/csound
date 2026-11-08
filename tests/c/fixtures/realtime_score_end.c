/* Compile the real scheduler here so this test can count its sleep calls.
   The linker uses these definitions instead of musmon.o from the archive. */
#include "csoundCore.h"

static _Thread_local int32_t checking_audio_sleep;
static _Thread_local int32_t audio_sleep_calls;

/* The scheduler should have no sleep calls; keep the interceptor for regressions. */
static void __attribute__((unused)) checked_sleep(size_t milliseconds)
{
    if (checking_audio_sleep)
        audio_sleep_calls++;
    else
        csoundSleep(milliseconds);
}

#define csoundSleep checked_sleep
#include "../../../Engine/musmon.c"
#undef csoundSleep

int32_t csound_test_score_end_perform(CSOUND *csound, int32_t *sleep_calls)
{
    int32_t result;
    audio_sleep_calls = 0;
    checking_audio_sleep = 1;
    result = csoundPerformKsmps(csound);
    checking_audio_sleep = 0;
    *sleep_calls = audio_sleep_calls;
    return result;
}
