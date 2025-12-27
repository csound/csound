<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Test import with alias syntax */
import "simple_test.orc" as stm

instr 1
    /* Access module variable via alias */
    iVal = stm.giTestValue

    /* Verify the value is correct */
    if (iVal != 42) then
        prints "FAIL: Expected stm.giTestValue = 42, got %d\n", iVal
        exitnow 1
    endif
    prints "PASS: Import with alias syntax works\n"

    /* Use UDO from the aliased module */
    aout = TestOsc(440)
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
