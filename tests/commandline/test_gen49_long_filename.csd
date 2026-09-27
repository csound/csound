<CsTest>
description = "GEN49 rejects filenames that do not fit its buffer"

[expect]
exit = "nonzero"
stderr = ["GEN49: filename too long"]
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1

instr 1
  ; Include both quotes so GEN49 exercises its quote-stripping path.
  SName sprintf "\"%01024d\"", 0
  iTable ftgen 0, 0, 16, 49, SName, 0, 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
