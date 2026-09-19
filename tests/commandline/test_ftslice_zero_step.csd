<CsTest>
description = "ftslice rejects a zero step before computing the slice length"
[expect]
exit = "nonzero"
stderr = ["ftslice: invalid slice bounds or step"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1

instr 1
  iSource ftgen 0, 0, -4, -2, 1, 2, 3, 4
  iDestination ftgen 0, 0, -4, -2, 0
  ftslicei iSource, iDestination, 0, 4, 0
endin
</CsInstruments>
<CsScore>
i 1 0 .015625
e
</CsScore>
</CsoundSynthesizer>
