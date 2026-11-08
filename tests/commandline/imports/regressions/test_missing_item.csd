<CsTest>
expect.exit=1
expect.stderr=["Item 'Missing' not found"]
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
from "a.orc" import Missing
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
