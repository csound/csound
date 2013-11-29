<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./libcudaop2.dylib
</CsOptions>
<CsInstruments>

ksmps = 64
0dbfs = 1

instr 1
ifftsize = 2048
asig diskin2 "flutec3.wav", 1, 0, 1
fsig cudanal asig, ifftsize, ifftsize/8, ifftsize, 1
a1 pvsynth fsig
a2 linenr a1*0.5,0.005,0.01,0.01    
   out a2
endin

instr 2
ifftsize = 2048
asig diskin2 "flutec3.wav", 1, 0, 1
fsig pvsanal asig, ifftsize, ifftsize/8, ifftsize, 1
a1 cudasynth fsig
a2 linenr a1*0.5,0.005,0.01,0.01    
   out a2
endin


</CsInstruments>
<CsScore>
i1 0 5
i2 5 5
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
