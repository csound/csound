<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


instr 1
 cvar:Complex init 1,1
 cvar2:Complex init 0,0
 chnset cvar, "Test1"
 cvar2 chnget "Test1"
 if real(cvar2) != 1 then
   exitnowk(-1)
 else
   printk2 real(cvar2)
 endif
endin

</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>
</CsoundSynthesizer>
