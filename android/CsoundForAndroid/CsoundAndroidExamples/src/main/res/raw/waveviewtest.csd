<CsoundSynthesizer>
<CsOptions>
-o dac -+rtmidi=null -d -+msg_color=0 -M0 -m0 -i adc
</CsOptions>
<CsInstruments>
nchnls=2
0dbfs=1
ksmps=64
sr = 44100

instr 1

aout oscil3 .5, 440, 1

outs aout, aout

endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1

i1 0 360000
 
</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>1036</x>
 <y>61</y>
 <width>396</width>
 <height>744</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="nobackground">
  <r>231</r>
  <g>46</g>
  <b>255</b>
 </bgcolor>
 <bsbObject version="2" type="BSBVSlider">
  <objectName>slider1</objectName>
  <x>5</x>
  <y>5</y>
  <width>20</width>
  <height>100</height>
  <uuid>{ea2b771b-3cad-462f-b9df-a821dbfeaaf8}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>-3</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.00000000</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
<MacGUI>
ioView nobackground {59367, 11822, 65535}
ioSlider {5, 5} {20, 100} 0.000000 1.000000 0.000000 slider1
</MacGUI>
