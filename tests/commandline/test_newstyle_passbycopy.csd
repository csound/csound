<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>
0dbfs=1

opcode testVco, a, i
  setksmps 1
  iFreq xin
  aSig vco2 1, iFreq
  xout aSig
endop

opcode testVco2(iFreq:i):a
  setksmps 1
  aSig vco2 1, iFreq
  xout aSig
endop


instr 1
krms rms testVco2(400)-testVco(400);
if krms > 0 then
  printk2 krms
  event "i", 2, 0, 0
endif
turnoff
endin

instr 2
exitnow(-1)
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


