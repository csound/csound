<CsTest>
description = "pvsarp rejects unsupported sliding spectra during initialization"

[expect]
exit = 1
stderr = ["pvsarp: sliding analysis is not supported"]
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
 aInput oscili .1, 500
 ; A one-sample hop selects sliding analysis, whose frames use a different
 ; layout from the non-sliding frames that pvsarp supports.
 fInput pvsanal aInput, 128, 1, 128, 0
 fOutput pvsarp fInput, .5, 1, 2
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
