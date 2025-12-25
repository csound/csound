<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test: import with alias and qualified variable access */
import "simple_test.orc" as stm

instr 1
    /* Test 1: Access module variable via qualified alias */
    iVal = stm.giTestValue

    /* Verify the value is correct */
    if (iVal == 42) then
        prints "PASS: stm.giTestValue = 42\n"
    else
        prints "FAIL: expected 42, got %f\n", iVal
        exitnow 1
    endif

    /* Test 2: Use UDO from the module (UDOs are currently imported into global namespace) */
    aout TestOsc 440
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
