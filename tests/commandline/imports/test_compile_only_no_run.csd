<CsoundSynthesizer>
<CsOptions>
-d -n --syntax-check-only
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Test basic module import - compilation only */
import oscillators

/* Test that we can compile after import */
instr 1
    aout = SimpleOsc(0.5, 440, 0)
    out aout
endin
</CsInstruments>

<CsScore>
; Empty score - just test compilation
e
</CsScore>
</CsoundSynthesizer>
