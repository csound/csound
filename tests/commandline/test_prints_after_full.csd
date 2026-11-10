<CsTest>
description = "A conversion that starts when the output is already full is an error, not a silently dropped value"

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
  ; The first conversion's own output fills the buffer and is cut to fit; the
  ; second has nowhere to go.
  prints "%9000.0f%d", 1, 3
endin
</CsInstruments>
<CsScore>
i 1 0 .1
e
</CsScore>
</CsoundSynthesizer>
