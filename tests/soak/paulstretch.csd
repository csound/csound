<CsoundSynthesizer>
<CsOptions>
-o paulstretch.wav -W
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 100
0dbfs = 1
nchnls = 2

giwav ftgen 0, 0, 0, 1, "fox.wav", 0, 0, 1

instr 1
aout paulstretch 10, 0.2, giwav
outs aout, aout
endin

</CsInstruments>
<CsScore>
i1 0 30

</CsScore>
</CsoundSynthesizer>

