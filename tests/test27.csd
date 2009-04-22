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
           kc   chnget    "cutoff"
           a1   oscil     p4, p5, 100
           a2   lowpass2  a1, kc, 200
                out       a2
        endin

</CsInstruments>

<CsScore>
f 100  0 16384 10 1
i 1 0 3 10000 440
e

</CsScore>

</CsoundSynthesizer>