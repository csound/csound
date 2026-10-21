#include <pthread.h>
#include <stdint.h>
#if !defined(__wasm_atomics__) || !defined(__wasm_bulk_memory__)
#error WASI realtime requires -pthread and Wasm atomics
#endif
_Static_assert(__atomic_always_lock_free(sizeof(long), 0), "long atomics");
_Static_assert(__atomic_always_lock_free(sizeof(int64_t), 0), "64-bit atomics");
/* Single-thread wasi-libc supplies pthread_create/join stubs too. The actual
   WASI thread entry is present only in the threaded runtime. */
extern void wasi_thread_start(int, void *);
static void (*volatile thread_entry)(int, void *) = wasi_thread_start;
static void *worker(void *data) { return data; }
int main(void) {
    if (thread_entry == 0) return 1;
    pthread_t thread;
    void *result;
    if (pthread_create(&thread, 0, worker, 0)) return 1;
    return pthread_join(thread, &result);
}
