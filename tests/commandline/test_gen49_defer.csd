<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>
0dbfs=1



instr 1
S1 = "beats.mp3"
ifn ftgen 0,0,0,49,S1,0,1
if ftlen(ifn) != mp3len(S1)*mp3sr(S1) then
  prints "GEN49 deferred length fail\n"
  exitnow(-1)
endif
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


