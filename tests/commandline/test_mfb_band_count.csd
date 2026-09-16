<CsTest>
description = "mfb rejects invalid band count"

[expect]
exit = "nonzero"
stderr = ["mfb: band count must be positive and less than input length"]
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
iInput[] init 129
iOutput[] mfb iInput, 0, 24000, 0
endin
</CsInstruments>
<CsScore>
i 1 0 .001
</CsScore>
</CsoundSynthesizer>
