<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

instr 1  
  k2 oscil 10000, 1, 1
  a1 oscil 10000+k2, 120, 1
  chano a1, 1
  out a1
endin

</CsInstruments>

<CsScore>

f1 0 16384 49 "mpadec/new.mp3" 0 0 0
; run for 30 secs
i1 0 30 

</CsScore>

</CsoundSynthesizer>
