<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libcudaop1.dylib
</CsOptions>
<CsInstruments>
ksmps = 64
0dbfs = 1

i1 ftgen 1,0,512,7,1,512,0.001
i2 ftgen 2,0,512,-7,1,512,512
i3 ftgen 3,0,8192,10,1
schedule 1,0,10

instr 1
a1 cudasynth 0.001, 100,3, 2, 1, 128
    out a1

endin

</CsInstruments>
<CsScore>
e 10
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
