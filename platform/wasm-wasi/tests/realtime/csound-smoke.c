/* Hold an init opcode on its worker while the calling thread renders. */
#define _POSIX_C_SOURCE 200809L
#include "csoundCore.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        fprintf(stderr, "%s:%d: %s failed\n", __FILE__, __LINE__, #condition); \
        abort();                                                             \
    }                                                                        \
} while (0)

#define EVENTS 32

struct host {
    pthread_t render_thread;
    pthread_mutex_t lock;
    pthread_cond_t changed;
    int entered, released;
    _Atomic int markers, errors;
    int order[EVENTS];
};

struct marker_opcode {
    OPDS header;
    MYFLT *sequence;
};

static struct timespec deadline(void)
{
    struct timespec until;
    CHECK(clock_gettime(CLOCK_REALTIME, &until) == 0);
    until.tv_sec += 10;
    return until;
}

static int32_t hold_init(CSOUND *csound, void *opcode)
{
    (void) opcode;
    struct host *host = csoundGetHostData(csound);
    /* Fails promptly if --realtime was ignored or selected thread stubs. */
    CHECK(!pthread_equal(pthread_self(), host->render_thread));
    CHECK(pthread_mutex_lock(&host->lock) == 0);
    host->entered = 1;
    CHECK(pthread_cond_broadcast(&host->changed) == 0);
    const struct timespec until = deadline();
    while (!host->released)
        CHECK(pthread_cond_timedwait(&host->changed, &host->lock, &until) == 0);
    CHECK(pthread_mutex_unlock(&host->lock) == 0);
    return 0;
}

static int32_t mark_init(CSOUND *csound, void *opcode)
{
    struct host *host = csoundGetHostData(csound);
    struct marker_opcode *marker = opcode;
    CHECK(!pthread_equal(pthread_self(), host->render_thread));
    int count = atomic_load_explicit(&host->markers, memory_order_relaxed);
    CHECK(count < EVENTS);
    host->order[count] = (int) *marker->sequence;
    atomic_store_explicit(&host->markers, count + 1, memory_order_release);
    return 0;
}

static int32_t error_init(CSOUND *csound, void *opcode)
{
    (void) opcode;
    struct host *host = csoundGetHostData(csound);
    CHECK(!pthread_equal(pthread_self(), host->render_thread));
    atomic_fetch_add(&host->errors, 1);
    return csound->InitError(csound, "expected WASI smoke init error");
}

static int32_t fatal_init(CSOUND *csound, void *opcode)
{
    (void) opcode;
    struct host *host = csoundGetHostData(csound);
    CHECK(!pthread_equal(pthread_self(), host->render_thread));
    puts("WASI fatal init reached");
    fflush(stdout);
    csound->LongJmp(csound, 1);
    return -1;
}

static uintptr_t fatal_thread(void *data)
{
    return (uintptr_t) fatal_init(data, NULL);
}

static void run(CSOUND *csound, struct host *host, int fatal)
{
    static const char orchestra[] =
        "sr = 48000\n"
        "ksmps = 16\n"
        "nchnls = 1\n"
        "0dbfs = 1\n"
        "instr 1\n"
        "  test_wasi_mark p4\n"
        "  asig init 0.0078125\n"
        "  out asig\n"
        "endin\n"
        "instr 2\n"
        "  test_wasi_error\n"
        "endin\n"
        "instr 3\n"
        "  test_wasi_fatal\n"
        "endin\n"
        "instr 4\n"
        "  asig init 0.125\n"
        "  out asig\n"
        "endin\n"
        "instr 9\n"
        "  test_wasi_hold\n"
        "endin\n";
    host->entered = host->released = 0;
    atomic_store(&host->markers, 0);
    atomic_store(&host->errors, 0);
    CHECK(csoundSetOption(csound, "-n -d -m0 --daemon --realtime") == 0);
    CHECK(csoundAppendOpcode(csound, "test_wasi_hold", sizeof(OPDS), 0,
                            "", "", hold_init, NULL, NULL) == 0);
    CHECK(csoundAppendOpcode(csound, "test_wasi_mark",
                            sizeof(struct marker_opcode), 0,
                            "", "i", mark_init, NULL, NULL) == 0);
    CHECK(csoundAppendOpcode(csound, "test_wasi_error", sizeof(OPDS), 0,
                            "", "", error_init, NULL, NULL) == 0);
    CHECK(csoundAppendOpcode(csound, "test_wasi_fatal", sizeof(OPDS), 0,
                            "", "", fatal_init, NULL, NULL) == 0);
    CHECK(csoundCompileOrc(csound, orchestra, 0) == 0);
    CHECK(csoundStart(csound) == 0);
    if (fatal) {
        const MYFLT event[] = {3, 0, 60};
        csoundEvent(csound, CS_INSTR_EVENT, event, 3, 1);
        for (int attempt = 0; attempt < 5000; ++attempt) {
            CHECK(csoundPerformKsmps(csound) == 0);
            csoundSleep(1);
        }
        fputs("fatal init did not end the command\n", stderr);
        exit(2);
    }
    /* An existing voice must keep rendering while a later init waits. */
    const MYFLT steady[] = {4, 0, 60};
    csoundEvent(csound, CS_INSTR_EVENT, steady, 3, 1);
    int steady_rendered = 0;
    for (int attempt = 0; attempt < 5000; ++attempt) {
        CHECK(csoundPerformKsmps(csound) == 0);
        if (csoundGetSpout(csound)[0] == FL(0.125)) {
            steady_rendered = 1;
            break;
        }
        csoundSleep(1);
    }
    CHECK(steady_rendered);
    const MYFLT hold[] = {9, 0, 60};
    csoundEvent(csound, CS_INSTR_EVENT, hold, 3, 1);
    int entered = 0;
    for (int attempt = 0; attempt < 5000; ++attempt) {
        CHECK(csoundPerformKsmps(csound) == 0);
        CHECK(pthread_mutex_lock(&host->lock) == 0);
        entered = host->entered;
        CHECK(pthread_mutex_unlock(&host->lock) == 0);
        if (entered) break;
        csoundSleep(1);
    }
    CHECK(entered);

    const MYFLT error[] = {2, 0, 60};
    csoundEvent(csound, CS_INSTR_EVENT, error, 3, 1);

    /* The numeric API queue must copy the reused caller-owned p-fields. */
    MYFLT event[] = {1, 0, 60, 0};
    for (int i = 0; i < EVENTS; ++i) {
        event[3] = i;
        csoundEvent(csound, CS_INSTR_EVENT, event, 4, 1);
    }
    const int64_t before = csoundGetCurrentTimeSamples(csound);
    for (int block = 0; block < 100; ++block) {
        CHECK(csoundPerformKsmps(csound) == 0);
        for (uint32_t sample = 0; sample < csoundGetKsmps(csound); ++sample)
            CHECK(csoundGetSpout(csound)[sample] == FL(0.125));
    }
    CHECK(csoundGetCurrentTimeSamples(csound) == before + 100 * 16);
    CHECK(atomic_load_explicit(&host->markers, memory_order_acquire) == 0);

    CHECK(pthread_mutex_lock(&host->lock) == 0);
    host->released = 1;
    CHECK(pthread_cond_broadcast(&host->changed) == 0);
    CHECK(pthread_mutex_unlock(&host->lock) == 0);

    int rendered = 0;
    for (int attempt = 0; attempt < 5000; ++attempt) {
        CHECK(csoundPerformKsmps(csound) == 0);
        if (atomic_load_explicit(&host->markers, memory_order_acquire) == EVENTS &&
            csoundGetSpout(csound)[0] == FL(0.375)) {
            rendered = 1;
            break;
        }
        csoundSleep(1);
    }
    CHECK(rendered);
    CHECK(atomic_load(&host->errors) == 1);
    for (int i = 0; i < EVENTS; ++i)
        CHECK(host->order[i] == i);
    for (uint32_t i = 0; i < csoundGetKsmps(csound); ++i)
        CHECK(csoundGetSpout(csound)[i] == FL(0.375));
    /* Reset must join the old init and log workers before another start. */
    csoundReset(csound);
}

int main(int argc, char **argv)
{
    struct host host = {0};
    host.render_thread = pthread_self();
    CHECK(pthread_mutex_init(&host.lock, NULL) == 0);
    CHECK(pthread_cond_init(&host.changed, NULL) == 0);
    atomic_init(&host.markers, 0);
    atomic_init(&host.errors, 0);
    CSOUND *csound = csoundCreate(&host, NULL);
    CHECK(csound != NULL);
    const int fatal = argc == 2 && strcmp(argv[1], "--fatal-init") == 0;
    const int direct = argc == 2 && strcmp(argv[1], "--fatal-thread") == 0;
    CHECK(argc == 1 || fatal || direct);
    if (direct) {
        void *thread = csoundCreateThread2(fatal_thread, 0, csound);
        CHECK(thread != NULL);
        csoundJoinThread(thread);
        fputs("fatal worker did not end the command\n", stderr);
        return 2;
    }
    run(csound, &host, fatal);
    run(csound, &host, 0);
    csoundDestroy(csound);
    CHECK(pthread_cond_destroy(&host.changed) == 0);
    CHECK(pthread_mutex_destroy(&host.lock) == 0);
    puts("WASI Csound realtime smoke passed");
    return 0;
}
