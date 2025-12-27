<CsoundSynthesizer>
<CsOptions>
-n -d -odebug 100
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

import "simple_test.orc" as stm

/* Try using the struct syntax, but stm is not a variable */
giLocal = 123

instr 1
    /* Verify the local variable is working */
    if (giLocal != 123) then
        prints "FAIL: Expected giLocal = 123, got %d\\n", giLocal
        exitnow 1
    endif
    prints "PASS: Debug test - local variable works\\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
