<CsoundSynthesizer>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

instr 1

idamp = p4			;vary damping amount
asig stix .5, 0.01, 30, idamp
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i1 0 1 .3
i1 + 1  >
i1 + 1  >
i1 + 1 .95

e
</CsScore>
</CsoundSynthesizer>
