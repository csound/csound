<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
nchnls =        1

instr 1  
  i1 wiiconnect
  print i1
k1  wiidata 1
printk 0, k1
endin

</CsInstruments>

<CsScore>

f1 0 16384 49 "mpadec/new.mp3" 0 0 0
; run for 30 secs
i1 0 30 

</CsScore>

</CsoundSynthesizer>
