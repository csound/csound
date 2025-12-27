<CsoundSynthesizer>
<CsOptions>
-d -n
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Import the simple test module */
import "test_simple.orc"

/* Test instrument that uses the imported functionality */
instr 1
    iResult = TestConst(123)
    if (iResult != 123) then
        prints "FAIL: Expected TestConst(123) = 123, got %d\n", iResult
        exitnow 1
    endif
    prints "PASS: Simple import test\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>
