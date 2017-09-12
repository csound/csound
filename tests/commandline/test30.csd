<CsoundSynthesizer>

<CsInstruments>
;
sr=44100
ksmps=10
nchnls=1

        sr = 44100
        ksmps = 100
        nchnls = 1

        instr  1
           a1   oscil     p4, p5, 100
           a2   lowpass2  a1, p3, 200
                out       a2+a1
        endin
        instr  2, 3
           a1   oscil     p4, p5, 100
           a2   lowpass2  a1, p3, 230
                out       a2+a1
        endin

</CsInstruments>

<CsScore>
f 100  0 16384 10 1
i 1 0 3 10000 220
i 3 1 3 10000 235
e

</CsScore>

</CsoundSynthesizer>