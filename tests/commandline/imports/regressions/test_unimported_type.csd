<CsTest>
expect.exit=1
expect.stderr=["Unknown type annotation: Point"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=44100
ksmps=32
nchnls=2
0dbfs=1
from "type_first.orc" import MakePoint
instr 1
 p:Point init 1,2
endin
</CsInstruments>
<CsScore>
i1 0 .01
e
</CsScore>
</CsoundSynthesizer>
