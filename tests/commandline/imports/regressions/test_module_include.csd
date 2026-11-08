<CsTest>
expect.exit=0
expect.output=["MODULE_INCLUDE_PASS"]
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
from "include_fixture/library.orc" import giIncluded
instr 1
 assert giIncluded==42
 prints "MODULE_INCLUDE_PASS\n"
endin
</CsInstruments>
<CsScore>
i1 0 .01
e
</CsScore>
</CsoundSynthesizer>
