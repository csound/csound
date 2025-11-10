<CsoundSynthesizer>
<CsOptions>
-d -n
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Test from module import syntax */
from oscillators import SimpleOsc
from test_simple import TestConst

/* Test instrument that uses selectively imported functionality */
instr 1
    aout = SimpleOsc(0.5, 440, 0)
    iResult = TestConst(123)
    print iResult
endin
</CsInstruments>

<CsScore>
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>