<CsoundSynthesizer>
<CsInstruments>
0dbfs = 1
instr 1
a1 = vco2(p4/3, p5) +
     vco2(p4/3, p5*1.01) +
     vco2(p4/3, p5*0.99) 
out a1
endin

</CsInstruments>
<CsScore>
i1 0 1 0.5 440
</CsScore>
</CsoundSynthesizer>
