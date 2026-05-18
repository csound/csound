<CsoundSynthesizer>
<CsOptions>
-o rev.wav -W -f
</CsOptions>
<CsInstruments>
0dbfs=1

instr 1
  asig diskin "sweep.wav"
  arev reverb asig, p3/2
  out arev
endin

</CsInstruments>
<CsScore>
i1 0 2
</CsScore>
</CsoundSynthesizer>
