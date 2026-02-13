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

var:i init 1
test:b init true
test2:b = test

if b(var) then
 print i(test2)
endif

endin

opcode Test2(A:i,B:i,C:i):b
  xout A+B==C
endop

instr 2
  if Test2(1+1,2,4)  goto end 
  prints "hello\n"
  end:
endin


</CsInstruments>
<CsScore>
i1 0 0
i2 0 0
</CsScore>
</CsoundSynthesizer>

