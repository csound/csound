<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Verifies that a non-UDT generic type can round-trip through the
; generic software bus. We write a Complex value, read it back through
; chnget, then assert both components at k-time.

instr 1
 cvar:Complex init 1,1
 cvar2:Complex init 0,0
 chnset cvar, "Test1"
 cvar2 chnget "Test1"

 if timeinstk() > 0 then
   if abs(real(cvar2) - 1) > 0.000001 then
     printks "generic complex real mismatch: %f\n", 0, real(cvar2)
     exitnowk(-1)
   endif
   if abs(imag(cvar2) - 1) > 0.000001 then
     printks "generic complex imag mismatch: %f\n", 0, imag(cvar2)
     exitnowk(-1)
   endif
   turnoff
 endif
endin

</CsInstruments>

<CsScore>
i1 0 0.05
e
</CsScore>
</CsoundSynthesizer>
