<CsTest>
description = "partials rejects negative maximum track counts"

[expect]
exit = "nonzero"
stderr = ["partials: imaxtracks must be at least 1"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

giSine ftgen 0, 0, 4096, 10, 1

instr 1
  ain oscili 0.5, 440, giSine
  ffr, fphs pvsifd ain, 1024, 256, 1
  ftrk partials ffr, fphs, 0.001, 1, 1, -1
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
