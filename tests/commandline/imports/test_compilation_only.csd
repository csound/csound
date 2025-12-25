<CsoundSynthesizer>
<CsOptions>
-d -n
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* Test basic module import - this should work without segfaults */
import "oscillators.orc"

/* Test that we can compile after import - instrument should exist but won't actually run */
instr 1
    ; This won't actually produce sound since SimpleOsc is a stub,
    ; but it demonstrates that the import system works
    aout = SimpleOsc(0.5, 440, 0)
    out aout
endin
</CsInstruments>

<CsScore>
; Don't even schedule the instrument - just test compilation
; If we get here, the module system worked!
i 1 0 0
e
</CsScore>
</CsoundSynthesizer>