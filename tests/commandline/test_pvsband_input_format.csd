<CsTest>
description = "pvsband filters reject phase spectra instead of treating phases as frequencies"

[expect]
exit = "nonzero"
stderr = ["pvsband: input must be amp-freq"]
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
 fPhase pvsinit 128, 32, 128, 1, 1
 fPass pvsbandp fPhase, 100, 200, 300, 400
endin
instr 2
 fPhase pvsinit 128, 32, 128, 1, 1
 fReject pvsbandr fPhase, 100, 200, 300, 400
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
e
</CsScore>
</CsoundSynthesizer>
