<CsoundSynthesizer>
<CsOptions>
-d -odac -B1024
</CsOptions>
<CsInstruments>
sr=44100
ksmps=100
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
a1 oscili p4*kv, p5*kp,1
a2 moogladder a1, kf*kv, 0.8
ks rms a1
gklevel = gklevel + ks
   out a2
endin

instr 100
gks = gklevel
gklevel = 0
endin

</CsInstruments>
<CsScore>

f1 0 16384 10 1 .5 .3 .2 .1 .05
i100 0 360
e
</CsScore>
</CsoundSynthesizer>

