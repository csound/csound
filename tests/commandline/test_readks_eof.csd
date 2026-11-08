<CsTest>
description = "readks reports a performance error at EOF"

[expect]
exit = "nonzero"
stderr = ["Read failure in readks"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1

instr 1
  fprints "readks_eof.dat", "%s", "line\n"
endin

instr 2
  SLine readks "readks_eof.dat", 0
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 .01 .01
</CsScore>
</CsoundSynthesizer>
