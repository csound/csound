<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


instr 1
 cvar@global:i chnexport "test",3
 chnset 1, "test"
endin

instr 2
 var:i chnget "test"
 if(var != cvar) then
  prints "exported channel not matching var"
  exitnow(-1)
 else
  print var
 endif
 cvar = 2
endin


</CsInstruments>
<CsScore>
i1 0 1
i2 1 1
i2 2 1
e
</CsScore>
</CsoundSynthesizer>
