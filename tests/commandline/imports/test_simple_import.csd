<CsoundSynthesizer>
<CsOptions>
-d -n
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Import the simple test module */
import "test_simple.orc"

/* Test instrument that uses the imported functionality */
instr 1
    iResult = TestConst(123)
    print iResult
endin
</CsInstruments>

<CsScore>
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>