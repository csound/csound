<CsTest>
description = "A conversion too long for sprints to hold is an error, not an overrun"

[expect]
exit = "nonzero"
stderr = ["printks2", "a format too long"]
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
  kCycle timeinstk
  printks2 "%00000000000000000000000000000000000000000000000000000000000000000000001.5f\n", kCycle
endin

</CsInstruments>
<CsScore>
i 1 0 .1
e
</CsScore>
</CsoundSynthesizer>
