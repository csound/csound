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
    print stm.giTestValue

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
