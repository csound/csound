<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>


sr=44100
ksmps=32
nchnls=2
0dbfs=1

/* this opcode should never be used */
opcode testNoise, a, 0
 apink  oscili 0.2, 440
 xout apink
endop

/* this opcode should replace the above */
opcode testNoise, a, 0
 awhite unirand 2.0
 awhite = awhite - 1.0
 apink  pinkish awhite, 1
 xout apink
endop


instr 1
 apink testNoise
 outs apink * 0.25, apink * 0.25
endin

instr 2

i1 compilestr {{
/* this opcode should replace the above
   after compilation */
opcode testNoise, a, 0
 apink  oscili 1.0, 440
 xout apink
endop

instr 1
 apink testNoise
 outs apink * 0.25, apink * 0.25
endin

}}

endin

</CsInstruments>
<CsScore>
i1 0 3
i2 1 0.1
i1 4 1
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
