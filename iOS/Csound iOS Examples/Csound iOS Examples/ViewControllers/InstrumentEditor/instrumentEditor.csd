<CsoundSynthesizer>
<CsOptions>
-o dac -+rtaudio=null -dm0
</CsOptions>
<CsInstruments>
nchnls=2
0dbfs=1
ksmps=64
sr = 44100

instr 1
a1 oscili 0.5, 440
outs a1,a1
endin

</CsInstruments>
<CsScore>

</CsScore>
</CsoundSynthesizer>
