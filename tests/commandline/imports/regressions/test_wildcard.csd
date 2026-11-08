<CsTest>
expect.exit=0
expect.output=["MODULE_WILDCARD_PASS"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --run-unit-tests
</CsOptions>
<CsInstruments>
sr=44100
ksmps=32
nchnls=2
0dbfs=1
from "a.orc" import *
instr 1
 assert giValue==11 && giValues[2]==3
 iResult=GetValue(2)
 assert iResult==13
 prints "MODULE_WILDCARD_PASS\n"
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
