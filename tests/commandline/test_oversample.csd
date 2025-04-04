<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

0dbfs = 1

opcode Test(k1:k,a1:a):a
oversample 2,3
a2 = a1*k1
  xout a2
endop

instr 1
a1 oscili p4, p5
a2 Test 0.5, a1
   out a2
endin


</CsInstruments>
<CsScore>
i1 0 2 0.25 440
i1 + 2 0.25 440
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
