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
kgain chnget "gain"
asig1,asig2 ins
chnset asig1, "meter"
asig1 = asig1 * kgain
asig2 = asig2 * kgain
outs asig1, asig2
endin

</CsInstruments>
<CsScore>

i1 0 360000
 
</CsScore>
</CsoundSynthesizer>
