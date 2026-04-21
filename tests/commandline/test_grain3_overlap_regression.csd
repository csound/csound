<CsoundSynthesizer>
<CsInstruments>
sr = 48000
ksmps = 48
nchnls = 1

giw ftgen 1, 0, 16384, 20, 3, 1
gis ftgen 2, 0, 16384, 10, 1

instr 1
a1 grain3 440, 0, 0, 0, 0.2, 200, 100, 2, 1, 0, 0, 1, 2
out a1
endin
</CsInstruments>
<CsScore>
i 1 0 0.25
e
</CsScore>
</CsoundSynthesizer>
