<CsoundSynthesizer>
<CsOptions>
-d -odac -B1024
</CsOptions>
<CsInstruments>
ksmps=64
gklevel init 0
gks chnexport "meter",3             
gar init 0


instr 1
 kp chnget "pitch"
 kp tonek kp, 10
 kv chnget "volume"
 kv tonek kv, 10
 kf linsegr 500, 0.1, 4000, 1.5, 1000, 0.1, 100
 a1 vco2 p4*kv, p5*kp
 a11 vco2 p4*kv, 1.5*p5*kp
 a2 moogladder a1+a11, kf*kv, 0.8
 ks rms a2
 gklevel += ks
   out a2
   gar += a2
endin



instr 100
 a1,a2 reverbsc gar,gar,0.7,2000
 out (a1+a2)/2
 gks = gklevel                                       
 gklevel = 0
 gar = 0
endin
schedule(100,0,-1)

</CsInstruments>
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
