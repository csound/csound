<CsoundSynthesizer>
<CsOptions>
-d -odac -iadc  -B2048
</CsOptions>
<CsInstruments>
sr=44100
ksmps=128
nchnls=1
0dbfs=2.0

gklevel init 0
gks chnexport "meter",3


instr 1
ain diskin2 "ah.wav", 1,0,1
k1 linenr 1, 0.1,0.1, 0.1
kf linsegr 500, 0.1, 4000, 1.5, 1000, 0.1, 100
fam pvsanal ain*k1, 1024,256,1024,1
fex pvsosc p4, p5/2, 1, 1024,256,1024,1
fvo pvsvoc  fam,fex,1,1
a1 pvsynth fvo
ks rms a1
gklevel = gklevel + ks
   out a1*k1


endin

instr 100
gks = gklevel
gklevel = 0
endin

</CsInstruments>
<CsScore>

f1 0 16384 10 1
i100 0 36000
e
</CsScore>
</CsoundSynthesizer>

