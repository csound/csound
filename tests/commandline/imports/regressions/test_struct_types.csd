<CsTest>
expect.exit=0
expect.output=["MODULE_TYPES_PASS"]
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
from "type_first.orc" import Point as First,MakePoint as MakeFirst,MakePoints
from "type_second.orc" import Point as Second,MakePoint as MakeSecond
struct Wrapper point:First
instr 1
 p:First=MakeFirst()
 q:Second=MakeSecond()
 w:Wrapper init p
 points:First[]=MakePoints()
 assert w.point.y==12 && q.z==22
 assert points[0].x==31 && points[0].y==32
 prints "MODULE_TYPES_PASS\n"
endin
</CsInstruments>
<CsScore>
i1 0 .01
e
</CsScore>
</CsoundSynthesizer>
