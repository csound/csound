<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test: assignment through module alias writes through to imported global */
import "simple_test.orc" as stm
from "simple_test.orc" import giTestValue

instr 1
    if (stm.giTestValue != 42) then
        prints "FAIL: Expected initial stm.giTestValue = 42, got %d\n", stm.giTestValue
        exitnow 1
    endif

    stm.giTestValue = 100

    if (stm.giTestValue != 100) then
        prints "FAIL: Expected assigned stm.giTestValue = 100, got %d\n", stm.giTestValue
        exitnow 1
    endif

    if (giTestValue != 100) then
        prints "FAIL: Expected imported giTestValue = 100 after alias assignment, got %d\n", giTestValue
        exitnow 1
    endif
endin

instr 2
    if (stm.giTestValue != 100) then
        prints "FAIL: Expected stm.giTestValue to persist at 100, got %d\n", stm.giTestValue
        exitnow 1
    endif

    prints "PASS: Aliased module variable assignment works and persists\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
i 2 0.2 0.1
e
</CsScore>
</CsoundSynthesizer>

