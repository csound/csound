/* oscillators.orc: A simple test module for Csound module system
 * Copyright (C) 2025 Csound Developers
 * This file is part of Csound.
 */

/* Define some useful oscillators as UDOs */
opcode SimpleOsc, a, kkk
    kamp, kfreq, kphase xin
    aout = kamp * oscili:a(kfreq, kphase)
    xout aout
endop

opcode SineOsc, a, kk
    kamp, kfreq xin
    aout = kamp * sin(kfreq * (2 * $M_PI) / sr)
    xout aout
endop

/* Module-level variables */
giModuleVersion = 1
; gsModuleName = "oscillators"  ; String variables need special handling - disabled for now

/* Export a simple instrument that uses the module */
; instr SimpleOscInstrument
;     aout = SimpleOsc(0.5, 440, 0)
;     out aout
; endin