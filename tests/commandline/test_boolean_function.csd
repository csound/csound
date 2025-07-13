<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


opcode Test(i1:i,i2:i):b
 i1,i2 xin
 xout (i1 == i2)
endop

instr 1
if Test(1,1) then
 prints "passed\n"
endif
endin


</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>

