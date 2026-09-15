<CsTest>
description = "mfb rejects invalid frequency order"

[expect]
exit = "nonzero"
stderr = ["mfb: frequencies must be finite, non-negative and ordered"]
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
kInput[] init 129
kOutput[] mfb kInput, 8000, 300, 40
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
