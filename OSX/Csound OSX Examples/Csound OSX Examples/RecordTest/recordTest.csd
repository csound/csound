<CsoundSynthesizer>
<CsOptions>
-o dac -+rtmidi=null -+rtaudio=null -d -+msg_color=0 -M0 -m0 -i adc
</CsOptions>
<CsInstruments>
nchnls=2
0dbfs=1
ksmps=64
sr = 44100

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
