<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test that from import only imports what's specified */
from "simple_test.orc" import TestOsc

instr 1
    /* This should work - TestOsc is explicitly imported */
    aout TestOsc 440
    
    /* This should fail - giTestValue is NOT imported */
    print giTestValue
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
