<CsTest>
expect.exit=1
expect.stderr=["Instruments in modules must be named"]
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
import "numbered.orc"
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
