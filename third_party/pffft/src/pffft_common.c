
#include "pffft.h"

#include <stdint.h>
#include <stdlib.h>

/* SSE and co like 16-bytes aligned pointers
 * with a 64-byte alignment, we are even aligned on L2 cache lines... */
#define MALLOC_V4SF_ALIGNMENT 64
#define EXTRA_BYTES_FOR_ALIGNMENT_AND_A_POINTER (MALLOC_V4SF_ALIGNMENT - 1 + sizeof(void*))

static void * Valigned_malloc(size_t nb_bytes) {
  void *p, *p0 = malloc(nb_bytes + EXTRA_BYTES_FOR_ALIGNMENT_AND_A_POINTER);
  if (!p0) return (void *) 0;
  p = (void *) (((uintptr_t) p0 + EXTRA_BYTES_FOR_ALIGNMENT_AND_A_POINTER) & (~((uintptr_t) (MALLOC_V4SF_ALIGNMENT-1))));
  *((void **) p - 1) = p0;
  return p;
}

static void Valigned_free(void *p) {
  if (p) free(*((void **) p - 1));
}


static int next_power_of_two(int N) {
  /* https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
  /* compute the next highest power of 2 of 32-bit v */
  unsigned v = N;
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

static int is_power_of_two(int N) {
  /* https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2 */
  int f = N && !(N & (N - 1));
  return f;
}



PFFFT_EXPORT void *pffft_aligned_malloc(size_t nb_bytes) { return Valigned_malloc(nb_bytes); }
PFFFT_EXPORT void pffft_aligned_free(void *p) { Valigned_free(p); }
PFFFT_EXPORT int pffft_next_power_of_two(int N) { return next_power_of_two(N); }
PFFFT_EXPORT int pffft_is_power_of_two(int N) { return is_power_of_two(N); }

PFFFT_EXPORT void *pffftd_aligned_malloc(size_t nb_bytes) { return Valigned_malloc(nb_bytes); }
PFFFT_EXPORT void pffftd_aligned_free(void *p) { Valigned_free(p); }
PFFFT_EXPORT int pffftd_next_power_of_two(int N) { return next_power_of_two(N); }
PFFFT_EXPORT int pffftd_is_power_of_two(int N) { return is_power_of_two(N); }
