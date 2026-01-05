<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; Test: Import instrument from module and verify it executes
; Also includes baseline test with local instrument to verify same behavior

; Import the test instrument from the module
from "synth_module.orc" import TestInstr, giExecutionCount

; Counters for both imported and local instruments
giLocalCount init 0

; Local test instrument (baseline - same as in module)
instr LocalTestInstr
  giLocalCount += 1
  prints "LocalTestInstr executed: count=%d\n", giLocalCount
endin

; Main instrument that schedules both imported and local instruments
instr Main
  prints "Main: Testing imported instrument\n"
  schedule TestInstr, 0, 0.01
  schedule TestInstr, 0.02, 0.01

  prints "Main: Testing local instrument (baseline)\n"
  schedule LocalTestInstr, 0.04, 0.01
  schedule LocalTestInstr, 0.06, 0.01

  prints "Main: Scheduled all test instances\n"
endin

; Verify execution counts and exit
instr Verify
  prints "Verify: giExecutionCount (imported) = %d\n", giExecutionCount
  prints "Verify: giLocalCount (local) = %d\n", giLocalCount

  if (giExecutionCount == 2 && giLocalCount == 2) then
    prints "PASS: Both imported and local instruments executed correctly\n"
    exitnow
  else
    prints "FAIL: Expected 2 executions each, got imported=%d, local=%d\n", giExecutionCount, giLocalCount
    exitnow 1
  endif
endin

</CsInstruments>
<CsScore>
i "Main" 0 0.1
i "Verify" 0.15 0.01
</CsScore>
</CsoundSynthesizer>
