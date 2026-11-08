<CsTest>
expect.exit=0
description="Nested modules retain scoped aliases and share initialized dependencies"
expect.output=["MODULE_NESTED_PASS"]
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
from "left.orc" import ReadLeft,giDerived
from "right.orc" import ReadRight
import "diamond_left.orc"
import "diamond_right.orc"
instr 1
 iLeft=ReadLeft()
 iRight=ReadRight()
 assert iLeft==11 && iRight==22 && giDerived==12
 iCount chnget "module_init_count"
 assert iCount==1
 assert giLeft==1 && giRight==1
 prints "MODULE_NESTED_PASS\n"
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
