<CsTest>
description = "GEN43 requires a whole channel number"

[expect]
exit = "nonzero"
stderr = ["illegal channel number"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
instr 1
  iTable ftgen 0, 0, 32, -43, "test_pvsdiskin_stereo.pvx", 1.5
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
