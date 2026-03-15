<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

struct Test var1:i, var2:i

instr 1
val new Test
val.var1 = 1
val2 new Test(1,2)
print val.var1
print val2.var1
if val.var1 != val2.var1 then
 exitnow(-1);
endif 
endin

</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>
</CsoundSynthesizer>
