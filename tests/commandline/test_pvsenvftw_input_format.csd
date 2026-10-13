<CsTest>
description = "pvsenvftw rejects sliding and complex-format spectra"

[expect]
exit = "nonzero"
stderr = ["pvsenvftw: cannot use sliding PVS", "pvsenvftw: format must be amp-freq or amp-phase"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 6400
ksmps = 8
nchnls = 1
0dbfs = 1
giEnvelope ftgen 1, 0, 64, 2, 0

instr 1
 ; Sliding spectra have a different layout and a spectrum for every sample.
 fInput pvsinit 128, 1
 kUpdated pvsenvftw fInput, giEnvelope
endin

instr 2
 ; Real and imaginary components are not amplitude/phase pairs.
 fInput pvsinit 128, 32, 128, 1, 2
 kUpdated pvsenvftw fInput, giEnvelope
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
e
</CsScore>
</CsoundSynthesizer>
