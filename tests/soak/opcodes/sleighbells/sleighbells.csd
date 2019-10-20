<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1


instr 1

idamp = p4
asig  sleighbells .7, 0.01, 32, idamp
      outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0.00 0.25 0	;short sound
i 1 0.30 0.25
i 1 0.60 0.25
i 1 0.90 0.25
i 1 1.20 0.25
i 1 1.50 1   .3	;longer sound
i 1 1.80 0.25 0	;short sound again
i 1 2.10 0.25
i 1 2.40 0.25
i 1 2.70 0.25
i 1 3.00 0.25
e

</CsScore>
</CsoundSynthesizer>
