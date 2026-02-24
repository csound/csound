<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

0dbfs = 1


opcode Test(k1:k,a1:a):a
oversample 2,4
a2 = a1*k1
  xout a2
endop

instr 1
a1 oscili p4, p5
a2 Test 0.5, a1
   out a2
endin

opcode SVF,a,akk
oversample 2
a1,kcf,kq xin
alow, ahigh, aband svfilter a1,kcf,kq
  xout alow
endop

instr 2
a1 vco2 p4, p5
kcf expseg 16000, p3, p5
kq = 5
a2 SVF a1,kcf,kq
   out a2
endin


</CsInstruments>
<CsScore>
i1 0 2 0.25 440
i1 + 2 0.25 440
i2 4 6 0.1 440
</CsScore>
</CsoundSynthesizer>
















<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>100</x>
 <y>100</y>
 <width>320</width>
 <height>240</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>240</r>
  <g>240</g>
  <b>240</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
