<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs   =1

instr 1

itim = ftlptim(1)
     print itim
aout loscil3 .8, 40, 1
     outs aout, aout

endin
</CsInstruments>
<CsScore>
f 1 0 0 1 "Church.wav" 0 0 0 ;Csound computes tablesize

i 1 0 5
e
</CsScore>
</CsoundSynthesizer>
