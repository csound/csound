<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>


opcode Test(i:i,j:i):b
 xout (i == j)
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

