/* Capability probe only: one producer, one consumer, no Csound or libc. */
#include <stdatomic.h>
#include <stdint.h>

#define CAPACITY 64u
struct packet { uint32_t sequence, value; };
static struct {
  _Atomic uint32_t write, read;
  struct packet slots[CAPACITY];
} queue;

void queue_reset(void) {
  atomic_store_explicit(&queue.write, 0, memory_order_relaxed);
  atomic_store_explicit(&queue.read, 0, memory_order_relaxed);
}

int queue_push(uint32_t sequence, uint32_t value) {
  uint32_t write = atomic_load_explicit(&queue.write, memory_order_relaxed);
  uint32_t read = atomic_load_explicit(&queue.read, memory_order_acquire);
  if (write - read == CAPACITY) return 0;
  queue.slots[write & (CAPACITY - 1)] = (struct packet){sequence, value};
  atomic_store_explicit(&queue.write, write + 1, memory_order_release);
  return 1;
}

int64_t queue_pop(void) {
  uint32_t read = atomic_load_explicit(&queue.read, memory_order_relaxed);
  uint32_t write = atomic_load_explicit(&queue.write, memory_order_acquire);
  if (read == write) return -1;
  struct packet packet = queue.slots[read & (CAPACITY - 1)];
  atomic_store_explicit(&queue.read, read + 1, memory_order_release);
  return (int64_t)(((uint64_t)packet.sequence << 32) | packet.value);
}
