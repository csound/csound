<CsTest>
description = "nested struct fields cannot have literal defaults"

[expect]
exit = "nonzero"
stderr = ["[struct] default for", "requires type i, k or S"]
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

struct Event velocity:i
struct Bad event:Event = 100

instr 1
  value:Bad init
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
