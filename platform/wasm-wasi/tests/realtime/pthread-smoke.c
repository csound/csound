/* Exercise the libc runtime, not just WebAssembly atomic instructions. */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
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

#define STACK_BYTES 32768
#define ITERATIONS 2000

static _Thread_local int local_value = 7;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static int ready, released, total;

struct worker_data {
    int value;
    uintptr_t stack_address;
    uintptr_t tls_address;
};

static struct timespec deadline(void)
{
    struct timespec until;
    CHECK(clock_gettime(CLOCK_REALTIME, &until) == 0);
    until.tv_sec += 10;
    return until;
}

static void *worker(void *argument)
{
    struct worker_data *data = argument;
    volatile unsigned char stack_data[STACK_BYTES];
    CHECK(local_value == 7);
    local_value = data->value;
    for (size_t i = 0; i < sizeof(stack_data); ++i)
        stack_data[i] = (unsigned char) data->value;

    CHECK(pthread_mutex_lock(&lock) == 0);
    data->stack_address = (uintptr_t) &stack_data[0];
    data->tls_address = (uintptr_t) &local_value;
    ++ready;
    CHECK(pthread_cond_broadcast(&changed) == 0);
    const struct timespec until = deadline();
    while (!released)
        CHECK(pthread_cond_timedwait(&changed, &lock, &until) == 0);
    CHECK(pthread_mutex_unlock(&lock) == 0);

    for (int i = 0; i < ITERATIONS; ++i) {
        const size_t size = (size_t) (64 + i % 997);
        unsigned char *allocation = malloc(size);
        CHECK(allocation != NULL);
        memset(allocation, local_value, size);
        CHECK(pthread_mutex_lock(&lock) == 0);
        total += local_value;
        CHECK(pthread_mutex_unlock(&lock) == 0);
        for (size_t j = 0; j < size; ++j)
            CHECK(allocation[j] == local_value);
        free(allocation);
    }
    for (size_t i = 0; i < sizeof(stack_data); ++i)
        CHECK(stack_data[i] == data->value);
    CHECK(local_value == data->value);
    return (void *) (intptr_t) local_value;
}

int main(void)
{
    struct worker_data data[2] = {{.value = 3}, {.value = 5}};
    pthread_t threads[2];
    void *results[2];
    for (int i = 0; i < 2; ++i)
        CHECK(pthread_create(&threads[i], NULL, worker, &data[i]) == 0);

    CHECK(pthread_mutex_lock(&lock) == 0);
    const struct timespec until = deadline();
    while (ready != 2)
        CHECK(pthread_cond_timedwait(&changed, &lock, &until) == 0);
    CHECK(data[0].tls_address != data[1].tls_address);
    CHECK(data[0].tls_address != (uintptr_t) &local_value);
    CHECK(data[1].tls_address != (uintptr_t) &local_value);
    CHECK(data[0].stack_address + STACK_BYTES <= data[1].stack_address ||
          data[1].stack_address + STACK_BYTES <= data[0].stack_address);
    released = 1;
    CHECK(pthread_cond_broadcast(&changed) == 0);
    CHECK(pthread_mutex_unlock(&lock) == 0);

    for (int i = 0; i < 2; ++i) {
        CHECK(pthread_join(threads[i], &results[i]) == 0);
        CHECK((intptr_t) results[i] == data[i].value);
    }
    CHECK(local_value == 7);
    CHECK(total == (3 + 5) * ITERATIONS);
    CHECK(pthread_cond_destroy(&changed) == 0);
    CHECK(pthread_mutex_destroy(&lock) == 0);
    puts("WASI pthread smoke passed");
    return 0;
}
