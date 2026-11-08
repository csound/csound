<CsTest>
expect.exit=1
expect.stderr=["unable to find opcode"]
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
import "a.orc" as one
instr 1
 iResult one.abs -1
endin
</CsInstruments>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
