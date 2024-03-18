<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

instr 1

asig oscili p4, p5
a1,a2 hilbert2 asig,1024,256
  outs a1, a2

endin


</CsInstruments>
<CsScore>
i1 0 10 0.5 440
</CsScore>
</CsoundSynthesizer>