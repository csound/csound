<CsoundSynthesizer>
<CsInstruments>
instr 1
 asig vco2 p4, p5
 fs1 pvsanal asig,1000,250,1000,1
 ahr pvsynth fs1
 out ahr
endin
</CsInstruments>
<CsScore>
i1 0 1 20000 440
</CsScore>
</CsoundSynthesizer>
