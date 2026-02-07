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

opcode testVcoOver, a, i
  oversample 2
  iFreq xin
  aSig vco2 1, iFreq
  xout aSig
endop

opcode testVcoOver2(iFreq:i):a
  oversample 2
  aSig vco2 1, iFreq
  xout aSig
endop

opcode testVcoUnder, a, i
  undersample 2
  iFreq xin
  aSig vco2 1, iFreq
  xout aSig
endop

opcode testVcoUnder2(iFreq:i):a
  undersample 2
  aSig vco2 1, iFreq
  xout aSig
endop


instr 1
krms1 rms testVco2(400)-testVco(400)
krms2 rms testVcoOver2(400)-testVcoOver(400)
krms3 rms testVcoUnder2(400)-testVcoUnder(400)
if (krms1 + krms2 + krms3) > 0 then
  printk2 krms1
  printk2 krms2
  printk2 krms3
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

