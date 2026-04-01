<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


instr 1
 cvar@global:k chnexport "test",3
 chnset 1, "test"
 sig:a oscili 0dbfs, A4
 chnmix sig, "audio"
endin

instr 2
 var:k chnget "test"
 if(var != cvar) then
  printks "exported channel not matching var", 1
  exitnowk(-1)
 else
  printk2 var
 endif
 cvar = 2
 sig:a chnget "audio"
 out sig
 chnclear "audio"
endin


</CsInstruments>
<CsScore>
i1 0 2
i2 0 2
e
</CsScore>
</CsoundSynthesizer>
