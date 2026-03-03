<CsoundSynthesizer>

<CsInstruments>
;

sr = 44100
ksmps = 100
nchnls = 1

chn_k "cutoff", 3

instr   1
kc   chnget    "cutoff"
a1   vco2     p4, p5
a2   lowpass2  a1, kc, 200
out       a2
endin

instr 2
 chnset p4, "cutoff"
endin 

</CsInstruments>

<CsScore>
i 2 0 1 1000
i 1 1 3 10000 440
i 2 3 2 10000
e

</CsScore>

</CsoundSynthesizer>