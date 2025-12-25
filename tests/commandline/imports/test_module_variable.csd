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
    /* Try to access the imported module variable */
    print giTestValue
    
    /* Try to access the @global annotated variable */
    print myGlobalVar
    
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
