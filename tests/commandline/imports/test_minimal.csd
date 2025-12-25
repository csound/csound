<CsoundSynthesizer>
<CsOptions>
-d -n
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Import the oscillators module */
import "oscillators.orc"

/* Simple test - just compile, don't run */
instr 1
    aout = SimpleOsc(0.5, 440, 0)
endin
</CsInstruments>

<CsScore>
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>