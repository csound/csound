<CsoundSynthesizer>
<CsOptions>
-o dac -dm0 -i adc
</CsOptions>
<CsInstruments>
nchnls=2
nchnls_i=1
0dbfs=1
ksmps=64
sr = 44100

instr 1

kldelay     chnget "leftDelayTime"
klfeedback  chnget "leftFeedback"
krdelay     chnget "rightDelayTime"
krfeedback  chnget "rightFeedback"

asig inch 1
a1 init 0
a2 init 0
a1 vdelay3 asig+a2*klfeedback, kldelay, 3000
a2 vdelay3 asig+a1*krfeedback, krdelay, 3000
outs a1,a2

endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1

i1 0 360000

</CsScore>
</CsoundSynthesizer>
