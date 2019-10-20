<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1

idec = p4
iamp = db(idec)
     print iamp
asig vco2 iamp, 110			;sawtooth
     outs asig, asig

endin

</CsInstruments>
<CsScore>

i 1 0 1 50
i 1 + 1 >
i 1 + 1 >
i 1 + 1 85
e

</CsScore>
</CsoundSynthesizer>
