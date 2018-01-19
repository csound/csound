<CsoundSynthesizer>
<CsOptions>
-odac -d -B4096 -b1024
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

f1 0 16384 10 1
; run for 30 secs
i1 0 30 

</CsScore>

</CsoundSynthesizer>
