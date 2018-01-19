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
gar init 0

instr 1
k1 linenr 2000, 0.1,0.1, 0.1
S1 sprintf "pitch-%d", frac(p1)*10
kp chnget S1
kp tonek kp, 10
S2 sprintf "volume-%d", frac(p1)*10
kv chnget S2
kv tonek kv, 10
kf linsegr 500, 0.1, 4000, 1.5, 1000, 0.1, 100
a1 vco2 p4*kv, p5*kp
a2 moogladder a1, kf*kv, 0.8
ks rms a1
gklevel = gklevel + ks
   out a2
      gar += a2
endin

instr 100
a1,a2 reverbsc gar,gar,0.7,2000
out (a1+a2)/2
gar = 0
gks = gklevel                                       
gklevel = 0
endin
schedule(100,0,-1)

</CsInstruments>
</CsoundSynthesizer>


