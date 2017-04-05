<CsoundSynthesizer>

<CsInstruments>
0dbfs=1
gio OSCinit 44100
instr 1
 kans init 0
 kstart init 0
 kdur init 0
 kamp init 0
 kfreq init 0

 listen:
 kans  OSClisten gio, "/instr2", "ffff",kstart,kdur,kamp,kfreq
 if kans == 1 then
  event "i",2,kstart,kdur, kamp,kfreq
  kgoto listen
 endif

endin

instr 2
 a1 vco2 p4, p5
    out a1
endin

	instr 11
OSCsend 1, "localhost", 44100, "/instr2", "ffff", 0, 1, 0.1, 440
    	endin

</CsInstruments>

<CsScore>
i 1 0 1000.0
i 11 2  1
e
</CsScore>

</CsoundSynthesizer>
<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>702</x>
 <y>61</y>
 <width>400</width>
 <height>292</height>
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
  <uuid>{2ec0cf9e-d2df-42de-952a-3af28f389d78}</uuid>
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
<MacOptions>
Version: 3
Render: Real
Ask: Yes
Functions: ioObject
Listing: Window
WindowBounds: 702 61 400 292
CurrentView: io
IOViewEdit: On
Options:
</MacOptions>

<MacGUI>
ioView nobackground {59367, 11822, 65535}
ioSlider {5, 5} {20, 100} 0.000000 1.000000 0.000000 slider1
</MacGUI>
