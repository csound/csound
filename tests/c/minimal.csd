<CsoundSynthesizer>
<CsOptions>
-d
</CsOptions>
<CsInstruments>
nchnls=1

instr 1
ga1 oscil3 p4*0.9, p5*1.1, -1
   ;out ga1  
endin

instr 20
 out ga1
endin

</CsInstruments>
<CsScore>

i1 0 1 1000 500
i1 + 2 1000 750
i1 + 3 1000 1000
i20 0 6

</CsScore>
</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>614</x>
 <y>61</y>
 <width>695</width>
 <height>747</height>
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
  <uuid>{fe924818-39b1-44b5-b240-331a49ca0049}</uuid>
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
