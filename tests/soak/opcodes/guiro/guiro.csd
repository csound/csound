<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

a1  guiro .8, p4
    outs a1, a1

endin
</CsInstruments>
<CsScore>

i1 0 1  1
i1 + 1 .01
e
</CsScore>
</CsoundSynthesizer>
