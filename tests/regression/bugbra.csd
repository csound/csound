<CsoundSynthesizer>

<CsInstruments>
   instr 1
k1    line    0, p3, 2
k2    tonek (k1/2), 1
a1    oscil    k2, 440
   outs    a1, a1
   endin

</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>

</CsoundSynthesizer>
