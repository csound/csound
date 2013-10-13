<CsoundSynthesizer>

<CsInstruments>
instr 1
kadsr adsr .1, .2, .5, .5
a1 oscili 1, 440
outs  (kadsr)*a1,a1
endin
</CsInstruments>

<CsScore>
i1 0 .5

</CsScore>

</CsoundSynthesizer>
