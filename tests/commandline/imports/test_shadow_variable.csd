<CsoundSynthesizer>
<CsOptions>
-odac -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; Import only giVar1 - giVar2 should NOT be imported
from "selective_test_module.orc" import giVar1

; Try to define our own giVar2 (same name as non-imported module variable)
; Since giVar2 is NOT imported, we should be able to define our own
giVar2 = 999

instr 1
    prints "giVar1 (imported) = %d\n", giVar1
    prints "giVar2 (local) = %d\n", giVar2

    ; giVar1 should be 100 from module, giVar2 should be 999 (our local)
    if (giVar1 == 100 && giVar2 == 999) then
        prints "=== Shadow Test PASSED ===\n"
    else
        prints "=== Shadow Test FAILED ===\n"
    endif
    turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
