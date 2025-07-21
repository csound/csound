<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode Assert,i,ii
 i1,i2 xin
 if i1 != i2  then
  exitnow(-1)
 endif
 xout i1
endop

instr 1
i1 = 0
i1 += 1
print Assert(i1,1)
i1 *= 2
print Assert(i1,2)
i1 /= 2
print Assert(i1,1)
i1 -= 1
print Assert(i1,0)
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>

