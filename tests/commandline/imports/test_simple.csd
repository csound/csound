<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

import simple_test

instr 1
    aout = TestOsc(440)
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
