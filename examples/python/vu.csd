<CsoundSynthesizer>
<CsOptions>
-d -odac -iadc -B2048 -b1024
</CsOptions>
<CsInstruments>
sr=44100
ksmps=128
nchnls=1

gklevel chnexport "meter",3
gkp chnexport "pitch", 3

instr 1
k10 chnget "clicked"

if k10 == 0 then
a1 in
ks rms a1
kp,ka pitchamdf a1,55,660 
gklevel = ks
if gklevel > 500 then
gkp = (kp-55)/660
else
gkp = 0.5
gklevel = 0
endif
kl tonek gklevel, 10
kp tonek kp, 10
else
kl tonek gklevel, 10
kp tonek gkp, 10
endif

asig oscili kl, kp, 1
 out asig

endin

</CsInstruments>
<CsScore>

f1 0 16384 10 1 .5 .3 .2 .1 .05
i1 0 360000
e
</CsScore>
</CsoundSynthesizer>

