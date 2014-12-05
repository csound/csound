;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This is a simple csound file to test your audio output   ;
; from WinXound. In order to work correctly with WinXound  ;
; it's very important that you fill all the required       ;
; compiler paths fields into the WinXound Settings         ;
; window with correct values (check Menu File > Settings). ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

<CsoundSynthesizer>
<CsOptions>
-o test.wav -d
</CsOptions>
<CsInstruments>
sr     = 44100
kr     = 4410
ksmps  = 10
nchnls = 2
0dbfs=1
i1 ftgen 1,0,2048,10,1
instr 1 ;Simple sine at 440Hz
a1	oscili 0.5,440, 1
outs a1, a1
endin

</CsInstruments>
<CsScore>

i1 0 30
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
