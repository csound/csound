<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
#include "libassert.orc"

instr 1
  ival = 7

  ;; bitwise AND in conditional context (if-then)
  if (ival & 4 == 4) then
    prints "PASS: ival & 4 == 4 is true\n"
  else
    prints "FAIL: ival & 4 == 4 should be true\n"
    exitnow(-1)
  endif

  if (ival & 2 == 2) then
    prints "PASS: ival & 2 == 2 is true\n"
  else
    prints "FAIL: ival & 2 == 2 should be true\n"
    exitnow(-1)
  endif

  if (ival & 1 == 1) then
    prints "PASS: ival & 1 == 1 is true\n"
  else
    prints "FAIL: ival & 1 == 1 should be true\n"
    exitnow(-1)
  endif

  ;; also test in elseif
  if (ival == 0) then
    prints "FAIL: should not be here\n"
    exitnow(-1)
  elseif (ival & 1 == 1) then
    prints "PASS: elseif with bitwise AND works\n"
  else
    prints "FAIL: elseif bitwise AND should work\n"
    exitnow(-1)
  endif

  prints "ALL BITWISE AND CONDITIONAL TESTS PASSED\n"
  exitnow 0
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
