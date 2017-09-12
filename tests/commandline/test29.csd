<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=10
nchnls=1

        sr = 44100
        ksmps = 100
        nchnls = 1

        instr   1
           kc   =      p3 & 255
           ab   =      p4 | 15
           kd   =      p5
           a1   oscil     p4, kd<<2, 100
           a2   lowpass2  a1, p3, 200
eee:
                out       a2
        endin

</CsInstruments>

<CsScore>
f 100  0 16384 10 1
i 1 0 3 100 220
e

</CsScore>

</CsoundSynthesizer>