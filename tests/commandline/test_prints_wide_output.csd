<CsTest>
description = "prints reports conversions whose output does not fit what it writes into, and does not overrun it"

[expect]
exit = "nonzero"
stderr = ["prints", "a format too long"]
output_excludes = ["AddressSanitizer", "runtime error:"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  prints "%8000.0f %8000.0f\n", 1, 2
endin

</CsInstruments>
<CsScore>
i 1 0 .1
e
</CsScore>
</CsoundSynthesizer>
