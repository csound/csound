/* Include the reader so the test can pause its pitch conversion without a
   hook in the engine. This object supplies diskin2 to this test executable. */
#include "csoundCore.h"
#include <math.h>
#include <stddef.h>
/* These tests call only perf/control/reader paths. A lock here is a realtime
   regression, even if an uncontended lock would let the output test pass. */
static void forbidden_control_lock(spin_lock_t *lock)
{
    IGN(lock);
    abort();
}
static void (*after_control_exchange)(void);
static int32_t test_control_exchange(int32_t *slot, int32_t value, int order)
{
    int32_t previous = __atomic_exchange_n(slot, value, order);
    if (after_control_exchange != NULL) {
        void (*callback)(void) = after_control_exchange;
        after_control_exchange = NULL;
        callback();
    }
    return previous;
}
static void (*after_pitch_read)(void);
static long long test_llrint(double value)
{
    if (after_pitch_read != NULL) {
        void (*callback)(void) = after_pitch_read;
        after_pitch_read = NULL;
        callback();
    }
    return llrint(value);
}
#define llrint test_llrint
#define csoundSpinLock forbidden_control_lock
#define __atomic_exchange_n test_control_exchange
#include "../../../OOps/diskin2.c"
#undef llrint
#undef csoundSpinLock
#undef __atomic_exchange_n

static CSOUND *test_csound;
static DISKIN2 *scalar;
static DISKIN2_ARRAY *array;
static MYFLT pitch;

static int32_t free_frames(CSOUND *csound, void *cb, int32_t write)
{
    return 2;
}

static int32_t write_frames(CSOUND *csound, void *cb, const void *data,
                            int32_t count)
{
    return count;
}

static int32_t read_frames(CSOUND *csound, void *cb, void *data, int32_t count)
{
    memset(data, 0, count * sizeof(MYFLT));
    return count;
}

static void publish_step(void)
{
    pitch = FL(2.0);
    if (scalar != NULL)
        diskin2_perf_asynchronous(test_csound, scalar);
    else
        diskin2_perf_asynchronous_array(test_csound, array);
}

/* Both polls capture at the loop start. A step published after the first
   pitch read must invalidate that old-speed head on the next poll. */
int64_t csound_test_diskin_pitch_step(int32_t use_array)
{
    CSOUND cs = {0};
    INSDS instance = {0};
    DISKIN2 scalar_reader = {0};
    DISKIN2_ARRAY array_reader = {0};
    ARRAYDAT output_array = {0};
    MYFLT file[64] = {0}, worker_output[2], perf_output[1], audio[1], head[2];
    int64_t result;
    cs.CheckCircularBuffer = free_frames;
    cs.WriteCircularBuffer = write_frames;
    cs.ReadCircularBuffer = read_frames;
    instance.ksmps = 1;
    test_csound = &cs;
    pitch = FL(1.0);
    scalar = use_array ? NULL : &scalar_reader;
    array = use_array ? &array_reader : NULL;

#define SETUP(p) do {                                                   \
    diskin2_xf_setup(&cs, &(p)->xf, 0, FL(0.0), 0, 1, pitch);           \
    (p)->h.insdshead = &instance;                                       \
    (p)->kTranspose = &pitch;                                          \
    (p)->initDone = 1;                                                  \
    (p)->nChannels = 1;                                                 \
    (p)->bufSize = (p)->loopEnd = (p)->loopLength = 64;                  \
    (p)->wrapMode = (p)->winSize = 1;                                   \
    (p)->warpScale = 1.0;                                              \
    (p)->fdch.fd = (void *)1;                                          \
    (p)->buf = file;                                                    \
    (p)->aOut_buf = worker_output;                                     \
    (p)->auxData2.size = sizeof(worker_output);                         \
    (p)->audioData.auxp = audio;                                       \
    (p)->xf.len = 2;                                                   \
    (p)->xf.buf = head;                                                \
} while (0)
    if (use_array) {
        SETUP(array);
        output_array.data = perf_output;
        array->aOut = &output_array;
        after_pitch_read = publish_step;
        diskin_file_read_array_xfade(&cs, array);
        array->pos_frac = 0;
        diskin_file_read_array_xfade(&cs, array);
        result = array->xf.headEnd;
    }
    else {
        SETUP(scalar);
        scalar->oChannels = 1;
        scalar->out[0] = scalar->aOut[0] = perf_output;
        after_pitch_read = publish_step;
        diskin_file_read_xfade(&cs, scalar);
        scalar->pos_frac = 0;
        diskin_file_read_xfade(&cs, scalar);
        result = scalar->xf.headEnd;
    }
#undef SETUP
    return result;
}

typedef struct {
    CSOUND csound;
    DISKIN2_XF xf;
} TEST_CONTROL;

void *csound_test_diskin_control_create(void)
{
    TEST_CONTROL *test = calloc(1, sizeof(TEST_CONTROL));
    if (test == NULL)
        return NULL;
    diskin2_xf_setup(&test->csound, &test->xf, 0, FL(0.0), 0, 1, FL(1.0));
    return test;
}

void csound_test_diskin_control_destroy(void *context)
{
    TEST_CONTROL *test = context;
    free(test);
}

void csound_test_diskin_control_publish(void *context, double transpose)
{
    TEST_CONTROL *test = context;
    diskin2_publish_control(&test->xf, (MYFLT)transpose);
}

double csound_test_diskin_control_read(void *context, int32_t *reset)
{
    TEST_CONTROL *test = context;
    return diskin2_read_control(&test->xf, reset);
}

static void *paused_control;
static void publish_while_reader_owns_slot(void)
{
    csound_test_diskin_control_publish(paused_control, 3);
    csound_test_diskin_control_publish(paused_control, 3);
    csound_test_diskin_control_publish(paused_control, 4);
}

void csound_test_diskin_pause_control_read(void *context)
{
    paused_control = context;
    after_control_exchange = publish_while_reader_owns_slot;
}
