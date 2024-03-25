<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

instr 1
p3 = filelen("fox.wav")/p4
k1 linen 1,0.01,p3,0.1
a1 filescal p4,0.5,1,"fox.wav",1
  out a1*k1
endin

</CsInstruments>
<CsScore>
i1 0 1 1.25
i1 2.5 1 .75
</CsScore>
</CsoundSynthesizer>

