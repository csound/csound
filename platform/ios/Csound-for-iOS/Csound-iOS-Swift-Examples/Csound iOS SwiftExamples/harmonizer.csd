<CsoundSynthesizer>
<CsOptions>
-o dac
-i adc
</CsOptions>
<CsInstruments>

sr        = 44100
ksmps     = 64
nchnls    = 2
0dbfs	  = 1

instr 3

asig,asig1 ins

ksl chnget "slider"
kgain chnget "gain"
fsig1 pvsanal asig,2048,256,2048,1
fsig2 pvscale fsig1,0.75+ksl,1
fsig3 pvscale fsig1,(0.75+ksl)*1.25,1
fsig4 pvsmix fsig2, fsig3
a1 pvsynth fsig4 

a1 = a1 * kgain
   outs a1, a1
endin

</CsInstruments>
<CsScore>
f1 0 16384 10 1

i3 0.1 10000

</CsScore>
</CsoundSynthesizer>
