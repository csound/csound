<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1

asig   crunch 0.8, 0.1, 7, p4
       outs asig, asig

endin

</CsInstruments>
<CsScore>

i1 0 1 .9
i1 1 1 .1

e
</CsScore>
</CsoundSynthesizer>
