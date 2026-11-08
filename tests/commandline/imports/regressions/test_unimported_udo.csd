<CsTest>
expect.exit=1
expect.stderr=["was not imported"]
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
from "a.orc" import giValue
instr 1
 iResult=GetValue(0)
endin
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
