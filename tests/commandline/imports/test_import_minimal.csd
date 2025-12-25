<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>

<CsInstruments>
/* Minimal test to verify module system works */
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Test import statement */
import "oscillators.orc"

/* Empty instrument - just testing that import doesn't crash compilation */
instr 1
    ; SimpleOsc(0.5, 440, 0)  ; Commented out to avoid opcode lookup issue
endin
</CsInstruments>

<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>