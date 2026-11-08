<CsoundSynthesizer>
<CsOptions>
-o dac
-d
-i adc
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 64
nchnls    = 2
0dbfs	  = 1

instr 1

kldelay     chnget "leftDelayTime"
klfeedback  chnget "leftFeedback"
krdelay     chnget "rightDelayTime"
krfeedback  chnget "rightFeedback"

asig,asig1 ins
a1 init 0
a2 init 0
a1 vdelay3 asig+a2*klfeedback, kldelay, 3000
a2 vdelay3 asig+a1*krfeedback, krdelay, 3000
outs a2,a1

endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1

i1 0 360000
 
</CsScore>
</CsoundSynthesizer>
