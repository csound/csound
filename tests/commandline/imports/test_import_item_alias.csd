<CsoundSynthesizer>
<CsOptions>
-o /dev/null
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test: from ... import X as Y syntax for aliased imports */
from "simple_test.orc" import giTestValue as localVal

instr 1
    /* Access the imported variable using its local alias */
    prints "localVal = %d\n", localVal
    
    if (localVal != 42) then
        prints "FAIL: Expected localVal = 42, got %d\n", localVal
        exitnow 1
    endif
    
    prints "PASS: Import alias works correctly\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
