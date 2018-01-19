<CsoundSynthesizer>
<CsOptions>
-d -odac -B1024
</CsOptions>
<CsInstruments>
sr=44100
ksmps=64
nchnls=1

gklevel init 0
gks chnexport "meter",3             

instr 1
k1 linenr 2000, 0.1,0.1, 0.1
kp chnget "pitch"
kp tonek kp, 10
kv chnget "volume"
kv tonek kv, 10
kf linsegr 500, 0.1, 4000, 1.5, 1000, 0.1, 100
a1 vco2 p4*kv, p5*kp
a2 moogladder a1, kf*kv, 0.8
ks rms a1
gklevel = gklevel + ks
   out a2
endin

instr 100
gks = gklevel                                       
gklevel = 0
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
