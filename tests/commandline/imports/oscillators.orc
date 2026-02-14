/* oscillators.orc: A simple test module for Csound module system
 * Copyright (C) 2025 Csound Developers
 * This file is part of Csound.
 */

/* Define some useful oscillators as UDOs */
opcode SimpleOsc(kamp, kfreq, kphase):a
    aout = kamp * oscili:a(kfreq, kphase)
    xout aout
endop

opcode SineOsc(kamp, kfreq):a
    aout = kamp * sin(kfreq * (2 * $M_PI) / sr)
    xout aout
endop

/* Module-level variables */
giModuleVersion = 1
; gSModuleName = "oscillators"  ; Example string global (left commented to keep fixture minimal)

/* Export a simple instrument that uses the module */
; instr SimpleOscInstrument
;     aout = SimpleOsc(0.5, 440, 0)
;     out aout
; endin
