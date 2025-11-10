<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

import nested_module

instr 1
    aout = NestedOsc(440)
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
