<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode one_dim, 0, k[]
kArr[] xin
endop

opcode two_dim, 0, k[][]
kArr[][] xin
endop

instr 1
kArr1[] init 3
kArr2[][] init 3, 4
one_dim kArr1
two_dim kArr2
turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 1
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
 <bgcolor mode="nobackground">
  <r>255</r>
  <g>255</g>
  <b>255</b>
 </bgcolor>
</bsbPanel>
<bsbPresets>
</bsbPresets>
