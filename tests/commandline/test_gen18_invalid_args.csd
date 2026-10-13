<CsTest>
description = "GEN18 rejects negative destination offsets and incomplete waveform arguments"

[expect]
exit = "nonzero"
stderr = ["GEN18: invalid destination range", "wrong number of args"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
giSource ftgen 1, 0, 4, -2, 1, 2, 3, 4

instr 1
  iTable ftgen 2, 0, 8, -18, giSource, 1, -1, 4
endin

instr 2
  iTable ftgen 3, 0, 8, -18, giSource, 1, 0, 7, giSource
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0 0.01
e
</CsScore>
</CsoundSynthesizer>
