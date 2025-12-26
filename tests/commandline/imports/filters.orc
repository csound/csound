/* filters.orc: Filter module test
 * Copyright (C) 2025 Csound Developers
 */

opcode LowPass(asig, kfreq):a
    aout = tone(asig, kfreq)
    xout aout
endop

opcode HighPass(asig, kfreq):a
    aout = atone(asig, kfreq)
    xout aout
endop

opcode BandPass(asig, klow, khigh):a
    a1 = tone(asig, klow)
    aout = atone(a1, khigh)
    xout aout
endop

/* Module variables */
giFilterCount = 3
