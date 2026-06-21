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

instr 3
 var@global:Complex chnexport "complex", 3
 var = 2,1
endin

instr 4
 var:Complex chnget "complex"
 if real(var) != imag(var) + 1 then
   exitnowk(-1)
 endif
 printk2 abs(var)
endin

</CsInstruments>
<CsScore>
i1 0 2
i2 0 2
i3 0 1
i4 0 1
e
</CsScore>
</CsoundSynthesizer>
