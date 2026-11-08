<CsTest>
description = "fprintks reports oversized formatted output instead of truncating it"

[expect]
exit = "nonzero"
stderr = ["formatted output exceeds 8192 characters"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  fprintks "fprintks_limit.txt", "%8193d", 1
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
