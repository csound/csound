/* filters.orc: Filter module test
 * Copyright (C) 2025 Csound Developers
 */

opcode LowPass, a, ak
    asig, kfreq xin
    aout tone(asig, kfreq)
    xout aout
endop

opcode HighPass, a, ak
    asig, kfreq xin
    aout atone(asig, kfreq)
    xout aout
endop

opcode BandPass, a, akk
    asig, klow, khigh xin
    a1 tone(asig, klow)
    a2 atone(a1, khigh)
    xout a2
endop

/* Module variables */
giFilterCount = 3