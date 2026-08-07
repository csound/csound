<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>


gatest init 0
gaatest[] init 2
gktest init 0

opcode Test0,0,0
  gatest = 1
  prints "OK!"
endop

opcode Test1,0,0
  setksmps 1
  gktest = 1  // ok
  prints "OK!"
endop

opcode Test2,0,0
  setksmps 1
  gatest = 1 // fail
endop

opcode Test3,0,0
  setksmps 1
  gaatest[0] = 1 // fail
endop

opcode Test4,0,0
 setksmps 1
 Test0
endop

opcode Test00,0,0
 setksmps 10 
 gaatest[0] = 1 //ok
 prints "OK!"
endop




instr 1
Test0
Test00
Test1
Test2
Test3
Test4
endin

instr 2
gatest = 1
endin

instr 3
setksmps 1
subinstr 2
endin



</CsInstruments>
<CsScore>
i 1 0 1
i 3 0 1
</CsScore>
</CsoundSynthesizer>
