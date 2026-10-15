/* Historical model of Top/threads.c's GCC atomic spin-lock branch at
 * 9d6f33d345e5691a70de776973e76415357b50a0, before the native fix in #3431.
 * The bounded attempts keep this study probe safe to run on its own.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

int main(void) {
  char lock = 1; /* Another owner holds it throughout this probe. */
  char expected = 0;
  bool first = __atomic_compare_exchange_n(&lock, &expected, 1, false,
                                           __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  assert(!first && expected == 1);
  bool second = __atomic_compare_exchange_n(&lock, &expected, 1, false,
                                            __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  assert(second && lock == 1); /* The old loop mistakes this for ownership. */
  for (int attempt = 0; attempt < 2; attempt++) {
    expected = 0; /* Reset before each attempt, unlike the old loop. */
    assert(!__atomic_compare_exchange_n(&lock, &expected, 1, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
  }
  puts("CONFIRMED: unchanged expected value permits 1-to-1 acquisition of a held lock; resetting it rejects acquisition.");
  return 0;
}
