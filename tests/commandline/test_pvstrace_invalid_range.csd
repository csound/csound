<CsTest>
description = "pvstrace rejects negative bin limits at initialization"

[expect]
exit = "nonzero"
stderr = ["pvstrace: bin limits must be nonnegative"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1
instr 1
 kInput[] init 18
 fInput pvsfromarray kInput, 4
 fOutput, kBins[] pvstrace fInput, 2, 0, p4, p5
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1 0
i 1 0 .01 0 -1
e
</CsScore>
</CsoundSynthesizer>
