<CsTest>
description = "variables cannot supply struct field defaults"

[expect]
exit = "nonzero"
stderr = ["[struct] default for", "must be a numeric literal"]
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

giDefault init 100
struct Bad velocity:i = giDefault

instr 1
  value:Bad init
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
