<CsTest>
description = "pvswarp rejects unsupported sliding spectra during initialization"

[expect]
exit = "nonzero"
stderr = ["pvswarp: sliding analysis is not supported"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
 ; A one-sample hop selects sliding analysis, which pvswarp cannot process.
 fSliding pvsinit 128, 1, 128, 1
 fOutput pvswarp fSliding, 1, 0
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
