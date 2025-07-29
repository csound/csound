<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 44100
0dbfs=1
nchnls = 2

opcode myop(var1:i,var2:o,var3:p,var4:q,var5:v,var6:j,var7:h):void
print var1,var2,var3,var4,var5,var6,var7
endop

instr 1
myop(1)
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>