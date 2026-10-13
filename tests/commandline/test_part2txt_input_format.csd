<CsTest>
description = "part2txt rejects non-track spectral frames"

[expect]
exit = "nonzero"
stderr = ["part2txt: input must be in TRACKS format"]
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

instr 1
  ain rand 0.1
  fsig pvsanal ain, 1024, 256, 1024, 1
  part2txt "part2txt_invalid.txt", fsig
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
