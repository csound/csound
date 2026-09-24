/* Include the reader so the test can pause its pitch conversion without a
   hook in the engine. This object supplies diskin2 to this test executable. */
#include "csoundCore.h"
#include <math.h>
#include <stddef.h>
/* Count locks on the simulated audio thread. Init and worker calls still use
   real locks, so lifecycle tests can also exercise contention. */
static __thread int32_t checking_audio_locks;
static __thread int32_t audio_locks;
static void (*before_registry_lock)(void);
static spin_lock_t *paused_registry;
static void checked_spin_lock(spin_lock_t *lock)
{
    if (checking_audio_locks) { audio_locks++; return; }
    if (lock == paused_registry && before_registry_lock != NULL) {
        void (*callback)(void) = before_registry_lock;
        before_registry_lock = NULL;
        callback();
    }
    csoundSpinLock(lock);
}
static void checked_spin_unlock(spin_lock_t *lock)
{
    if (!checking_audio_locks) csoundSpinUnLock(lock);
}
static void checked_owner_lock(CSOUND *csound)
{
    if (checking_audio_locks) { audio_locks++; return; }
    async_instance_lock(csound);
}
static void checked_owner_unlock(CSOUND *csound)
{
    if (!checking_audio_locks) async_instance_unlock(csound);
}
static INSDS *synthetic_owner;
static int32_t synthetic_closes;
static void checked_file_close(CSOUND *csound, INSDS *owner)
{
    if (owner == synthetic_owner) {
        synthetic_closes++;
        owner->fdchp = NULL;
    }
    else fdchclose(csound, owner);
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
#define csoundSpinLock checked_spin_lock
#define csoundSpinUnLock checked_spin_unlock
#define async_instance_lock checked_owner_lock
#define async_instance_unlock checked_owner_unlock
#define fdchclose checked_file_close
#define __atomic_exchange_n test_control_exchange
#include "../../../OOps/diskin2.c"
#include "../../../InOut/circularbuffer.c"
#undef llrint
#undef csoundSpinLock
#undef csoundSpinUnLock
#undef async_instance_lock
#undef async_instance_unlock
#undef fdchclose
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

int32_t csound_test_diskin_audio_locks(int32_t use_array, int32_t stop)
{
    CSOUND *cs = csoundCreate(NULL, NULL);
    INSDS owner = {0};
    DISKIN2 reader = {0};
    DISKIN2_ARRAY array_reader = {0};
    ARRAYDAT outputs = {0};
    DISKIN2_ASYNC_STATE state = {0};
    DISKIN2_ASYNC_ENTRY entry = {0};
    MYFLT speed = FL(1.0), audio[1], output[1], sample = FL(0.5);
    void *cb = csoundCreateCircularBuffer(cs, 4, sizeof(MYFLT));
    owner.ksmps = 1;
    owner.actflg = 1;
    realtime_spin_lock_init(&cs->diskin2_async_lock);
    realtime_spin_lock_init(&entry.spinlock);
    cs->diskin2_async_state = &state;
#define PREPARE(p) do { \
    (p)->h.insdshead = &owner; \
    (p)->kTranspose = &speed; \
    (p)->initDone = (p)->nChannels = (p)->async = 1; \
    (p)->audioData.auxp = audio; \
    (p)->fdch.fd = (void *)1; \
    (p)->cb = cb; \
    (p)->asyncEntry = &entry; \
    entry.instance = p; \
    entry.stopRequested = &(p)->asyncStopRequested; \
    entry.instanceReaders = &(p)->asyncReaders; \
    diskin2_xf_setup(cs, &(p)->xf, 0, 0, 0, 1, speed); \
} while (0)
    if (use_array) {
        PREPARE(&array_reader);
        array_reader.aOut = &outputs;
        outputs.data = output;
        state.activeArrayEntries = state.activeArrayEntryTail = &entry;
    } else {
        PREPARE(&reader);
        reader.oChannels = 1;
        reader.out[0] = reader.aOut[0] = output;
        state.activeEntries = state.activeEntryTail = &entry;
    }
#undef PREPARE
    entry.owner = &owner;
    entry.borrowed = entry.active = 1;
    csoundWriteCircularBuffer(cs, cb, &sample, 1);
    audio_locks = 0;
    checking_audio_locks = 1;
    if (stop) {
        if (use_array) diskin2_async_deinit_array(cs, &array_reader);
        else diskin2_async_deinit(cs, &reader);
    } else {
        if (use_array) diskin2_perf_asynchronous_array(cs, &array_reader);
        else diskin2_perf_asynchronous(cs, &reader);
    }
    checking_audio_locks = 0;
    int32_t result = audio_locks;
    cs->diskin2_async_state = NULL;
    realtime_spin_lock_destroy(&entry.spinlock);
    realtime_spin_lock_destroy(&cs->diskin2_async_lock);
    csoundDestroyCircularBuffer(cs, cb);
    csoundDestroy(cs);
    return result;
}

static void *fixture_calloc(CSOUND *csound, size_t size)
{
    IGN(csound);
    return calloc(1, size);
}
static void fixture_free(CSOUND *csound, void *memory)
{
    IGN(csound);
    free(memory);
}

typedef struct {
    DISKIN2_ASYNC_ENTRY *entry;
    volatile int32_t ready;
    volatile int32_t finish;
} BORROW_TEST;

static uintptr_t fixture_worker(void *data)
{
    BORROW_TEST *worker = data;
    if (diskin2_acquire_async_instance(worker->entry) == NULL) {
        ATOMIC_SET(worker->ready, -1);
        return 1;
    }
    ATOMIC_SET(worker->ready, 1);
    while (!ATOMIC_GET(worker->finish)) csoundSleep(1);
    diskin2_release_async_instance(worker->entry);
    return 0;
}

static void *fixture_start_worker(BORROW_TEST *worker, DISKIN2_ASYNC_ENTRY *entry)
{
    worker->entry = entry;
    void *thread = csoundCreateThread(fixture_worker, worker);
    if (thread != NULL)
        while (!ATOMIC_GET(worker->ready)) csoundSleep(1);
    return thread;
}

static CSOUND *paused_reinit;
static void fixture_cleanup_during_reinit(void)
{
    diskin2_async_drain_deferred(paused_reinit);
}

/* Check the complete registration/stop/cleanup path. The borrowed case has
   both disk workers using the same owner, and releases them separately. */
int32_t csound_test_diskin_retirement(int32_t borrowed, int32_t reinit)
{
    CSOUND cs = {0};
    DISKIN2_ASYNC_STATE state = {0};
    DISKIN2 reader = {0};
    DISKIN2_ARRAY array_reader = {0};
    INSDS owner = {0};
    FDCH file = {0};
    DISKIN2_ASYNC_ENTRY *entry, *array_entry = NULL;
    BORROW_TEST worker = {0}, array_worker = {0};
    void *thread = NULL, *array_thread = NULL;
    int32_t result = 1;
    cs.Calloc = fixture_calloc;
    cs.Free = fixture_free;
    cs.diskin2_async_state = &state;
    realtime_spin_lock_init(&cs.diskin2_async_lock);
    state.running = state.arrayRunning = 1; /* No real worker needed. */
    owner.actflg = 1;
    owner.fdchp = &file;
    reader.h.insdshead = array_reader.h.insdshead = &owner;
    synthetic_owner = &owner;
    synthetic_closes = 0;
    result &= diskin2_add_instance(&cs, &reader) == OK;
    entry = reader.asyncEntry;
    result &= owner.async_ref_count == 1 && reader.asyncReaders == 1;
    if (borrowed) {
        thread = fixture_start_worker(&worker, entry);
        result &= thread != NULL && ATOMIC_GET(worker.ready) == 1;
        if (!reinit) {
            result &= diskin2_add_array_instance(&cs, &array_reader) == OK;
            array_entry = array_reader.asyncEntry;
            array_thread = fixture_start_worker(&array_worker, array_entry);
            result &= array_thread != NULL && ATOMIC_GET(array_worker.ready) == 1;
            result &= owner.async_ref_count == 2;
        }
    }
    if (reinit) {
        /* The engine marks a note inactive while its queued reinit runs. */
        ATOMIC_SET8(owner.actflg, 0);
        paused_reinit = &cs;
        paused_registry = &cs.diskin2_async_lock;
        before_registry_lock = fixture_cleanup_during_reinit;
        diskin2_remove_instance(&cs, &reader, 0);
        paused_registry = NULL;
    }
    else {
        checking_audio_locks = 1;
        audio_locks = 0;
        diskin2_async_deinit(&cs, &reader);
        if (array_entry != NULL) diskin2_async_deinit_array(&cs, &array_reader);
        checking_audio_locks = 0;
        result &= audio_locks == 0;
        ATOMIC_SET8(owner.actflg, 0);
        result &= !instance_is_reclaimable(&owner);
    }
    diskin2_async_drain_deferred(&cs);
    if (borrowed) {
        result &= synthetic_closes == 0 && reader.asyncReaders == 1;
        result &= owner.async_ref_count == (array_entry != NULL ? 2 : 1);
        ATOMIC_SET(worker.finish, 1);
        if (thread != NULL) result &= csoundJoinThread(thread) == 0;
        diskin2_async_drain_deferred(&cs);
        if (array_entry != NULL) {
            result &= synthetic_closes == 0 && owner.async_ref_count == 1;
            ATOMIC_SET(array_worker.finish, 1);
            if (array_thread != NULL) result &= csoundJoinThread(array_thread) == 0;
            diskin2_async_drain_deferred(&cs);
        }
    }
    result &= reader.asyncReaders == 0 && reader.asyncEntry == NULL;
    result &= owner.async_ref_count == 0;
    result &= synthetic_closes == (reinit ? 0 : 1);
    if (reinit) {
        result &= owner.fdchp == &file;
        result &= !diskin2_begin_async_init(&cs, 1, &owner,
                                            &reader.asyncState,
                                            &reader.asyncStopRequested);
        result &= diskin2_add_instance(&cs, &reader) == OK;
        result &= reader.asyncEntry == entry && owner.async_ref_count == 1;
        diskin2_async_deinit(&cs, &reader);
        ATOMIC_SET8(owner.actflg, 0);
        diskin2_async_drain_deferred(&cs);
        result &= owner.async_ref_count == 0 && synthetic_closes == 1;
    }
    result &= state.activeEntries == NULL && state.activeArrayEntries == NULL;
    result &= state.deferredCloses == NULL;
    diskin2_free_entries(&cs, state.entries);
    diskin2_free_entries(&cs, state.arrayEntries);
    realtime_spin_lock_destroy(&cs.diskin2_async_lock);
    synthetic_owner = NULL;
    return result;
}
