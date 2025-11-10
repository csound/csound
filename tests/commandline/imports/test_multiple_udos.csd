<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

import simple_test

/* Define another UDO locally */
opcode LocalOsc, a, k
    kfreq xin
    aout oscili 0.3, kfreq, -1, 0
    xout aout
endop

instr 1
    /* Use both imported and local UDOs */
    a1 = TestOsc(440)
    a2 = LocalOsc(550)
    aout = a1 + a2
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
