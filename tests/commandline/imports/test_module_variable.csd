<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

import "simple_test.orc"

instr 1
    /* Access the imported module variable (g-prefix) */
    if (giTestValue != 42) then
        prints "FAIL: Expected giTestValue = 42, got %d\n", giTestValue
        exitnow 1
    endif

    /* Access the @global annotated variable */
    if (myGlobalVar != 100) then
        prints "FAIL: Expected myGlobalVar = 100, got %d\n", myGlobalVar
        exitnow 1
    endif

    prints "PASS: Module variable access (g-prefix AND @global)\n"

    /* Also use the UDO to verify imports work */
    aout = TestOsc(440)
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
