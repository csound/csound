<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode Test,0,ii
 var1:i, var2:i xin
 if var1 != var2 then
   prints "error\n"
   exitnow(-1)
 else
   prints "pass\n"
 endif
endop

instr 1
 var1:i = 1
 var2:i = 2
 var3:i = 3
 Test  var1+var2*var3, var1+var2*var3
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>