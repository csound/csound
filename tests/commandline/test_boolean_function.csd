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


</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>

