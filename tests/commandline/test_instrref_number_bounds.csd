<CsTest>
description = "numeric instrument references reject out-of-range indices"
[expect]
exit = "nonzero"
stderr = ["init: instrument number out of range", "init: instrument is not defined"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  ref:InstrDef init p4
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1
i 1 0 .01 1000000
i 1 0 .01 1e30
i 1 0 .01 199
e
</CsScore>
</CsoundSynthesizer>
