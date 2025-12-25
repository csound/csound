<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>

<CsInstruments>
/* Test import statements */
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Import the oscillators module */
import "oscillators.orc"

/* Test instrument that uses imported functionality */
instr 1
    aout = SimpleOsc(0.5, 440, 0)
    out aout, aout
endin

instr 2
    aout = SineOsc(0.3, 880)
    out aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 2
i 2 2 2
e
</CsScore>
</CsoundSynthesizer>