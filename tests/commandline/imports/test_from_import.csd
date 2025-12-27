<CsoundSynthesizer>
<CsOptions>
-d -n
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Test from module import syntax */
from "oscillators.orc" import SimpleOsc
from "test_simple.orc" import TestConst

/* Test instrument that uses selectively imported functionality */
instr 1
    aout = SimpleOsc(0.5, 440, 0)
    iResult = TestConst(123)
    if (iResult != 123) then
        prints "FAIL: Expected TestConst(123) = 123, got %d\\n", iResult
        exitnow 1
    endif
    prints "PASS: From import syntax\\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>
