<CsTest>
expect.exit=1
expect.stderr=["Circular import detected"]
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
import "cycle.orc"
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
