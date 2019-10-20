<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2

instr 1

ksig  oscil 2, 0.5, 1
kint  = int(ksig)
ktrig changed kint

endin

</CsInstruments>
<CsScore>
f 1 0 1024 10 1
i 1 0 20

e
</CsScore>
</CsoundSynthesizer>
