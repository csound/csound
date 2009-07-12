<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
nchnls =        2

instr 1  
  a1,a2 mp3in "new.mp3"
        outs a1,a2
endin

</CsInstruments>

<CsScore>
;;;             file  skip chan 
f1 0 16384 49 "new.mp3" 0 0

; run for 30 secs
i1 0 10 

</CsScore>

</CsoundSynthesizer>
