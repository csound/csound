<CsTest>
description = "padsynth requires its parameters and at least one partial"

[expect]
exit = "nonzero"
stderr = ["insufficient arguments"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  iTab ftgen 0, 0, 1024, "padsynth", 440, 25, 0, 1, 1, 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
