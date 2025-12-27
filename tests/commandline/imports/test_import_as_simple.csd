<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Test import with alias syntax - simpler test */
import "simple_test.orc" as stm

instr 1
    /* Access module variable via alias and assign to local */
    iVal = stm.giTestValue

    if (iVal != 42) then
        prints "FAIL: Expected stm.giTestValue = 42, got %d\n", iVal
        exitnow 1
    endif
    prints "PASS: Import as simple - qualified variable access\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
