<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>


gatest init 0
gaatest[] init 2
gktest init 0

opcode Test1,0,0
  setksmps 1
  gktest = 1  // ok
endop

opcode Test2,0,0
  setksmps 1
  gatest = 1 // fail
endop

opcode Test3,0,0
  setksmps 1
  gaatest[0] = 1 // fail
endop


instr 1
Test1
Test2
Test3
endin


</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
