<CsoundSynthesizer>
<CsOptions>
--opcode-lib=./opcodes.dylib
</CsOptions>
<CsInstruments>

instr 1
i1 = 1
i2 simple i1
print i2
endin

instr 2
k1 = 1
k2 init 0
k2 simple k1
printk2 k2
endin

instr 3
a1 diskin "fox.wav"
a2 simple a1
    out a2
endin

instr 4
kIn[] fillarray 2,3,4,5
kOut[] simple kIn

kn init 0
while kn < 4 do
printk2 kOut[kn]
kn+=1
od
endin

instr 5
tprint "Hello World !!\n"
endin

instr 6
a1 diskin "fox.wav"
a2 delayline a1,0.5
   out a1 + a2
endin

instr 7
a1 oscillator 0dbfs/4,A4,-1
   out a1
endin

instr 8
a1 diskin "fox.wav"
fs1 pvsanal a1,1024,256,1024,1
fs2 pvg fs1,0.5
a2 pvsynth fs2
    out a2
endin

</CsInstruments>
<CsScore>
i1 0   0
i2 0 0.001
i3 0 4
i4 0 0.001
i5 0 0
i6 4 5
i7 9 1
i8 10 4
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
