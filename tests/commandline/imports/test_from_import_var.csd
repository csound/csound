<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test from import with variable */
from "simple_test.orc" import giTestValue

instr 1
    /* Access the imported variable directly */
    if (giTestValue != 42) then
        prints "FAIL: Expected giTestValue = 42, got %d\n", giTestValue
        exitnow 1
    endif
    prints "PASS: From import with variable works\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
