<CsTest>
description = "seed rejects a value beyond the unsigned 32-bit range"

[expect]
exit = "nonzero"
stderr = ["seed: value out of range"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  seed 4294967296
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
