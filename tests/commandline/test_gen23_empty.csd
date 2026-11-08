<CsTest>
description = "GEN23 rejects automatic sizing of an empty file"

[expect]
exit = "nonzero"
stderr = ["GEN23: no numeric values"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
instr 1
  fprints "gen23_empty.txt", "%s", ""
  ficlose "gen23_empty.txt"
  iResult ftgen 0, 0, 0, -23, "gen23_empty.txt"
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
