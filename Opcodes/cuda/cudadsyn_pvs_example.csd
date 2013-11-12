<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libcudaop1.dylib 
</CsOptions>
<CsInstruments>

ksmps = 64
0dbfs = 1


instr 1

asig diskin2 "flutec3.wav", 1
fsig pvsanal asig, 1024, 128, 1024, 1
a1 cudasynth fsig,p4,p5/cpspch(8.00),-1, 128
a2 linenr a1,0.001,0.01,0.01
    out a2

endin


</CsInstruments>
<CsScore>
i1 0 5 0.5 440
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
