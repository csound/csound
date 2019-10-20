<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; White noise signal

asig rand 0.5
     outs asig, asig

endin



instr 2	; filtered noise

asig rand 0.7
alp  butterlp asig, 1000	  ;cutting frequencies above 1 KHz
     outs alp, alp

endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 2.5 2
e

</CsScore>
</CsoundSynthesizer>
