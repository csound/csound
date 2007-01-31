<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>

<CsInstruments>

; bus channel
gkfreq chnexport "freq", 3

instr 1  
  a1 oscil 10000, gkfreq, 1
  out a1
endin

</CsInstruments>

<CsScore>

f1 0 16384 10 1
; run for 30 secs
i1 0 30 

</CsScore>

</CsoundSynthesizer>
