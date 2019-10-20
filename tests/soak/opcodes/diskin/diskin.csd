<CsoundSynthesizer>
<CsInstruments>

sr     = 44100
ksmps  = 32
nchnls = 2
0dbfs  = 1

instr 1

ktrans linseg 1, 5, 2, 10, -2
a1     diskin "beats.wav", ktrans, 0, 1
       outs a1, a1

endin

</CsInstruments>
<CsScore>

i 1 0 15
e

</CsScore>
</CsoundSynthesizer>
