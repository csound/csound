// Helpers for instanced loop tests.

// Fails the test when two signals differ in any sample of the current cycle.
opcode assertSameSignal(got:a, want:a, label:S):void
  diff:k = max_k(got - want, 1, 1)
  if diff > 1e-12 then
    printks("%s: signals differ by %g\n", 0, label, diff)
    exitnowk(-1)
  endif
endop

// Fails the test at note end if the signal never left zero, so that two
// silent signals cannot pass assertSameSignal by accident.
opcode assertNotSilent(sig:a, label:S):void
  peak:k init 0
  peak = max(peak, max_k(sig, 1, 1))
  if lastcycle() == 1 && peak == 0 then
    printks("%s: signal is silent\n", 0, label)
    exitnowk(-1)
  endif
endop
